/* Borrowed engine data lives only for the synchronous target-input call. */
struct target_ui_state {
	struct loc grid;
	int mode;
	bool interesting, can_confirm;
	const struct point_set *candidates;
	const struct loc *path;
	int path_length;
};
static const struct target_ui_state *target_ui_current;
static ui_event anybandui_target_input(int mode, struct loc grid,
	bool interesting, const struct point_set *candidates)
{
	struct loc path[256];
	struct target_ui_state view = { grid, mode, interesting,
		interesting ? (*frontend_engine_ptr->p_target_able)((*frontend_engine_ptr->p_square_monster)((*frontend_engine_ptr->p_cave), grid)) :
			(*frontend_engine_ptr->p_square_in_bounds_fully)((*frontend_engine_ptr->p_cave), grid), candidates, path, 0 };
	const struct target_ui_state *previous = target_ui_current;
	ui_event result;
	view.path_length = (*frontend_engine_ptr->p_project_path)((*frontend_engine_ptr->p_cave), path, (*frontend_engine_ptr->p_z_info)->max_range,
		(*frontend_engine_ptr->p_player)->grid, grid, PROJECT_THRU | PROJECT_INFO);
	target_ui_current = &view;
	/* Retain the engine's tracking semantics without entering its prompt. */
	{
		struct monster *mon = (*frontend_engine_ptr->p_square_monster)((*frontend_engine_ptr->p_cave), grid);
		if (!(*frontend_engine_ptr->p_player)->timed[TMD_IMAGE] && mon && (*frontend_engine_ptr->p_monster_is_obvious)(mon)) {
			(*frontend_engine_ptr->p_monster_race_track)((*frontend_engine_ptr->p_player)->upkeep, mon->race);
			(*frontend_engine_ptr->p_health_track)((*frontend_engine_ptr->p_player)->upkeep, mon);
			(*frontend_engine_ptr->p_handle_stuff)((*frontend_engine_ptr->p_player));
		}
	}
	result = anybandui_interaction_input();
	/* Detailed recall and pile browsing keep their complete stock behavior. */
	if (result.type == EVT_KBRD && (result.key.code == 'r' ||
			result.key.code == KC_ENTER || result.key.code == ' ' ||
			result.key.code == (*frontend_engine_ptr->p_cmd_lookup_key)(CMD_IGNORE,
				OPT((*frontend_engine_ptr->p_player), rogue_like_commands) ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG))) {
		(*frontend_engine_ptr->p_Term_keypress)(result.key.code, result.key.mods);
		result = (*frontend_engine_ptr->p_target_get_input)(grid.y, grid.x,
			mode | (interesting ? 0 : TARGET_LOOK));
	}
	target_ui_current = previous;
	return result;
}

/* Adapter-owned presentation. Gameplay calculations remain in Angband. */

typedef void (*object_info_section_cb)(void *, const char *, const char *,
                                       const wchar_t *, size_t);
typedef void (*object_info_combat_cb)(void *, const char *, const char *, int,
                                      int, int);

struct anybandui_description {
	object_info_section_cb emit;
	void *user;
	size_t start;
};

static void anybandui_description_section(textblock *tb, const struct textblock_span *span,
                                          void *user)
{
	struct anybandui_description *context = user;
	static const char *ids[] = {"knowledge", "lore",        "curses",
	                            "bonuses",   "resistances", "durability",
	                            "abilities", "use",         "combat",
	                            "digging",   "notes"};
	static const char *titles[] = {"Identification",
	                               "Origin & lore",
	                               "Curses",
	                               "Bonuses & damage",
	                               "Resistances & protection",
	                               "Durability",
	                               "Abilities",
	                               "Use & activation",
	                               "Combat",
	                               "Digging",
	                               "Notes"};
	const wchar_t *text = (*frontend_engine_ptr->p_textblock_text)(tb);
	size_t end = span->start + span->length, i;
	const char *id = span->section;
	context->start = span->start;
	for (i = 0; i < N_ELEMENTS(ids); ++i) {
		if (streq(ids[i], id))
			break;
	}
	assert(i < N_ELEMENTS(ids));
	if (context->emit && end > context->start)
		context->emit(context->user, id, titles[i], text + context->start,
		              end - context->start);
	context->start = end;
}

