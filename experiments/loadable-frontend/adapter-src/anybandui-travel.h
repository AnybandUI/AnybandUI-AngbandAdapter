/* Frontend travel-and-act intent. Movement, routing and pickup remain engine
 * commands; interruption discards the intent rather than retrying a route. */
enum anybandui_travel_stage {
	TRAVEL_IDLE,
	TRAVEL_START,
	TRAVEL_APPROACH,
	TRAVEL_STEP,
	TRAVEL_ARRIVED,
	TRAVEL_DIGGING
};
static enum anybandui_travel_stage travel_stage;
static struct loc travel_grid, travel_waypoint;
static unsigned long travel_level;
static bool pickup_automatic;
static const char *travel_stop_reason;
static cmd_code travel_command = CMD_PICKUP;

static cmd_code anybandui_terrain_command(struct loc grid)
{
	if (!(*frontend_engine_ptr->p_player) || !(*frontend_engine_ptr->p_player)->cave || !(*frontend_engine_ptr->p_square_in_bounds_fully)((*frontend_engine_ptr->p_player)->cave, grid))
		return CMD_NULL;
	if ((*frontend_engine_ptr->p_square_isdisarmabletrap)((*frontend_engine_ptr->p_player)->cave, grid))
		return CMD_DISARM;
	if ((*frontend_engine_ptr->p_square_iscloseddoor)((*frontend_engine_ptr->p_player)->cave, grid))
		return CMD_OPEN;
	if ((*frontend_engine_ptr->p_square_isopendoor)((*frontend_engine_ptr->p_player)->cave, grid))
		return CMD_CLOSE;
	if ((*frontend_engine_ptr->p_square_isupstairs)((*frontend_engine_ptr->p_player)->cave, grid))
		return CMD_GO_UP;
	if ((*frontend_engine_ptr->p_square_isdownstairs)((*frontend_engine_ptr->p_player)->cave, grid))
		return CMD_GO_DOWN;
	if ((*frontend_engine_ptr->p_square_isdiggable)((*frontend_engine_ptr->p_player)->cave, grid) &&
	    !(*frontend_engine_ptr->p_square_isperm)((*frontend_engine_ptr->p_player)->cave, grid))
		return CMD_TUNNEL;
	return CMD_NULL;
}

static const char *anybandui_terrain_action(cmd_code command)
{
	return command == CMD_GO_UP     ? "up"
	       : command == CMD_GO_DOWN ? "down"
	       : command == CMD_TUNNEL  ? "tunnel"
	       : command == CMD_DISARM  ? "disarm"
	       : command == CMD_OPEN    ? "open"
	       : command == CMD_CLOSE   ? "close"
	                                : "";
}

static bool anybandui_adjacent_action(cmd_code command)
{
	return command == CMD_TUNNEL || command == CMD_DISARM ||
	       command == CMD_OPEN || command == CMD_CLOSE;
}

