/* Semantic presentation adapter, included by main-anybandui.c after JSON
 * helpers. The classic viewport and the free camera both use
 * the engine's read-only known-map renderer for the full level. No
 * terminal-cell parsing, RNG use, pointer handles or client gameplay rules. */
static unsigned long anybandui_level;
static bool anybandui_full_map;
static int anybandui_view_width, anybandui_view_height, anybandui_view_x,
        anybandui_view_y;
static bool anybandui_view_initialized;

/* Looking may cycle over terrain/items as well as monsters. Confirm those as
 * locations using the engine's existing free-cursor controls. Kill targeting
 * retains its ordinary monster eligibility checks. */
static bool anybandui_look_location(void)
{
	return target_ui_current && (target_ui_current->mode & TARGET_LOOK) &&
	       target_ui_current->interesting && !target_ui_current->can_confirm &&
	       (*frontend_engine_ptr->p_square_in_bounds_fully)((*frontend_engine_ptr->p_cave), target_ui_current->grid);
}
static void anybandui_target_key(int key)
{
	if (!(*frontend_engine_ptr->p_screen_save_depth) && anybandui_look_location() &&
	    (key == 't' || key == '5' || key == '0' || key == '.'))
		(*frontend_engine_ptr->p_Term_keypress)('o', 0);
	(*frontend_engine_ptr->p_Term_keypress)(key, 0);
}

static void anybandui_reset_view(void)
{
	anybandui_view_initialized = false;
	++anybandui_level;
}
static void anybandui_view_event(game_event_type type, game_event_data *data,
	void *user)
{
	anybandui_reset_view();
}