static void anybandui_description_damage(const struct object *obj,
                                         bool throwing,
                                         object_info_combat_cb emit, void *user)
{
	int i, normal;
	int *brand = (*frontend_engine_ptr->p_mem_zalloc)((*frontend_engine_ptr->p_z_info)->brand_max * sizeof(*brand));
	int *slay = (*frontend_engine_ptr->p_mem_zalloc)((*frontend_engine_ptr->p_z_info)->slay_max * sizeof(*slay));
	bool nonweapon = false;
	if (OPT((*frontend_engine_ptr->p_player), birth_percent_damage))
		(*frontend_engine_ptr->p_o_obj_known_damage)(obj, &normal, brand, slay, &nonweapon, throwing);
	else
		(*frontend_engine_ptr->p_obj_known_damage)(obj, &normal, brand, slay, &nonweapon, throwing);
	emit(user, throwing ? "throw_damage" : "damage", "Normal", normal, 0, 0);
	for (i = 0; i < (*frontend_engine_ptr->p_z_info)->brand_max; ++i) {
		if (brand[i] > 0) {
			char label[160];
			(*frontend_engine_ptr->p_strnfmt)(label, sizeof(label), "Not resistant to %s",
			        (*frontend_engine_ptr->p_brands)[i].name);
			emit(user, throwing ? "throw_variant" : "damage_variant", label,
			     brand[i], 0, 0);
		}
	}
	for (i = 0; i < (*frontend_engine_ptr->p_z_info)->slay_max; ++i)
		if (slay[i] > 0)
			emit(user, throwing ? "throw_variant" : "damage_variant",
			     (*frontend_engine_ptr->p_slays)[i].name, slay[i], 0, 0);
	if (nonweapon)
		emit(user, "note",
		     "This weapon may benefit from off-weapon brands or slays.", 0, 0,
		     0);
	(*frontend_engine_ptr->p_mem_free)(brand);
	(*frontend_engine_ptr->p_mem_free)(slay);
}

static void anybandui_description_combat(const struct object *obj,
                                         object_info_combat_cb emit, void *user)
{
	struct object *bow = (*frontend_engine_ptr->p_equipped_item_by_slot_name)((*frontend_engine_ptr->p_player), "shooting");
	bool weapon = (*frontend_engine_ptr->p_tval_is_melee_weapon)(obj);
	bool ammo = (*frontend_engine_ptr->p_player)->state.ammo_tval == obj->tval && bow;
	bool rock = (*frontend_engine_ptr->p_tval_is_ammo)(obj) && of_has(obj->flags, OF_THROWING);
	int range, break_chance, count, i;
	bool thrown_effect, heavy;
	struct blow_info blows[STAT_RANGE * 2];
	(*frontend_engine_ptr->p_obj_known_misc_combat)(obj, &thrown_effect, &range, &break_chance, &heavy);
	if (!weapon && !ammo && !rock) {
		if (thrown_effect)
			emit(user, "note",
			     "Can be thrown at creatures with damaging effect.", 0, 0, 0);
		return;
	}
	if (heavy)
		emit(user, "warning", "You are too weak to use this weapon.", 0, 0, 0);
	if (ammo) {
		emit(user, "range", "Range", range, 0, 0);
		emit(user, "break", "Break chance", break_chance, 0, 0);
	}
	count = (*frontend_engine_ptr->p_obj_known_blows)(obj, N_ELEMENTS(blows), blows);
	if (count)
		emit(user, "blows", "Blows / round", blows[0].centiblows, 0, 0);
	for (i = 1; i < count; ++i)
		emit(user, "upgrade",
		     blows[i].centiblows % 10 ? "Slightly faster" : "Blows / round",
		     blows[i].centiblows, blows[i].str_plus, blows[i].dex_plus);
	if (weapon || ammo)
		anybandui_description_damage(obj, false, emit, user);
	if ((weapon && of_has(obj->flags, OF_THROWING)) || rock)
		anybandui_description_damage(obj, true, emit, user);
}