static void anybandui_capture_terrain_actions(cJSON *state)
{
	cJSON *view = cJSON_GetObjectItem(state, "dungeon"),
	      *actions = cJSON_CreateArray();
	int x, y;
	if (view)
		for (y = num(view, "y", 0);
		     y < num(view, "y", 0) + num(view, "height", 0); ++y)
			for (x = num(view, "x", 0);
			     x < num(view, "x", 0) + num(view, "width", 0); ++x) {
				cmd_code command = anybandui_terrain_command((*frontend_engine_ptr->p_loc)(x, y));
				if (command != CMD_NULL) {
					cJSON *entry = cJSON_CreateObject();
					number(entry, "x", x);
					number(entry, "y", y);
					bool adjacent = (*frontend_engine_ptr->p_distance)((*frontend_engine_ptr->p_player)->grid, (*frontend_engine_ptr->p_loc)(x, y)) == 1;
					const char *hint =
					        command == CMD_DISARM
					                ? (adjacent && !(*frontend_engine_ptr->p_player_is_trapsafe)((*frontend_engine_ptr->p_player))
					                           ? "Click to attempt disarming; "
					                             "right-click for Disarm."
					                           : "Right-click to approach and "
					                             "disarm.")
					        : command == CMD_OPEN
					                ? (adjacent ? "Click to attempt opening; "
					                              "right-click for Open."
					                            : "Right-click to approach and "
					                              "open.")
					        : command == CMD_CLOSE ? "Click to move through; "
					                                 "right-click to close."
					        : command == CMD_TUNNEL
					                ? "Right-click to approach and tunnel."
					        : command == CMD_GO_UP
					                ? "Right-click to approach and go up."
					                : "Right-click to approach and go down.";
					if (!OPT((*frontend_engine_ptr->p_player), mouse_movement))
						hint = "Mouse movement is disabled; terrain actions "
						       "are available on right-click.";
					else if ((*frontend_engine_ptr->p_player)->timed[TMD_CONFUSED])
						hint = "Confused: clicks attempt a random step.";
					else if (adjacent && (*frontend_engine_ptr->p_square_monster)((*frontend_engine_ptr->p_cave), (*frontend_engine_ptr->p_loc)(x, y)) &&
					         (*frontend_engine_ptr->p_monster_is_visible)(
					                 (*frontend_engine_ptr->p_square_monster)((*frontend_engine_ptr->p_cave), (*frontend_engine_ptr->p_loc)(x, y))))
						hint = "Click to attack; the creature may block "
						       "terrain actions.";
					string(entry, "hint", hint);
					string(entry, "action", anybandui_terrain_action(command));
					cJSON_AddItemToArray(actions, entry);
				}
			}
	cJSON_AddItemToObject(state, "terrain_actions", actions);
}

/* An attempt is based on the displayed observation, including remembered or
 * hallucinated objects. Do not consult the real pile until we arrive. */
static bool anybandui_pickup_observed(struct loc grid)
{
	struct map_visual visual;
	if (!(*frontend_engine_ptr->p_square_in_bounds_fully)((*frontend_engine_ptr->p_cave), grid)) return false;
	map_visual_readonly(grid, &visual);
	return visual.object_char != 0;
}

static bool anybandui_pickup_possible(struct loc grid)
{
	struct object *o;
	if (!(*frontend_engine_ptr->p_square_in_bounds_fully)((*frontend_engine_ptr->p_cave), grid))
		return false;
	for (o = (*frontend_engine_ptr->p_square_object)((*frontend_engine_ptr->p_cave), grid); o; o = o->next)
		if (!(*frontend_engine_ptr->p_ignore_item_ok)((*frontend_engine_ptr->p_player), o) &&
		    ((*frontend_engine_ptr->p_tval_is_money)(o) || (*frontend_engine_ptr->p_inven_carry_okay)(o)))
			return true;
	return false;
}

static void anybandui_travel_event(game_event_type type, game_event_data *data,
                                   void *user)
{
	if (type == EVENT_AUTOPICKUP_BEGIN)
		pickup_automatic = true;
	else if (type == EVENT_AUTOPICKUP_END)
		pickup_automatic = false;
	else if (type == EVENT_INPUT_FLUSH) {
		/* Noticing the destination's pile is an ordinary part of arriving.
		 * Damage, traps, sightings and manual interruption still cancel the
		 * intent. */
		if (!(pickup_automatic && (*frontend_engine_ptr->p_loc_eq)((*frontend_engine_ptr->p_player)->grid, travel_grid)))
			travel_stage = TRAVEL_IDLE;
	} else
		travel_stage = TRAVEL_IDLE;
}

