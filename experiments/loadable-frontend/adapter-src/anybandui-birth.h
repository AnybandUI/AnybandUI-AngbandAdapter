/* Semantic birth adapter. All mutations use the engine's birth commands. */
static bool birth_active, birth_waiting, birth_rolled, birth_previous,
        birth_quickstart;
static int birth_spent[STAT_MAX], birth_cost[STAT_MAX], birth_left;
static void birth_event(game_event_type type, game_event_data *data, void *user)
{
	(void)user;
	if (type == EVENT_ENTER_BIRTH)
		birth_quickstart = data->flag;
	else {
		memcpy(birth_spent, data->birthpoints.points, sizeof(birth_spent));
		memcpy(birth_cost, data->birthpoints.inc_points, sizeof(birth_cost));
		birth_left = data->birthpoints.remaining;
	}
}
static void birth_queue(cmd_code code, int choice)
{
	(*frontend_engine_ptr->p_cmdq_push)(code);
	(*frontend_engine_ptr->p_cmd_set_arg_choice)((*frontend_engine_ptr->p_cmdq_peek)(), "choice", choice);
}
static cJSON *birth_abilities(const struct player_race *race,
                              const struct player_class *cl)
{
	cJSON *out = cJSON_CreateArray();
	struct player_ability *a;
	for (a = (*frontend_engine_ptr->p_player_abilities); a; a = a->next) {
		if ((race && (*frontend_engine_ptr->p_race_has_ability)(race, a)) ||
		    (cl && (*frontend_engine_ptr->p_class_has_ability)(cl, a))) {
			cJSON *r = cJSON_CreateObject();
			string(r, "name", a->name);
			string(r, "description", a->desc ? a->desc : "");
			cJSON_AddItemToArray(out, r);
		}
	}
	return out;
}
static cJSON *birth_skills(const int *skills)
{
	static const int ids[] = {SKILL_TO_HIT_MELEE, SKILL_TO_HIT_BOW,
	                          SKILL_TO_HIT_THROW, SKILL_DISARM_PHYS,
	                          SKILL_DISARM_MAGIC, SKILL_DEVICE,
	                          SKILL_SAVE,         SKILL_STEALTH,
	                          SKILL_DIGGING,      SKILL_SEARCH};
	static const char *names[] = {"Melee",        "Shooting",     "Throwing",
	                              "Disarm traps", "Disarm magic", "Devices",
	                              "Saving throw", "Stealth",      "Digging",
	                              "Searching"};
	cJSON *out = cJSON_CreateArray();
	int i;
	for (i = 0; i < (int)N_ELEMENTS(ids); ++i) {
		cJSON *v = cJSON_CreateObject();
		string(v, "name", names[i]);
		number(v, "value", skills[ids[i]]);
		cJSON_AddItemToArray(out, v);
	}
	return out;
}
static cJSON *anybandui_birth_record(void)
{
	static const char *names[] = {"STR", "INT", "WIS", "DEX", "CON"};
	cJSON *b = cJSON_CreateObject(), *rs = cJSON_CreateArray(),
	      *cs = cJSON_CreateArray(), *stats = cJSON_CreateArray(),
	      *opts = cJSON_CreateArray();
	struct player_race *r;
	struct player_class *c;
	int i;
	for (r = (*frontend_engine_ptr->p_races); r; r = r->next) {
		cJSON *v = cJSON_CreateObject();
		number(v, "id", r->ridx);
		string(v, "name", r->name);
		cJSON_AddItemToObject(v, "skills", birth_skills(r->r_skills));
		number(v, "hit_die", r->r_mhp);
		number(v, "experience", r->r_exp);
		number(v, "infravision", r->infra * 10);
		cJSON_AddItemToObject(v, "modifiers", ints(r->r_adj, STAT_MAX));
		cJSON_AddItemToObject(v, "abilities", birth_abilities(r, NULL));
		cJSON_AddItemToArray(rs, v);
	}
	for (c = (*frontend_engine_ptr->p_classes); c; c = c->next) {
		cJSON *v = cJSON_CreateObject();
		number(v, "id", c->cidx);
		string(v, "name", c->name);
		cJSON_AddItemToObject(v, "skills", birth_skills(c->c_skills));
		number(v, "hit_die", c->c_mhp);
		number(v, "experience", c->c_exp);
		json_bool(v, "spellcasting", c->magic.total_spells > 0);
		cJSON_AddItemToObject(v, "modifiers", ints(c->c_adj, STAT_MAX));
		cJSON_AddItemToObject(v, "abilities", birth_abilities(NULL, c));
		cJSON_AddItemToArray(cs, v);
	}
	for (i = 0; i < STAT_MAX; ++i) {
		cJSON *v = cJSON_CreateObject();
		number(v, "id", i);
		string(v, "name", names[i]);
		number(v, "base", (*frontend_engine_ptr->p_player)->stat_birth[i]);
		number(v, "race", (*frontend_engine_ptr->p_player)->race->r_adj[i]);
		number(v, "class", (*frontend_engine_ptr->p_player)->class->c_adj[i]);
		anybandui_sheet_stat(v, "total", (*frontend_engine_ptr->p_player)->state.stat_use[i]);
		number(v, "spent", birth_spent[i]);
		number(v, "cost", birth_cost[i]);
		json_bool(v, "can_buy",
		          !birth_rolled && (*frontend_engine_ptr->p_player)->stat_birth[i] < 18 &&
		                  birth_cost[i] <= birth_left);
		json_bool(v, "can_sell", !birth_rolled && (*frontend_engine_ptr->p_player)->stat_birth[i] > 10);
		cJSON_AddItemToArray(stats, v);
	}
	for (i = 0; i < OPT_MAX; ++i)
		if ((*frontend_engine_ptr->p_option_type)(i) == OP_BIRTH) {
			cJSON *v = cJSON_CreateObject();
			string(v, "id", (*frontend_engine_ptr->p_option_name)(i));
			string(v, "description", (*frontend_engine_ptr->p_option_desc)(i));
			json_bool(v, "value", (*frontend_engine_ptr->p_player)->opts.opt[i]);
			cJSON_AddItemToArray(opts, v);
		}
	cJSON_AddItemToObject(b, "races", rs);
	cJSON_AddItemToObject(b, "classes", cs);
	cJSON_AddItemToObject(b, "stats", stats);
	cJSON_AddItemToObject(b, "options", opts);
	number(b, "race", (*frontend_engine_ptr->p_player)->race->ridx);
	number(b, "class", (*frontend_engine_ptr->p_player)->class->cidx);
	number(b, "points_left", birth_left);
	number(b, "hp", (*frontend_engine_ptr->p_player)->mhp);
	number(b, "sp", (*frontend_engine_ptr->p_player)->msp);
	number(b, "gold", (*frontend_engine_ptr->p_z_info)->start_gold);
	number(b, "name_max", sizeof((*frontend_engine_ptr->p_player)->full_name) - 1);
	string(b, "name", (*frontend_engine_ptr->p_player)->full_name);
	string(b, "history", (*frontend_engine_ptr->p_player)->history ? (*frontend_engine_ptr->p_player)->history : "");
	json_bool(b, "rolled", birth_rolled);
	json_bool(b, "previous_roll", birth_previous);
	json_bool(b, "quickstart", birth_quickstart);
	return b;
}
static void anybandui_birth_action(const char *id, const cJSON *p)
{
	const char *op = str(p, "action");
	const cJSON *choice = cJSON_GetObjectItem(p, "choice");
	int n = 0;
	if (!birth_active || !birth_waiting) {
		error(id, "wrong_phase",
		      "Character creation is not awaiting a choice.");
		return;
	}
	if (!streq(str(p, "revision"), revision_text)) {
		error(id, "stale_revision", "Character changed; choose again.");
		return;
	}
	if (streq(op, "race") || streq(op, "class") || streq(op, "buy") ||
	    streq(op, "sell")) {
		if (!cJSON_IsNumber(choice) ||
		    choice->valuedouble != choice->valueint || choice->valueint < 0)
			goto invalid;
		n = choice->valueint;
	}
	if (streq(op, "race")) {
		struct player_race *r;
		for (r = (*frontend_engine_ptr->p_races); r && r->ridx != (unsigned)n; r = r->next) {
		}
		if (!r)
			goto invalid;
		birth_queue(CMD_CHOOSE_RACE, n);
		birth_rolled = birth_previous = false;
	} else if (streq(op, "class")) {
		struct player_class *c;
		for (c = (*frontend_engine_ptr->p_classes); c && c->cidx != (unsigned)n; c = c->next) {
		}
		if (!c)
			goto invalid;
		birth_queue(CMD_CHOOSE_CLASS, n);
		birth_rolled = birth_previous = false;
	} else if (streq(op, "buy") || streq(op, "sell")) {
		if (birth_rolled || n >= STAT_MAX)
			goto invalid;
		birth_queue(streq(op, "buy") ? CMD_BUY_STAT : CMD_SELL_STAT, n);
	} else if (streq(op, "quickstart")) {
		if (!birth_quickstart)
			goto invalid;
		(*frontend_engine_ptr->p_cmdq_push)(CMD_BIRTH_RESET);
		birth_rolled = true;
		birth_previous = false;
	} else if (streq(op, "reset") || streq(op, "suggest")) {
		birth_queue(CMD_RESET_STATS, streq(op, "suggest"));
		birth_rolled = birth_previous = false;
	} else if (streq(op, "roll")) {
		birth_previous = birth_rolled;
		birth_rolled = true;
		(*frontend_engine_ptr->p_cmdq_push)(CMD_ROLL_STATS);
	} else if (streq(op, "previous")) {
		if (!birth_rolled || !birth_previous)
			goto invalid;
		(*frontend_engine_ptr->p_cmdq_push)(CMD_PREV_STATS);
	} else if (streq(op, "option")) {
		const char *name = str(p, "option");
		int i;
		if (!cJSON_IsBool(cJSON_GetObjectItem(p, "value")))
			goto invalid;
		for (i = 0; i < OPT_MAX; ++i)
			if ((*frontend_engine_ptr->p_option_type)(i) == OP_BIRTH && streq((*frontend_engine_ptr->p_option_name)(i), name))
				break;
		if (i == OPT_MAX)
			goto invalid;
		(*frontend_engine_ptr->p_option_set)(name, cJSON_IsTrue(cJSON_GetObjectItem(p, "value")));
	} else if (streq(op, "accept")) {
		const char *name = str(p, "name"), *history = str(p, "history");
		const unsigned char *ch;
		if (!*name || strlen(name) >= sizeof((*frontend_engine_ptr->p_player)->full_name) ||
		    strlen(history) > 4096)
			goto invalid;
		for (ch = (const unsigned char *)name; *ch; ++ch)
			if (*ch < 32 || *ch == 127)
				goto invalid;
		(*frontend_engine_ptr->p_cmdq_push)(CMD_NAME_CHOICE);
		(*frontend_engine_ptr->p_cmd_set_arg_string)((*frontend_engine_ptr->p_cmdq_peek)(), "name", name);
		(*frontend_engine_ptr->p_cmdq_push)(CMD_HISTORY_CHOICE);
		(*frontend_engine_ptr->p_cmd_set_arg_string)((*frontend_engine_ptr->p_cmdq_peek)(), "history", history);
		(*frontend_engine_ptr->p_cmdq_push)(CMD_ACCEPT_CHARACTER);
	} else if (streq(op, "cancel")) {
		response(id, cJSON_CreateObject());
		closing = true;
		connected = false;
		birth_waiting = false;
		return;
	} else
		goto invalid;
	birth_waiting = false;
	response(id, cJSON_CreateObject());
	return;
invalid:
	error(id, "invalid_argument",
	      "That character creation choice is not available.");
}
static int anybandui_birth_session(void)
{
	(*frontend_engine_ptr->p_event_add_handler)(EVENT_ENTER_BIRTH, birth_event, NULL);
	(*frontend_engine_ptr->p_event_add_handler)(EVENT_BIRTHPOINTS, birth_event, NULL);
	(*frontend_engine_ptr->p_cmdq_push)(CMD_BIRTH_INIT);
	(*frontend_engine_ptr->p_cmdq_execute)(CTX_BIRTH);
	(*frontend_engine_ptr->p_cmdq_push)(CMD_BIRTH_RESET);
	(*frontend_engine_ptr->p_cmdq_execute)(CTX_BIRTH);
	/* A replay suggests identity choices only; all stats/options remain under
	 * the normal creation workflow and can be changed before accepting. */
	if (*birth_race) {
		struct player_race *r;
		for (r = (*frontend_engine_ptr->p_races); r; r = r->next)
			if (streq(r->name, birth_race)) {
				birth_queue(CMD_CHOOSE_RACE, r->ridx);
				(*frontend_engine_ptr->p_cmdq_execute)(CTX_BIRTH);
				break;
			}
	}
	if (*birth_class) {
		struct player_class *cl;
		for (cl = (*frontend_engine_ptr->p_classes); cl; cl = cl->next)
			if (streq(cl->name, birth_class)) {
				birth_queue(CMD_CHOOSE_CLASS, cl->cidx);
				(*frontend_engine_ptr->p_cmdq_execute)(CTX_BIRTH);
				break;
			}
	}
	birth_active = true;
	birth_rolled = birth_quickstart;
	birth_previous = false;
	/* CMD_BIRTH_RESET already restored the original birth stats. Do not replace
	 * those with a suggested point allocation when opening a completed save. */
	if (!birth_quickstart) {
		birth_queue(CMD_RESET_STATS, 1);
		(*frontend_engine_ptr->p_cmdq_execute)(CTX_BIRTH);
	}
	while (!(*frontend_engine_ptr->p_character_generated) && connected) {
		birth_waiting = true;
		ready = false;
		publish();
		while (birth_waiting && connected)
			pump();
		(*frontend_engine_ptr->p_cmdq_execute)(CTX_BIRTH);
	}
	birth_active = birth_waiting = false;
	(*frontend_engine_ptr->p_event_remove_handler)(EVENT_BIRTHPOINTS, birth_event, NULL);
	(*frontend_engine_ptr->p_event_remove_handler)(EVENT_ENTER_BIRTH, birth_event, NULL);
	if (closing) (*frontend_engine_ptr->p_event_signal)(EVENT_LEAVE_BIRTH);
	return closing ? -1 : 0;
}
