/* Read-only character sheet data; no terminal scraping or client rules. */
static void anybandui_sheet_row(void *user, const char *group,
                                const char *label, const char *value, int attr)
{
	cJSON *rows = user, *row = cJSON_CreateObject();
	string(row, "group", group);
	string(row, "label", label);
	string(row, "value", value);
	number(row, "color", attr);
	cJSON_AddItemToArray(rows, row);
}
static void anybandui_sheet_stat(cJSON *row, const char *key, int value)
{
	char text[32];
	if (value > 18)
		(*frontend_engine_ptr->p_strnfmt)(text, sizeof(text), "18/%02d", value - 18);
	else
		(*frontend_engine_ptr->p_strnfmt)(text, sizeof(text), "%d", value);
	string(row, key, text);
}
static void anybandui_character_sheet(cJSON *p)
{
	static const char *stats[] = {"STR", "INT", "WIS", "DEX", "CON"};
	static const char *elements[] = {
	        "Acid",   "Lightning", "Fire",          "Cold",   "Poison",
	        "Light",  "Dark",      "Sound",         "Shards", "Nexus",
	        "Nether", "Chaos",     "Disenchantment"};
	cJSON *sheet = cJSON_CreateObject(), *rows = cJSON_CreateArray(),
	      *attributes = cJSON_CreateArray();
	cJSON *resists = cJSON_CreateArray(), *abilities = cJSON_CreateArray();
	struct player_ability *ability;
	int i;
	textblock *document = (*frontend_engine_ptr->p_character_sheet)();
	size_t count, row_index;
	const struct textblock_span *fields = (*frontend_engine_ptr->p_textblock_spans)(document, &count);
	for (row_index = 0; row_index < count; ++row_index) {
		const struct textblock_span *field = &fields[row_index];
		if (field->label)
			anybandui_sheet_row(rows, field->section, field->label,
				field->value, field->attr);
	}
	(*frontend_engine_ptr->p_textblock_free)(document);
	string(sheet, "history", (*frontend_engine_ptr->p_player)->history ? (*frontend_engine_ptr->p_player)->history : "");
	for (i = 0; i < STAT_MAX; ++i) {
		cJSON *row = cJSON_CreateObject();
		string(row, "label", stats[i]);
		anybandui_sheet_stat(row, "base", (*frontend_engine_ptr->p_player)->stat_max[i]);
		anybandui_sheet_stat(row, "current", (*frontend_engine_ptr->p_player)->state.stat_use[i]);
		anybandui_sheet_stat(row, "best", (*frontend_engine_ptr->p_player)->state.stat_top[i]);
		number(row, "race", (*frontend_engine_ptr->p_player)->race->r_adj[i]);
		number(row, "class", (*frontend_engine_ptr->p_player)->class->c_adj[i]);
		number(row, "equipment", (*frontend_engine_ptr->p_player)->state.stat_add[i]);
		json_bool(row, "sustained",
		          of_has((*frontend_engine_ptr->p_player)->known_state.flags, (*frontend_engine_ptr->p_sustain_flag)(i)));
		cJSON_AddItemToArray(attributes, row);
	}
	for (i = 0; i < (int)N_ELEMENTS(elements); ++i) {
		cJSON *row = cJSON_CreateObject();
		int level = (*frontend_engine_ptr->p_player)->known_state.el_info[i].res_level;
		string(row, "label", elements[i]);
		number(row, "level", level);
		string(row, "value",
		       level < 0    ? "Vulnerable"
		       : level == 0 ? "Unprotected"
		       : level >= 3 ? "Immune"
		       : level == 2 ? "Double resistance"
		                    : "Resistant");
		cJSON_AddItemToArray(resists, row);
	}
	for (i = 1; i < OF_MAX; ++i) {
		struct obj_property *prop = (*frontend_engine_ptr->p_lookup_obj_property)(OBJ_PROPERTY_FLAG, i);
		if (prop && of_has((*frontend_engine_ptr->p_player)->known_state.flags, i)) {
			cJSON *row = cJSON_CreateObject();
			string(row, "label", prop->name);
			string(row, "description", prop->desc ? prop->desc : "");
			cJSON_AddItemToArray(abilities, row);
		}
	}
	for (ability = (*frontend_engine_ptr->p_player_abilities); ability; ability = ability->next) {
		if (streq(ability->type, "player") &&
		    ((*frontend_engine_ptr->p_class_has_ability)((*frontend_engine_ptr->p_player)->class, ability) ||
		     (*frontend_engine_ptr->p_race_has_ability)((*frontend_engine_ptr->p_player)->race, ability))) {
			cJSON *row = cJSON_CreateObject();
			string(row, "label", ability->name);
			string(row, "description", ability->desc ? ability->desc : "");
			cJSON_AddItemToArray(abilities, row);
		}
	}
	cJSON_AddItemToObject(sheet, "rows", rows);
	cJSON_AddItemToObject(sheet, "attributes", attributes);
	cJSON_AddItemToObject(sheet, "resistances", resists);
	cJSON_AddItemToObject(sheet, "abilities", abilities);
	cJSON_AddItemToObject(p, "character_sheet", sheet);
}