static textblock *anybandui_object_info_sections(const struct object *obj,
                                                 oinfo_detail_t mode,
                                                 object_info_section_cb emit,
                                                 object_info_combat_cb combat,
                                                 void *user)
{
	struct anybandui_description context = {emit, user, 0};
	textblock *tb = (*frontend_engine_ptr->p_object_info)(obj, mode);
	size_t count, i;
	const struct textblock_span *spans = (*frontend_engine_ptr->p_textblock_spans)(tb, &count);
	for (i = 0; i < count; ++i)
		anybandui_description_section(tb, &spans[i], &context);
	if (combat && !(mode & OINFO_EGO) && obj->kind == obj->known->kind)
		anybandui_description_combat(obj, combat, user);
	return tb;
}

void format_level_feeling(char *buf, size_t size)
{
	uint16_t obj_feeling =
	        MIN((*frontend_engine_ptr->p_cave)->feeling / 10, N_ELEMENTS((*frontend_engine_ptr->p_obj_feeling_text)) - 1);
	uint16_t mon_feeling =
	        MIN((*frontend_engine_ptr->p_cave)->feeling % 10, N_ELEMENTS((*frontend_engine_ptr->p_mon_feeling_text)) - 1);
	const char *join;

	/* Don't show feelings for cold-hearted characters */
	if (!OPT((*frontend_engine_ptr->p_player), birth_feelings)) {
		(*frontend_engine_ptr->p_my_strcpy)(buf, "Level feelings are disabled.", size);
		return;
	}

	/* No useful feeling in town */
	if (!(*frontend_engine_ptr->p_player)->depth) {
		(*frontend_engine_ptr->p_my_strcpy)(buf, "Looks like a typical town.", size);
		return;
	}

	/* Players automatically get a monster feeling. */
	if ((*frontend_engine_ptr->p_cave)->feeling_squares < (*frontend_engine_ptr->p_z_info)->feeling_need) {
		(*frontend_engine_ptr->p_strnfmt)(buf, size, "%s.", (*frontend_engine_ptr->p_mon_feeling_text)[mon_feeling]);
		return;
	}

	/* Decide the conjunction */
	if ((mon_feeling <= 5 && obj_feeling > 6) ||
	    (mon_feeling > 5 && obj_feeling <= 6))
		join = ", yet";
	else
		join = ", and";

	/* Display the feeling */
	(*frontend_engine_ptr->p_strnfmt)(buf, size, "%s%s %s", (*frontend_engine_ptr->p_mon_feeling_text)[mon_feeling], join,
	        (*frontend_engine_ptr->p_obj_feeling_text)[obj_feeling]);
}

#include "anybandui-map.h"

static bool anybandui_store_check(const char *prompt, int32_t price)
{
	char confirmation[1024];
	(*frontend_engine_ptr->p_strnfmt)(confirmation, sizeof(confirmation), "%s\nPrice: %ld gold", prompt,
	        (long)price);
	return (*frontend_engine_ptr->p_get_check)(confirmation);
}

/* Queue world coordinates through the engine's ordinary mouse path. */
static void anybandui_grid_press(struct loc grid)
{
	int head = (*frontend_engine_ptr->p_Term)->key_head;
	(*frontend_engine_ptr->p_Term_mousepress)(grid.x, grid.y, 1);
	(*frontend_engine_ptr->p_Term)->key_queue[head].mouse.mods |= MOUSE_MOD_GRID;
}

static bool target_ui_select(struct loc grid, bool confirm)
{
	if (!target_ui_current || !(*frontend_engine_ptr->p_square_in_bounds_fully)((*frontend_engine_ptr->p_cave), grid))
		return false;
	if (confirm && !(target_ui_current->mode & TARGET_LOOK)) {
		struct monster *mon = (*frontend_engine_ptr->p_square_monster)((*frontend_engine_ptr->p_cave), grid);
		if ((*frontend_engine_ptr->p_target_able)(mon))
			(*frontend_engine_ptr->p_target_set_monster)(mon);
		else
			(*frontend_engine_ptr->p_target_set_location)(grid.y, grid.x);
		/* The original target loop retains its normal cleanup and return. */
		(*frontend_engine_ptr->p_Term_keypress)(ESCAPE, 0);
	} else {
		anybandui_grid_press(grid);
	}
	return true;
}
