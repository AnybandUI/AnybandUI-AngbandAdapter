/* Bounded, observational movement events; never infer walking from positions.
 */
static cJSON *anybandui_motion;
static unsigned long anybandui_motion_level;
struct anybandui_motion_data {
	struct loc from, to;
	int index;
	bool blink, visible;
};
static void anybandui_motion_record(const struct anybandui_motion_data *data)
{
	cJSON *v, *tiles;
	int x, y;
	if (!data || !data->visible)
		return;
	if (anybandui_motion && anybandui_motion_level != anybandui_level) {
		cJSON_Delete(anybandui_motion);
		anybandui_motion = NULL;
	}
	if (!anybandui_motion) {
		anybandui_motion = cJSON_CreateArray();
		anybandui_motion_level = anybandui_level;
	}
	if (cJSON_GetArraySize(anybandui_motion) >= 128)
		return;
	v = cJSON_CreateObject();
	number(v, "index", data->index);
	json_bool(v, "blink", data->blink);
	number(v, "x", data->from.x);
	number(v, "y", data->from.y);
	number(v, "tx", data->to.x);
	number(v, "ty", data->to.y);
	tiles = cJSON_AddArrayToObject(v, "tiles");
	if (data->blink)
		for (y = -3; y <= 3; ++y)
			for (x = -3; x <= 3; ++x) {
				struct loc grid = loc(data->from.x + x, data->from.y + y);
				int tile[2] = {grid.x, grid.y};
				if (x * x + y * y <= 9 && square_in_bounds(cave, grid) &&
				    square_isseen(cave, grid))
					cJSON_AddItemToArray(tiles, cJSON_CreateIntArray(tile, 2));
			}
	cJSON_AddItemToArray(anybandui_motion, v);
}
static void anybandui_motion_publish(void)
{
	if (anybandui_motion) {
		cJSON *v = cJSON_CreateObject();
		counter(v, "level_id", anybandui_motion_level);
		cJSON_AddItemToObject(v, "effects", anybandui_motion);
		event("motion.feedback", v);
		anybandui_motion = NULL;
	}
}

/* The engine reports movement boundaries, not frontend animation policy.
 * Pair observations, including nested dispatch, without inferring movement
 * from turn snapshots or consuming gameplay randomness. */
struct anybandui_walk {
	struct anybandui_motion_data motion;
	struct monster *monster;
	struct anybandui_walk *previous;
};
static struct anybandui_walk *anybandui_walks;
static void anybandui_motion_event(game_event_type type, game_event_data *data,
                                   void *user)
{
	struct loc grid = data->point;
	struct monster *mon = square_monster(cave, grid);
	bool seen = mon && monster_is_visible(mon) && !monster_is_camouflaged(mon);
	if (type == EVENT_MONSTER_MOVE_BEGIN) {
		struct anybandui_walk *walk = mem_zalloc(sizeof(*walk));
		walk->motion.from = grid;
		walk->motion.index = mon ? mon->midx : 0;
		walk->motion.visible = seen;
		walk->monster = mon;
		walk->previous = anybandui_walks;
		anybandui_walks = walk;
	} else if (type == EVENT_MONSTER_MOVE_END) {
		struct anybandui_walk *walk = anybandui_walks;
		assert(walk);
		anybandui_walks = walk->previous;
		walk->motion.to = grid;
		walk->motion.visible = walk->motion.visible &&
		                       monster_is_visible(walk->monster) &&
		                       !player->timed[TMD_IMAGE];
		anybandui_motion_record(&walk->motion);
		mem_free(walk);
	} else {
		struct anybandui_motion_data motion = {
		        grid, grid, mon ? mon->midx : 0, true,
		        (mon ? seen : loc_eq(player->grid, grid)) &&
		                !player->timed[TMD_IMAGE]};
		anybandui_motion_record(&motion);
	}
}
