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
		interesting ? target_able(square_monster(cave, grid)) :
			square_in_bounds_fully(cave, grid), candidates, path, 0 };
	const struct target_ui_state *previous = target_ui_current;
	ui_event result;
	view.path_length = project_path(cave, path, z_info->max_range,
		player->grid, grid, PROJECT_THRU | PROJECT_INFO);
	target_ui_current = &view;
	/* Retain the engine's tracking semantics without entering its prompt. */
	{
		struct monster *mon = square_monster(cave, grid);
		if (!player->timed[TMD_IMAGE] && mon && monster_is_obvious(mon)) {
			monster_race_track(player->upkeep, mon->race);
			health_track(player->upkeep, mon);
			handle_stuff(player);
		}
	}
	result = anybandui_interaction_input();
	/* Detailed recall and pile browsing keep their complete stock behavior. */
	if (result.type == EVT_KBRD && (result.key.code == 'r' ||
			result.key.code == KC_ENTER || result.key.code == ' ' ||
			result.key.code == cmd_lookup_key(CMD_IGNORE,
				OPT(player, rogue_like_commands) ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG))) {
		Term_keypress(result.key.code, result.key.mods);
		result = target_get_input(grid.y, grid.x,
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
	const wchar_t *text = textblock_text(tb);
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
	int *brand = mem_zalloc(z_info->brand_max * sizeof(*brand));
	int *slay = mem_zalloc(z_info->slay_max * sizeof(*slay));
	bool nonweapon = false;
	if (OPT(player, birth_percent_damage))
		o_obj_known_damage(obj, &normal, brand, slay, &nonweapon, throwing);
	else
		obj_known_damage(obj, &normal, brand, slay, &nonweapon, throwing);
	emit(user, throwing ? "throw_damage" : "damage", "Normal", normal, 0, 0);
	for (i = 0; i < z_info->brand_max; ++i) {
		if (brand[i] > 0) {
			char label[160];
			strnfmt(label, sizeof(label), "Not resistant to %s",
			        brands[i].name);
			emit(user, throwing ? "throw_variant" : "damage_variant", label,
			     brand[i], 0, 0);
		}
	}
	for (i = 0; i < z_info->slay_max; ++i)
		if (slay[i] > 0)
			emit(user, throwing ? "throw_variant" : "damage_variant",
			     slays[i].name, slay[i], 0, 0);
	if (nonweapon)
		emit(user, "note",
		     "This weapon may benefit from off-weapon brands or slays.", 0, 0,
		     0);
	mem_free(brand);
	mem_free(slay);
}

static void anybandui_description_combat(const struct object *obj,
                                         object_info_combat_cb emit, void *user)
{
	struct object *bow = equipped_item_by_slot_name(player, "shooting");
	bool weapon = tval_is_melee_weapon(obj);
	bool ammo = player->state.ammo_tval == obj->tval && bow;
	bool rock = tval_is_ammo(obj) && of_has(obj->flags, OF_THROWING);
	int range, break_chance, count, i;
	bool thrown_effect, heavy;
	struct blow_info blows[STAT_RANGE * 2];
	obj_known_misc_combat(obj, &thrown_effect, &range, &break_chance, &heavy);
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
	count = obj_known_blows(obj, N_ELEMENTS(blows), blows);
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
	textblock *tb = object_info(obj, mode);
	size_t count, i;
	const struct textblock_span *spans = textblock_spans(tb, &count);
	for (i = 0; i < count; ++i)
		anybandui_description_section(tb, &spans[i], &context);
	if (combat && !(mode & OINFO_EGO) && obj->kind == obj->known->kind)
		anybandui_description_combat(obj, combat, user);
	return tb;
}

void format_level_feeling(char *buf, size_t size)
{
	uint16_t obj_feeling =
	        MIN(cave->feeling / 10, N_ELEMENTS(obj_feeling_text) - 1);
	uint16_t mon_feeling =
	        MIN(cave->feeling % 10, N_ELEMENTS(mon_feeling_text) - 1);
	const char *join;

	/* Don't show feelings for cold-hearted characters */
	if (!OPT(player, birth_feelings)) {
		my_strcpy(buf, "Level feelings are disabled.", size);
		return;
	}

	/* No useful feeling in town */
	if (!player->depth) {
		my_strcpy(buf, "Looks like a typical town.", size);
		return;
	}

	/* Players automatically get a monster feeling. */
	if (cave->feeling_squares < z_info->feeling_need) {
		strnfmt(buf, size, "%s.", mon_feeling_text[mon_feeling]);
		return;
	}

	/* Decide the conjunction */
	if ((mon_feeling <= 5 && obj_feeling > 6) ||
	    (mon_feeling > 5 && obj_feeling <= 6))
		join = ", yet";
	else
		join = ", and";

	/* Display the feeling */
	strnfmt(buf, size, "%s%s %s", mon_feeling_text[mon_feeling], join,
	        obj_feeling_text[obj_feeling]);
}

#include "anybandui-map.h"

static bool anybandui_store_check(const char *prompt, int32_t price)
{
	char confirmation[1024];
	strnfmt(confirmation, sizeof(confirmation), "%s\nPrice: %ld gold", prompt,
	        (long)price);
	return get_check(confirmation);
}

/* Queue world coordinates through the engine's ordinary mouse path. */
static void anybandui_grid_press(struct loc grid)
{
	int head = Term->key_head;
	Term_mousepress(grid.x, grid.y, 1);
	Term->key_queue[head].mouse.mods |= MOUSE_MOD_GRID;
}

static bool target_ui_select(struct loc grid, bool confirm)
{
	if (!target_ui_current || !square_in_bounds_fully(cave, grid))
		return false;
	if (confirm && !(target_ui_current->mode & TARGET_LOOK)) {
		struct monster *mon = square_monster(cave, grid);
		if (target_able(mon))
			target_set_monster(mon);
		else
			target_set_location(grid.y, grid.x);
		/* The original target loop retains its normal cleanup and return. */
		Term_keypress(ESCAPE, 0);
	} else {
		anybandui_grid_press(grid);
	}
	return true;
}