static bool anybandui_travel_continue(void)
{
	if (travel_stage == TRAVEL_IDLE)
		return false;
	bool adjacent = anybandui_adjacent_action(travel_command);
	if (travel_level != anybandui_level || (*frontend_engine_ptr->p_player)->is_dead ||
	    ((*frontend_engine_ptr->p_player)->timed[TMD_CONFUSED] &&
	     !(travel_stage == TRAVEL_START && adjacent &&
	       (*frontend_engine_ptr->p_distance)((*frontend_engine_ptr->p_player)->grid, travel_grid) == 1))) {
		travel_stage = TRAVEL_IDLE;
		return false;
	}
	if (travel_stage == TRAVEL_START && adjacent) {
		int best = -1, dir;
		if ((*frontend_engine_ptr->p_distance)((*frontend_engine_ptr->p_player)->grid, travel_grid) == 1) {
			best = 0;
			travel_waypoint = (*frontend_engine_ptr->p_player)->grid;
		}
		for (dir = 1; best != 0 && dir <= 9; ++dir)
			if (dir != 5) {
				struct loc adjacent = (*frontend_engine_ptr->p_loc_sum)(travel_grid, (*frontend_engine_ptr->p_ddgrid)[dir]);
				int16_t *steps = NULL;
				int count;
				if (!(*frontend_engine_ptr->p_square_in_bounds_fully)((*frontend_engine_ptr->p_player)->cave, adjacent) ||
				    !(*frontend_engine_ptr->p_square_ispassable)((*frontend_engine_ptr->p_player)->cave, adjacent))
					continue;
				count = (*frontend_engine_ptr->p_find_path)((*frontend_engine_ptr->p_player), (*frontend_engine_ptr->p_player)->grid, adjacent, &steps);
				(*frontend_engine_ptr->p_mem_free)(steps);
				if (count >= 0 && (best < 0 || count < best)) {
					best = count;
					travel_waypoint = adjacent;
				}
			}
		if (best < 0) {
			travel_stop_reason =
			        "No route to an adjacent position could be found.";
			travel_stage = TRAVEL_IDLE;
			return false;
		}
		if (best > 0) {
			travel_stage = TRAVEL_APPROACH;
			(*frontend_engine_ptr->p_cmdq_push)(CMD_PATHFIND);
			(*frontend_engine_ptr->p_cmd_set_arg_point)((*frontend_engine_ptr->p_cmdq_peek)(), "point", travel_waypoint);
			return true;
		}
		travel_stage = TRAVEL_ARRIVED;
	}
	if (travel_stage == TRAVEL_START) {
		if ((*frontend_engine_ptr->p_loc_eq)((*frontend_engine_ptr->p_player)->grid, travel_grid))
			travel_stage = TRAVEL_ARRIVED;
		else {
			int16_t *steps = NULL;
			int count = (*frontend_engine_ptr->p_find_path)((*frontend_engine_ptr->p_player), (*frontend_engine_ptr->p_player)->grid, travel_grid, &steps);
			if (count <= 0) {
				(*frontend_engine_ptr->p_mem_free)(steps);
				travel_stop_reason = "No walking route could be found.";
				travel_stage = TRAVEL_IDLE;
				return false;
			}
			/* Ordinary running stops before visible objects. Route to the
			 * previous square, then take one normal step, without weakening
			 * running checks. */
			travel_waypoint = (*frontend_engine_ptr->p_loc_diff)(travel_grid, (*frontend_engine_ptr->p_ddgrid)[steps[0]]);
			(*frontend_engine_ptr->p_mem_free)(steps);
			if (count > 1) {
				travel_stage = TRAVEL_APPROACH;
				(*frontend_engine_ptr->p_cmdq_push)(CMD_PATHFIND);
				(*frontend_engine_ptr->p_cmd_set_arg_point)((*frontend_engine_ptr->p_cmdq_peek)(), "point", travel_waypoint);
				return true;
			}
			travel_stage = TRAVEL_STEP;
		}
	}
	if (travel_stage == TRAVEL_APPROACH) {
		if (!(*frontend_engine_ptr->p_loc_eq)((*frontend_engine_ptr->p_player)->grid, travel_waypoint)) {
			travel_stage = TRAVEL_IDLE;
			return false;
		}
		travel_stage = adjacent ? TRAVEL_ARRIVED : TRAVEL_STEP;
	}
	if (travel_stage == TRAVEL_STEP) {
		if ((*frontend_engine_ptr->p_square_monster)((*frontend_engine_ptr->p_cave), travel_grid)) {
			travel_stop_reason = "Travel stopped: destination occupied.";
			travel_stage = TRAVEL_IDLE;
			return false;
		}
		travel_stage = TRAVEL_ARRIVED;
		(*frontend_engine_ptr->p_cmdq_push)(CMD_WALK);
		(*frontend_engine_ptr->p_cmd_set_arg_direction)((*frontend_engine_ptr->p_cmdq_peek)(), "direction",
		                      (*frontend_engine_ptr->p_motion_dir)((*frontend_engine_ptr->p_player)->grid, travel_grid));
		return true;
	}
	travel_stage = TRAVEL_IDLE;
	if (adjacent) {
		if (!(*frontend_engine_ptr->p_loc_eq)((*frontend_engine_ptr->p_player)->grid, travel_waypoint) ||
		    anybandui_terrain_command(travel_grid) != travel_command)
			return false;
		if (travel_command == CMD_TUNNEL && (*frontend_engine_ptr->p_square_monster)((*frontend_engine_ptr->p_cave), travel_grid))
			return false;
		/* Continue after the engine's repeat batch expires. Success, futile
		 * digging, danger and manual cancellation call disturb() and discard
		 * this intent. */
		if (travel_command == CMD_TUNNEL)
			travel_stage = TRAVEL_DIGGING;
		(*frontend_engine_ptr->p_cmdq_push)(travel_command);
		(*frontend_engine_ptr->p_cmd_set_arg_direction)((*frontend_engine_ptr->p_cmdq_peek)(), "direction",
		                      (*frontend_engine_ptr->p_motion_dir)((*frontend_engine_ptr->p_player)->grid, travel_grid));
		return true;
	}
	if (travel_command == CMD_GO_UP || travel_command == CMD_GO_DOWN) {
		if (!(*frontend_engine_ptr->p_loc_eq)((*frontend_engine_ptr->p_player)->grid, travel_grid))
			return false;
		(*frontend_engine_ptr->p_cmdq_push)(travel_command);
		return true;
	}
	if (!(*frontend_engine_ptr->p_loc_eq)((*frontend_engine_ptr->p_player)->grid, travel_grid) ||
	    !anybandui_pickup_possible(travel_grid))
		return false;
	/* The normal pickup command owns capacity checks and item-selection
	 * prompts. Queue it directly: arrival's input flush must not eat synthetic
	 * keystrokes. */
	(*frontend_engine_ptr->p_cmdq_push)(CMD_PICKUP);
	return true;
}

