/* Depth milestones use the existing generic history type, not a new save flag.
 * Matching the complete note also recognizes records written by earlier builds.
 */
static bool anybandui_depth_entry(const struct history_info *entry)
{
	char text[80];
	(*frontend_engine_ptr->p_strnfmt)(text, sizeof(text), "Reached depth %d", entry->dlev);
	return entry->dlev > 0 && streq(entry->event, text);
}
static int anybandui_journal_depth = -1;
static void anybandui_journal_level(game_event_type type, game_event_data *data,
                                    void *user)
{
	if (!(*frontend_engine_ptr->p_player) || !(*frontend_engine_ptr->p_character_generated))
		return;
	int depth = (*frontend_engine_ptr->p_player)->depth, previous = anybandui_journal_depth;
	anybandui_journal_depth = depth;
	/* The first display follows loading/birth, not a new visit. */
	if (previous < 0 || previous == depth || depth <= 0 ||
	    (*frontend_engine_ptr->p_player)->upkeep->arena_level)
		return;
	struct history_info *entries;
	size_t n = (*frontend_engine_ptr->p_history_get_list)((*frontend_engine_ptr->p_player), &entries);
	for (size_t i = 0; i < n; ++i)
		if (entries[i].dlev == depth && anybandui_depth_entry(&entries[i]))
			return;
	char text[80];
	(*frontend_engine_ptr->p_strnfmt)(text, sizeof(text), "Reached depth %d", depth);
	(*frontend_engine_ptr->p_history_add)((*frontend_engine_ptr->p_player), text, HIST_GENERIC);
}
/* Read-only projection of the native save-persistent history. Queries do not
 * generate milestones, consume RNG, identify artifacts or advance the game. */
static cJSON *anybandui_journal(bool finished)
{
	struct history_info *history;
	size_t i, count = (*frontend_engine_ptr->p_history_get_list)((*frontend_engine_ptr->p_player), &history);
	bool levels[PY_MAX_LEVEL + 1] = {false};
	cJSON *out = cJSON_CreateObject(),
	      *entries = cJSON_AddArrayToObject(out, "entries");
	for (i = 0; i < count; ++i) {
		const struct history_info *h = &history[i];
		const char *kind = NULL;
		cJSON *entry;
		if (hist_has(h->type, HIST_ARTIFACT_UNKNOWN))
			continue;
		/* Post-mortem identification also reveals artifacts never discovered.
		 */
		if (hist_has(h->type, HIST_ARTIFACT_LOST) &&
		    (*frontend_engine_ptr->p_prefix)(h->event, "Missed "))
			continue;
		if (hist_has(h->type, HIST_PLAYER_BIRTH))
			kind = "beginning";
		else if (hist_has(h->type, HIST_SLAY_UNIQUE))
			kind = "unique";
		else if (hist_has(h->type, HIST_ARTIFACT_KNOWN))
			kind = "artifact";
		else if (anybandui_depth_entry(h))
			kind = "depth";
		else if (hist_has(h->type, HIST_GAIN_LEVEL)) {
			if (h->clev < 1 || h->clev > PY_MAX_LEVEL || levels[h->clev])
				continue;
			levels[h->clev] = true;
			kind = "level";
		}
		if (!kind)
			continue;
		entry = cJSON_CreateObject();
		string(entry, "kind", kind);
		string(entry, "text", h->event);
		number(entry, "turn", h->turn);
		number(entry, "depth", h->dlev);
		number(entry, "level", h->clev);
		json_bool(entry, "lost", hist_has(h->type, HIST_ARTIFACT_LOST));
		cJSON_AddItemToArray(entries, entry);
	}
	if (finished) {
		char text[160];
		cJSON *entry = cJSON_CreateObject();
		if ((*frontend_engine_ptr->p_player)->total_winner)
			(*frontend_engine_ptr->p_my_strcpy)(text, "Victory. A legend remembered.", sizeof(text));
		else if (streq((*frontend_engine_ptr->p_player)->died_from, "Retiring"))
			(*frontend_engine_ptr->p_my_strcpy)(text, "Retired from adventuring.", sizeof(text));
		else
			(*frontend_engine_ptr->p_strnfmt)(text, sizeof(text), "The journey ended: %s",
			        (*frontend_engine_ptr->p_player)->died_from);
		string(entry, "kind", "ending");
		string(entry, "text", text);
		number(entry, "turn", (*frontend_engine_ptr->p_player)->total_energy / 100);
		number(entry, "depth", (*frontend_engine_ptr->p_player)->depth);
		number(entry, "level", (*frontend_engine_ptr->p_player)->lev);
		cJSON_AddItemToArray(entries, entry);
	}
	return out;
}