static void anybandui_capture_view(cJSON *state_record)
{
	int x, y, width, height, ox, oy;
	cJSON *view, *rows, *observed_items;
	/* Leaving the world adds one screen depth before flushing post-fatal spell
	 * messages. There is no overlay yet: retain the dungeon and native ribbon
	 * until these are acknowledged. Nested screens still own the terminal. */
	const bool final_messages =
	        (*frontend_engine_ptr->p_player) && (*frontend_engine_ptr->p_player)->is_dead && (*frontend_engine_ptr->p_textui_message_pending) &&
	        (*frontend_engine_ptr->p_screen_save_depth) == 1 && streq(phase, "playing");
	/* Native targeting/aiming share the dungeon; nested recall screens still
	 * own the terminal. Presentation mode never depends on parsing terminal
	 * text. */
	if ((!ready && !native_prompt && !(*frontend_engine_ptr->p_textui_message_pending) &&
	     !target_ui_current && !textui_aiming && !anybandui_direction &&
	     !item_choice_objects && !spell_selection) ||
	    (active_prompt && !native_prompt) ||
	    ((*frontend_engine_ptr->p_screen_save_depth) && !final_messages) || !streq(phase, "playing") ||
	    !(*frontend_engine_ptr->p_cave) || !(*frontend_engine_ptr->p_player)->cave)
		return;
	ox = anybandui_full_map ? 0 : terminal.offset_x;
	oy = anybandui_full_map ? 0 : terminal.offset_y;
	width = anybandui_full_map ? (*frontend_engine_ptr->p_cave)->width
	                           : MIN(SCREEN_WID, (*frontend_engine_ptr->p_cave)->width - ox);
	height = anybandui_full_map ? (*frontend_engine_ptr->p_cave)->height
	                            : MIN(SCREEN_HGT, (*frontend_engine_ptr->p_cave)->height - oy);
	if (width < 1 || height < 1 || terminal.offset_x < 0 ||
	    terminal.offset_y < 0)
		return;
	if (!anybandui_full_map && anybandui_view_width > 0) {
		const struct loc focus =
		        target_ui_current ? target_ui_current->grid : (*frontend_engine_ptr->p_player)->grid;
		width = MIN(anybandui_view_width, (*frontend_engine_ptr->p_cave)->width);
		height = MIN(anybandui_view_height, (*frontend_engine_ptr->p_cave)->height);
		const bool centered = OPT((*frontend_engine_ptr->p_player), center_player) &&
		                      !(*frontend_engine_ptr->p_player)->upkeep->running && !target_ui_current;
		if (!anybandui_view_initialized || centered ||
		    focus.x < anybandui_view_x + 3 ||
		    focus.x >= anybandui_view_x + width - 3)
			anybandui_view_x = focus.x - width / 2;
		if (!anybandui_view_initialized || centered ||
		    focus.y < anybandui_view_y + 2 ||
		    focus.y >= anybandui_view_y + height - 2)
			anybandui_view_y = focus.y - height / 2;
		anybandui_view_x = MAX(0, MIN(anybandui_view_x, (*frontend_engine_ptr->p_cave)->width - width));
		anybandui_view_y = MAX(0, MIN(anybandui_view_y, (*frontend_engine_ptr->p_cave)->height - height));
		ox = anybandui_view_x;
		oy = anybandui_view_y;
		anybandui_view_initialized = true;
	}
	view = cJSON_CreateObject();
	rows = cJSON_CreateArray();
	observed_items = cJSON_CreateArray();
	json_bool(state_record, "spell_selection", spell_selection);
	json_bool(state_record, "native_prompt", native_prompt);
	counter(view, "level_id", anybandui_level);
	number(view, "x", ox);
	number(view, "y", oy);
	json_bool(view, "full_level", anybandui_full_map);
	number(view, "width", width);
	number(view, "height", height);
	for (y = 0; y < height; ++y) {
		cJSON *row = cJSON_CreateArray();
		for (x = 0; x < width; ++x) {
			struct map_visual whole;
			const struct map_visual *v;
			map_visual_readonly((*frontend_engine_ptr->p_loc)(x + ox, y + oy), &whole);
			v = &whole;
			int cell[13] = {v->terrain_char, v->terrain_attr, v->trap_char,
			                v->trap_attr,    v->object_char,  v->object_attr,
			                v->actor_char,   v->actor_attr,   v->feature,
			                v->lighting,     v->seen,         v->hallucinated,
			                v->is_player};
			cJSON_AddItemToArray(row, ints(cell, 13));
			/* Describe remembered piles, not live-world objects at these
			 * coordinates. Copies keep object_desc's everseen bookkeeping out
			 * of read-only capture. */
			if (v->object_char && !v->hallucinated && (*frontend_engine_ptr->p_player)->cave) {
				struct loc grid = (*frontend_engine_ptr->p_loc)(x + ox, y + oy);
				const struct object *o;
				for (o = (*frontend_engine_ptr->p_square_object)((*frontend_engine_ptr->p_player)->cave, grid); o; o = o->next) {
					struct object copy = *o;
					char label[512];
					cJSON *entry;
					if (o->kind != (*frontend_engine_ptr->p_unknown_item_kind) &&
					    o->kind != (*frontend_engine_ptr->p_unknown_gold_kind) &&
					    (*frontend_engine_ptr->p_ignore_known_item_ok)((*frontend_engine_ptr->p_player), o))
						continue;
					if (o->kind == (*frontend_engine_ptr->p_unknown_item_kind))
						(*frontend_engine_ptr->p_my_strcpy)(label, "An unknown item", sizeof(label));
					else if (o->kind == (*frontend_engine_ptr->p_unknown_gold_kind))
						(*frontend_engine_ptr->p_my_strcpy)(label, "Unknown treasure", sizeof(label));
					else {
						copy.known = &copy;
						describe(&copy, false, label, sizeof(label), 0);
					}
					entry = cJSON_CreateObject();
					number(entry, "x", grid.x);
					number(entry, "y", grid.y);
					string(entry, "label", label);
					number(entry, "quantity", o->number);
					number(entry, "color",
					       o->kind->base ? o->kind->base->attr : COLOUR_WHITE);
					/* Visible magic is a presentation hint, even before
					 * identification. Match the actual floor object to its
					 * remembered record; descriptions above still use only
					 * player knowledge. Never expose offscreen magic. */
					const char *aura = "";
					if (v->seen && !v->actor_char && !v->is_player &&
					    o->kind != (*frontend_engine_ptr->p_unknown_item_kind) &&
					    o->kind != (*frontend_engine_ptr->p_unknown_gold_kind)) {
						const struct object *actual;
						for (actual = (*frontend_engine_ptr->p_square_object)((*frontend_engine_ptr->p_cave), grid); actual;
						     actual = actual->next) {
							if (actual->known != o)
								continue;
							if (cursed(actual))
								aura = "cursed";
							else if (actual->artifact)
								aura = "artifact";
							else
								for (int rune = 0; rune < (*frontend_engine_ptr->p_max_runes)(); ++rune)
									if ((*frontend_engine_ptr->p_object_has_rune)(actual, rune)) {
										aura = "rune";
										break;
									}
							break;
						}
					}
					string(entry, "aura", aura);
					cJSON_AddItemToArray(observed_items, entry);
				}
			}
		}
		cJSON_AddItemToArray(rows, row);
	}
	anybandui_capture_tiles(view, ox, oy, width, height);
	cJSON_AddItemToObject(view, "items", observed_items);
	cJSON_AddItemToObject(view, "cells", rows);
	cJSON_AddItemToObject(state_record, "dungeon", view);
	if (target_ui_current) {
		const struct target_ui_state *t = target_ui_current;
		cJSON *selection = cJSON_CreateObject(),
		      *candidates = cJSON_CreateArray(), *path = cJSON_CreateArray();
		int i;
		string(selection, "mode", (t->mode & TARGET_KILL) ? "target" : "look");
		number(selection, "x", t->grid.x);
		number(selection, "y", t->grid.y);
		json_bool(selection, "interesting", t->interesting);
		json_bool(selection, "can_confirm",
		          t->can_confirm || anybandui_look_location());
		for (i = 0; i < t->candidates->n; ++i) {
			int xy[2] = {t->candidates->pts[i].x, t->candidates->pts[i].y};
			cJSON_AddItemToArray(candidates, ints(xy, 2));
		}
		if (t->mode & TARGET_KILL)
			for (i = 0; i < t->path_length; ++i) {
				int xy[2] = {t->path[i].x, t->path[i].y};
				cJSON_AddItemToArray(path, ints(xy, 2));
			}
		cJSON_AddItemToObject(selection, "candidates", candidates);
		cJSON_AddItemToObject(selection, "path", path);
		cJSON_AddItemToObject(state_record, "targeting", selection);
	}
	if (textui_aiming ||
	    (target_ui_current && (target_ui_current->mode & TARGET_KILL)))
		number(state_record, "blast_radius", anybandui_blast_radius());
	json_bool(state_record, "aiming", textui_aiming);
	json_bool(state_record, "direction_prompt", anybandui_direction);
	json_bool(state_record, "item_selection", item_choice_objects != NULL);
}
static void anybandui_character_details(cJSON *p)
{
	anybandui_character_sheet(p);
	char feeling_description[256];
	const struct trap *trap = (*frontend_engine_ptr->p_square_trap)((*frontend_engine_ptr->p_cave), (*frontend_engine_ptr->p_player)->grid);
	const char *title = (*frontend_engine_ptr->p_player)->class->title[MIN(((*frontend_engine_ptr->p_player)->lev - 1) / 5, 9)];
	if ((*frontend_engine_ptr->p_player)->wizard)
		title = "Wizard";
	else if ((*frontend_engine_ptr->p_player)->total_winner)
		title = "Winner";
	else if ((*frontend_engine_ptr->p_player_is_shapechanged)((*frontend_engine_ptr->p_player)))
		title = (*frontend_engine_ptr->p_player)->shape->name;
	string(p, "title", title);
	number(p, "max_level", (*frontend_engine_ptr->p_player)->max_lev);
	number(p, "experience", (*frontend_engine_ptr->p_player)->exp);
	number(p, "max_experience", (*frontend_engine_ptr->p_player)->max_exp);
	number(p, "level_start_experience",
	       (*frontend_engine_ptr->p_player)->lev > 1 ? (int)((int64_t)(*frontend_engine_ptr->p_player_exp)[(*frontend_engine_ptr->p_player)->lev - 2] *
	                               (*frontend_engine_ptr->p_player)->expfact / 100)
	                       : 0);
	number(p, "next_level_experience",
	       (*frontend_engine_ptr->p_player)->lev < PY_MAX_LEVEL
	               ? (int)((int64_t)(*frontend_engine_ptr->p_player_exp)[(*frontend_engine_ptr->p_player)->lev - 1] *
	                       (*frontend_engine_ptr->p_player)->expfact / 100)
	               : 0);
	format_level_feeling(feeling_description, sizeof(feeling_description));
	string(p, "feeling_description", feeling_description);
	number(p, "depth_feet", (*frontend_engine_ptr->p_player)->depth * 50);
	number(p, "light", (*frontend_engine_ptr->p_square_light)((*frontend_engine_ptr->p_cave), (*frontend_engine_ptr->p_player)->grid));
	string(p, "floor",
	       trap && !(*frontend_engine_ptr->p_square_isinvis)((*frontend_engine_ptr->p_cave), (*frontend_engine_ptr->p_player)->grid)
	               ? trap->kind->name
	               : (*frontend_engine_ptr->p_square_feat)((*frontend_engine_ptr->p_cave), (*frontend_engine_ptr->p_player)->grid)->name);
	number(p, "recall", (*frontend_engine_ptr->p_player)->word_recall);
	number(p, "descent", (*frontend_engine_ptr->p_player)->deep_descent);
	number(p, "resting", (*frontend_engine_ptr->p_player)->upkeep->resting);
	number(p, "running", (*frontend_engine_ptr->p_player)->upkeep->running);
	number(p, "repeat", (*frontend_engine_ptr->p_cmd_get_nrepeats)());
	number(p, "study", (*frontend_engine_ptr->p_player)->upkeep->new_spells);
	number(p, "extra_moves", (*frontend_engine_ptr->p_player)->state.num_moves);
	json_bool(p, "unignoring", (*frontend_engine_ptr->p_player)->unignoring);
	json_bool(p, "trap_detected", (*frontend_engine_ptr->p_square_isdtrap)((*frontend_engine_ptr->p_cave), (*frontend_engine_ptr->p_player)->grid));
	if (OPT((*frontend_engine_ptr->p_player), birth_feelings) && (*frontend_engine_ptr->p_player)->depth) {
		int objects = (*frontend_engine_ptr->p_cave)->feeling / 10, monsters = (*frontend_engine_ptr->p_cave)->feeling % 10;
		char feeling[40], treasure[8];
		if ((*frontend_engine_ptr->p_cave)->feeling_squares < (*frontend_engine_ptr->p_z_info)->feeling_need)
			(*frontend_engine_ptr->p_my_strcpy)(treasure, "?", sizeof(treasure));
		else if (objects < 2)
			(*frontend_engine_ptr->p_my_strcpy)(treasure, objects ? "$" : "*", sizeof(treasure));
		else
			(*frontend_engine_ptr->p_strnfmt)(treasure, sizeof(treasure), "%d", 11 - objects);
		if (monsters)
			(*frontend_engine_ptr->p_strnfmt)(feeling, sizeof(feeling), "%d / %s", 10 - monsters,
			        treasure);
		else
			(*frontend_engine_ptr->p_strnfmt)(feeling, sizeof(feeling), "? / %s", treasure);
		string(p, "feeling", feeling);
	}
	if ((*frontend_engine_ptr->p_player)->upkeep->health_who) {
		const struct monster *m = (*frontend_engine_ptr->p_player)->upkeep->health_who;
		cJSON *tracked = cJSON_CreateObject();
		string(tracked, "name", m->race ? m->race->name : "");
		number(tracked, "hp", m->hp);
		number(tracked, "max_hp", m->maxhp);
		json_bool(tracked, "visible",
		          (*frontend_engine_ptr->p_monster_is_visible)(m) && !(*frontend_engine_ptr->p_player)->timed[TMD_IMAGE]);
		cJSON_AddItemToObject(p, "tracked_creature", tracked);
	}
}