/* Presentation-only tracking; never changes the movement or disturbance rules.
 */
static bool travel_display_active, travel_display_attack;
static struct loc travel_display_grid;
static unsigned long travel_display_level;
static cmd_code travel_display_command;
static void anybandui_travel_feedback(const char *label, bool active,
                                      bool interrupted)
{
	cJSON *v = cJSON_CreateObject();
	string(v, "label", label);
	json_bool(v, "active", active);
	json_bool(v, "interrupted", interrupted);
	number(v, "x", travel_display_grid.x);
	number(v, "y", travel_display_grid.y);
	counter(v, "level_id", travel_display_level);
	event("travel.changed", v);
	travel_display_active = active;
}
static void anybandui_travel_begin(struct loc grid, cmd_code command)
{
	travel_stop_reason = NULL;
	travel_display_attack = command == CMD_WALK &&
	                        (*frontend_engine_ptr->p_distance)((*frontend_engine_ptr->p_player)->grid, grid) <= 1 &&
	                        (*frontend_engine_ptr->p_square_monster)((*frontend_engine_ptr->p_cave), grid) != NULL;
	travel_display_grid = grid;
	travel_display_level = anybandui_level;
	travel_display_command = command;
	anybandui_travel_feedback(
	        travel_display_attack    ? "Engaging adjacent creature"
	        : command == CMD_PICKUP  ? "Walking to pick up"
	        : command == CMD_TUNNEL  ? "Approaching wall to tunnel"
	        : command == CMD_GO_UP   ? "Walking to ascend"
	        : command == CMD_GO_DOWN ? "Walking to descend"
	                                 : "Moving to destination",
	        true, false);
}
static void anybandui_travel_finish(void)
{
	bool arrived;
	if (!travel_display_active)
		return;
	arrived = (*frontend_engine_ptr->p_loc_eq)((*frontend_engine_ptr->p_player)->grid, travel_display_grid);
	if ((travel_display_command == CMD_GO_UP ||
	     travel_display_command == CMD_GO_DOWN) &&
	    travel_display_level != anybandui_level)
		anybandui_travel_feedback("Stairs used", false, false);
	else if (travel_display_level != anybandui_level)
		anybandui_travel_feedback("Travel stopped: level changed", false, true);
	else if (travel_display_command == CMD_TUNNEL &&
	         anybandui_terrain_command(travel_display_grid) != CMD_TUNNEL)
		anybandui_travel_feedback("Passage opened", false, false);
	else if (travel_display_attack)
		anybandui_travel_feedback("Interaction finished", false, false);
	else if (arrived)
		/* Ordinary arrivals clear the destination marker without a
		 * notification. */
		anybandui_travel_feedback(travel_display_command == CMD_PICKUP
		                                  ? "Arrived: pickup attempted"
		                                  : "",
		                          false, false);
	else
		anybandui_travel_feedback(travel_stop_reason ? travel_stop_reason : "",
		                          false, true);
}
static void anybandui_route_preview(const char *id, const cJSON *p)
{
	struct loc dest = (*frontend_engine_ptr->p_loc)(num(p, "x", -1), num(p, "y", -1)), at;
	int16_t *steps = NULL;
	int count, i;
	cJSON *out, *points;
	const cJSON *x = cJSON_GetObjectItem(p, "x"),
	            *y = cJSON_GetObjectItem(p, "y");
	if (!ready || active_prompt || !(*frontend_engine_ptr->p_character_generated) ||
	    !streq(phase, "playing")) {
		error(id, "busy", "Route previews require normal play.");
		return;
	}
	if (!streq(str(p, "context"), context_text)) {
		error(id, "stale_revision", "View changed.");
		return;
	}
	if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y) ||
	    x->valuedouble != x->valueint || y->valuedouble != y->valueint ||
	    !(*frontend_engine_ptr->p_square_in_bounds_fully)((*frontend_engine_ptr->p_player)->cave, dest)) {
		error(id, "invalid_argument", "Invalid route destination.");
		return;
	}
	/* Use the same knowledge, door and trap costs as ordinary mouse
	 * pathfinding. */
	count = (*frontend_engine_ptr->p_find_path)((*frontend_engine_ptr->p_player), (*frontend_engine_ptr->p_player)->grid, dest, &steps);
	out = cJSON_CreateObject();
	points = cJSON_CreateArray();
	at = (*frontend_engine_ptr->p_player)->grid;
	for (i = count - 1; i >= 0; --i) {
		int xy[2];
		at = (*frontend_engine_ptr->p_loc_sum)(at, (*frontend_engine_ptr->p_ddgrid)[steps[i]]);
		xy[0] = at.x;
		xy[1] = at.y;
		cJSON_AddItemToArray(points, ints(xy, 2));
	}
	(*frontend_engine_ptr->p_mem_free)(steps);
	string(out, "context", context_text);
	number(out, "x", dest.x);
	number(out, "y", dest.y);
	json_bool(out, "reachable", count >= 0);
	cJSON_AddItemToObject(out, "path", points);
	response(id, out);
}
