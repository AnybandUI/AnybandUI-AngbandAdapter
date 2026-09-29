/* Read-only, on-demand knowledge records. Engine recall owns descriptions. */

static bool anybandui_knowledge_visible(const char *category, int i)
{
	if (streq(category, "creatures"))
		return i > 0 && i < (*frontend_engine_ptr->p_z_info)->r_max && (*frontend_engine_ptr->p_r_info)[i].name &&
		       ((*frontend_engine_ptr->p_get_lore)(&(*frontend_engine_ptr->p_r_info)[i])->sights ||
		        (*frontend_engine_ptr->p_get_lore)(&(*frontend_engine_ptr->p_r_info)[i])->all_known);
	if (streq(category, "items"))
		return i >= 0 && i < (*frontend_engine_ptr->p_z_info)->k_max && (*frontend_engine_ptr->p_k_info)[i].name &&
		       !kf_has((*frontend_engine_ptr->p_k_info)[i].kind_flags, KF_INSTA_ART) &&
		       ((*frontend_engine_ptr->p_k_info)[i].everseen || (*frontend_engine_ptr->p_k_info)[i].flavor ||
		        OPT((*frontend_engine_ptr->p_player), cheat_xtra));
	if (streq(category, "artifacts"))
		return i >= 0 && i < (*frontend_engine_ptr->p_z_info)->a_max && (*frontend_engine_ptr->p_a_info)[i].name &&
		       ((*frontend_engine_ptr->p_is_artifact_seen)(&(*frontend_engine_ptr->p_a_info)[i]) || (*frontend_engine_ptr->p_player)->wizard ||
		        OPT((*frontend_engine_ptr->p_player), cheat_xtra));
	if (streq(category, "terrain"))
		return i >= 0 && i < FEAT_MAX && (*frontend_engine_ptr->p_f_info)[i].name && !(*frontend_engine_ptr->p_f_info)[i].mimic;
	return false;
}
static cJSON *anybandui_knowledge_entry(const char *category, int i)
{
	cJSON *row = cJSON_CreateObject();
	char name[256];
	const char *group = "";
	int attr = COLOUR_WHITE;
	number(row, "id", i);
	json_bool(row, "known",
	          !streq(category, "items") || !(*frontend_engine_ptr->p_k_info)[i].flavor ||
	                  (*frontend_engine_ptr->p_k_info)[i].aware);
	if (streq(category, "creatures")) {
		struct monster_race *race = &(*frontend_engine_ptr->p_r_info)[i];
		(*frontend_engine_ptr->p_my_strcpy)(name, race->name, sizeof(name));
		group = rf_has(race->flags, RF_UNIQUE) ? "Unique creatures"
		                                       : race->base->name;
		attr = race->d_attr;
	} else if (streq(category, "items")) {
		/* Name the complete type, not just its flavour. Copy the kind because
		 * object_desc marks aware kinds as everseen even for a recall template.
		 */
		struct object_kind kind = (*frontend_engine_ptr->p_k_info)[i];
		struct object obj = OBJECT_NULL, known = OBJECT_NULL;
		obj.kind = &kind;
		obj.tval = kind.tval;
		obj.sval = kind.sval;
		obj.number = 1;
		known = obj;
		obj.known = &known;
		(*frontend_engine_ptr->p_object_desc)(name, sizeof(name), &obj, ODESC_BASE | ODESC_SINGULAR,
		            (*frontend_engine_ptr->p_player));
		group = (*frontend_engine_ptr->p_tval_find_name)(kind.tval);
		attr = (*frontend_engine_ptr->p_object_kind_attr)(&kind);
		if (attr == COLOUR_DARK)
			attr = COLOUR_WHITE;
	} else if (streq(category, "artifacts")) {
		(*frontend_engine_ptr->p_my_strcpy)(name, (*frontend_engine_ptr->p_a_info)[i].name, sizeof(name));
		group = (*frontend_engine_ptr->p_tval_find_name)((*frontend_engine_ptr->p_a_info)[i].tval);
		attr = COLOUR_VIOLET;
	} else {
		struct feature *feat = &(*frontend_engine_ptr->p_f_info)[i];
		(*frontend_engine_ptr->p_my_strcpy)(name, feat->name, sizeof(name));
		attr = feat->d_attr;
		group = feat->shopnum                      ? "Shops"
		        : tf_has(feat->flags, TF_PASSABLE) ? "Passable terrain"
		                                           : "Obstacles";
	}
	(*frontend_engine_ptr->p_my_strcap)(name);
	string(row, "name", name);
	string(row, "group", group ? group : "Other");
	number(row, "color", attr);
	return row;
}
static void anybandui_knowledge_text(cJSON *out, const char *title,
                                     textblock *tb)
{
	const wchar_t *text = (*frontend_engine_ptr->p_textblock_text)(tb);
	inspection_section(out, title, title, text, wcslen(text));
	(*frontend_engine_ptr->p_textblock_free)(tb);
}
static void anybandui_knowledge_value(cJSON *out, const char *label, int value)
{
	cJSON *row = cJSON_CreateObject();
	string(row, "label", label);
	number(row, "value", value);
	cJSON_AddItemToArray(cJSON_GetObjectItem(out, "stats"), row);
}
static cJSON *anybandui_knowledge_detail(const char *category, int i)
{
	cJSON *out = anybandui_knowledge_entry(category, i);
	textblock *tb;
	uint32_t saved_state[RAND_DEG], saved_index = (*frontend_engine_ptr->p_state_i),
	                                saved_value = (*frontend_engine_ptr->p_Rand_value);
	bool saved_quick = (*frontend_engine_ptr->p_Rand_quick);
	memcpy(saved_state, (*frontend_engine_ptr->p_STATE), sizeof(saved_state));
	cJSON_AddItemToObject(out, "stats", cJSON_CreateArray());
	cJSON_AddItemToObject(out, "description_sections", cJSON_CreateArray());
	if (streq(category, "creatures")) {
		struct monster_race *race = &(*frontend_engine_ptr->p_r_info)[i];
		const struct monster_lore *lore = (*frontend_engine_ptr->p_get_lore)(race);
		bitflag flags[RF_SIZE];
		(*frontend_engine_ptr->p_monster_flags_known)(race, lore, flags);
		anybandui_knowledge_value(out, "Sightings", lore->sights);
		anybandui_knowledge_value(out, "Kills this life", lore->pkills);
		anybandui_knowledge_value(out, "Total kills", lore->tkills);
		tb = (*frontend_engine_ptr->p_textblock_new)();
		(*frontend_engine_ptr->p_lore_append_flavor)(tb, race);
		anybandui_knowledge_text(out, "Description", tb);
		tb = (*frontend_engine_ptr->p_textblock_new)();
		(*frontend_engine_ptr->p_lore_append_movement)(tb, race, lore, flags);
		(*frontend_engine_ptr->p_lore_append_awareness)(tb, race, lore, flags);
		anybandui_knowledge_text(out, "Movement & awareness", tb);
		tb = (*frontend_engine_ptr->p_textblock_new)();
		(*frontend_engine_ptr->p_lore_append_toughness)(tb, race, lore, flags);
		(*frontend_engine_ptr->p_lore_append_attack)(tb, race, lore, flags);
		anybandui_knowledge_text(out, "Combat", tb);
		tb = (*frontend_engine_ptr->p_textblock_new)();
		(*frontend_engine_ptr->p_lore_append_abilities)(tb, race, lore, flags);
		(*frontend_engine_ptr->p_lore_append_spells)(tb, race, lore, flags);
		anybandui_knowledge_text(out, "Abilities & spells", tb);
		tb = (*frontend_engine_ptr->p_textblock_new)();
		(*frontend_engine_ptr->p_lore_append_drop)(tb, race, lore, flags);
		(*frontend_engine_ptr->p_lore_append_exp)(tb, race, lore, flags);
		anybandui_knowledge_text(out, "Rewards", tb);
		tb = (*frontend_engine_ptr->p_textblock_new)();
		(*frontend_engine_ptr->p_lore_append_kills)(tb, race, lore, flags);
		(*frontend_engine_ptr->p_lore_append_friends)(tb, race, lore, flags);
		anybandui_knowledge_text(out, "Encounters", tb);
	} else if (streq(category, "terrain")) {
		struct feature *feat = &(*frontend_engine_ptr->p_f_info)[i];
		tb = (*frontend_engine_ptr->p_textblock_new)();
		(*frontend_engine_ptr->p_textblock_append)(tb, "%s",
		                 feat->desc ? feat->desc : "No further description.");
		anybandui_knowledge_text(out, "Description", tb);
		tb = (*frontend_engine_ptr->p_textblock_new)();
		(*frontend_engine_ptr->p_textblock_append)(tb, "%s",
		                 tf_has(feat->flags, TF_PASSABLE)
		                         ? "You can walk across this terrain."
		                         : "This terrain blocks ordinary movement.");
		if (feat->shopnum)
			(*frontend_engine_ptr->p_textblock_append)(tb, " This is a shop entrance.");
		anybandui_knowledge_text(out, "Traversal", tb);
	} else {
		struct object *obj = (*frontend_engine_ptr->p_object_new)(), *known = (*frontend_engine_ptr->p_object_new)();
		bool artifact = streq(category, "artifacts");
		if (artifact)
			(*frontend_engine_ptr->p_make_fake_artifact)(obj, &(*frontend_engine_ptr->p_a_info)[i]);
		else
			(*frontend_engine_ptr->p_object_prep)(obj, &(*frontend_engine_ptr->p_k_info)[i], 0, EXTREMIFY);
		if (artifact || (*frontend_engine_ptr->p_k_info)[i].aware || !(*frontend_engine_ptr->p_k_info)[i].flavor)
			(*frontend_engine_ptr->p_object_copy)(known, obj);
		obj->known = known;
		tb = anybandui_object_info_sections(obj, OINFO_FAKE, inspection_section, NULL,
		                          out);
		(*frontend_engine_ptr->p_textblock_free)(tb);
		(*frontend_engine_ptr->p_object_delete)(NULL, NULL, &known);
		(*frontend_engine_ptr->p_object_delete)(NULL, NULL, &obj);
	}
	memcpy((*frontend_engine_ptr->p_STATE), saved_state, sizeof(saved_state));
	(*frontend_engine_ptr->p_state_i) = saved_index;
	(*frontend_engine_ptr->p_Rand_value) = saved_value;
	(*frontend_engine_ptr->p_Rand_quick) = saved_quick;
	return out;
}
/* Observe only the inspected race. The cheap comparison includes pointed-to
 * attack knowledge, without regenerating recall on ordinary unchanged turns.
 * Lore's other linked lists are loaded definitions, not mutable recall inputs.
 */
static int anybandui_watched_race = -1, anybandui_watched_level,
           anybandui_watched_max_num;
static struct monster_lore anybandui_watched_lore;
static struct monster_blow *anybandui_watched_blows;
static bool *anybandui_watched_blow_known;
static void anybandui_knowledge_unwatch(void)
{
	anybandui_watched_race = -1;
	(*frontend_engine_ptr->p_mem_free)(anybandui_watched_blows);
	anybandui_watched_blows = NULL;
	(*frontend_engine_ptr->p_mem_free)(anybandui_watched_blow_known);
	anybandui_watched_blow_known = NULL;
}
static bool anybandui_knowledge_watch_changed(void)
{
	const struct monster_lore *lore = (*frontend_engine_ptr->p_get_lore)(&(*frontend_engine_ptr->p_r_info)[anybandui_watched_race]);
	return memcmp(&anybandui_watched_lore, lore, sizeof(*lore)) ||
	       memcmp(anybandui_watched_blows, lore->blows,
	              (*frontend_engine_ptr->p_z_info)->mon_blows_max * sizeof(*lore->blows)) ||
	       memcmp(anybandui_watched_blow_known, lore->blow_known,
	              (*frontend_engine_ptr->p_z_info)->mon_blows_max * sizeof(*lore->blow_known)) ||
	       anybandui_watched_level != (*frontend_engine_ptr->p_player)->lev ||
	       anybandui_watched_max_num != (*frontend_engine_ptr->p_r_info)[anybandui_watched_race].max_num;
}
static void anybandui_knowledge_remember(void)
{
	const struct monster_lore *lore = (*frontend_engine_ptr->p_get_lore)(&(*frontend_engine_ptr->p_r_info)[anybandui_watched_race]);
	memcpy(&anybandui_watched_lore, lore, sizeof(*lore));
	memcpy(anybandui_watched_blows, lore->blows,
	       (*frontend_engine_ptr->p_z_info)->mon_blows_max * sizeof(*lore->blows));
	memcpy(anybandui_watched_blow_known, lore->blow_known,
	       (*frontend_engine_ptr->p_z_info)->mon_blows_max * sizeof(*lore->blow_known));
	anybandui_watched_level = (*frontend_engine_ptr->p_player)->lev;
	anybandui_watched_max_num = (*frontend_engine_ptr->p_r_info)[anybandui_watched_race].max_num;
}
static void anybandui_knowledge_watch(int race)
{
	anybandui_knowledge_unwatch();
	anybandui_watched_race = race;
	anybandui_watched_blows =
	        (*frontend_engine_ptr->p_mem_alloc)((*frontend_engine_ptr->p_z_info)->mon_blows_max * sizeof(*anybandui_watched_blows));
	anybandui_watched_blow_known = (*frontend_engine_ptr->p_mem_alloc)(
	        (*frontend_engine_ptr->p_z_info)->mon_blows_max * sizeof(*anybandui_watched_blow_known));
	anybandui_knowledge_remember();
}
static void anybandui_knowledge_publish(void)
{
	cJSON *detail;
	if (anybandui_watched_race < 0 || !(*frontend_engine_ptr->p_character_generated) ||
	    !streq(phase, "playing"))
		return;
	if (!anybandui_knowledge_watch_changed())
		return;
	detail = anybandui_knowledge_detail("creatures", anybandui_watched_race);
	string(detail, "category", "creatures");
	anybandui_knowledge_remember();
	event("knowledge.changed", detail);
}
static void anybandui_knowledge_request(const char *id, const char *method,
                                        const cJSON *p)
{
	const char *category = str(p, "category");
	int i, max = 0;
	cJSON *out, *entries;
	if (!(*frontend_engine_ptr->p_character_generated) || !streq(phase, "playing")) {
		error(id, "wrong_phase", "Knowledge is available during play.");
		return;
	}
	if (streq(category, "creatures"))
		max = (*frontend_engine_ptr->p_z_info)->r_max;
	else if (streq(category, "items"))
		max = (*frontend_engine_ptr->p_z_info)->k_max;
	else if (streq(category, "artifacts"))
		max = (*frontend_engine_ptr->p_z_info)->a_max;
	else if (streq(category, "terrain"))
		max = FEAT_MAX;
	else {
		error(id, "invalid_argument", "Unknown knowledge category.");
		return;
	}
	if (streq(method, "knowledge.get")) {
		const cJSON *entry = cJSON_GetObjectItem(p, "id");
		i = num(p, "id", -1);
		if (!cJSON_IsNumber(entry) || entry->valuedouble != i ||
		    !anybandui_knowledge_visible(category, i)) {
			error(id, "invalid_argument", "Knowledge entry is unavailable.");
			return;
		}
		out = anybandui_knowledge_detail(category, i);
		if (streq(category, "creatures") &&
		    cJSON_IsTrue(cJSON_GetObjectItem(p, "watch")))
			anybandui_knowledge_watch(i);
	} else {
		out = cJSON_CreateObject();
		entries = cJSON_CreateArray();
		for (i = 0; i < max; ++i)
			if (anybandui_knowledge_visible(category, i))
				cJSON_AddItemToArray(entries,
				                     anybandui_knowledge_entry(category, i));
		cJSON_AddItemToObject(out, "entries", entries);
	}
	string(out, "category", category);
	response(id, out);
}
