/* Constants editor: persistent overrides, validated with the engine parser.
 * Allocation-sensitive constants are pinned per save before world creation. */
#include "anybandui-tuning-schema.h"
/* Adapter-only alternate constants source. Synchronous at input boundaries.
 * finish() owns the parser and publishes z_info even on failure; cleanup()
 * releases that temporary value before the live pointer is restored. */
static char *custom_constants_text;
static errr tuning_parse_text(struct parser *p, const char *text)
{
	char line[2048];
	while (*text) {
		size_t n = 0;
		while (*text && *text != '\n') {
			if (n + 1 >= sizeof(line))
				return PARSE_ERROR_INVALID_VALUE;
			line[n++] = *text++;
		}
		if (*text == '\n')
			++text;
		line[n] = 0;
		enum parser_error result = (*frontend_engine_ptr->p_parser_parse)(p, line);
		if (result)
			return result;
	}
	return PARSE_ERROR_NONE;
}
static errr tuning_run_constants(struct parser *p)
{
	return tuning_parse_text(p, custom_constants_text);
}
static bool validate_constants_text(const char *text)
{
	struct angband_constants *live = (*frontend_engine_ptr->p_z_info);
	struct parser *p = (*frontend_engine_ptr->p_constants_parser).init();
	errr parsed = tuning_parse_text(p, text);
	errr finished = (*frontend_engine_ptr->p_constants_parser).finish(p);
	(*frontend_engine_ptr->p_constants_parser).cleanup();
	(*frontend_engine_ptr->p_z_info) = live;
	return parsed == PARSE_ERROR_NONE && finished == PARSE_ERROR_NONE;
}

static cJSON *tuning_entries, *tuning_active_values;
static char tuning_error[256];

static void tuning_path(char *path, size_t size, const char *name)
{
	char leaf[100];
	if (name)
		(*frontend_engine_ptr->p_strnfmt)(leaf, sizeof(leaf), "tuning-%s.json", name);
	else
		(*frontend_engine_ptr->p_my_strcpy)(leaf, "tuning-overrides.json", sizeof(leaf));
	(*frontend_engine_ptr->p_path_build)(path, size, (*frontend_engine_ptr->p_ANGBAND_DIR_USER), leaf);
}
static cJSON *tuning_entry(const char *id)
{
	cJSON *e;
	cJSON_ArrayForEach(e, tuning_entries) if (streq(str(e, "id"), id)) return e;
	return NULL;
}
static bool tuning_init(void)
{
	if (tuning_entries)
		return true;
	char path[1024], line[2048];
	int counts[4] = {0};
	const char *tiers[] = {"melee-critical-level", "ranged-critical-level",
	                       "o-melee-critical-level", "o-ranged-critical-level"};
	tuning_entries = cJSON_Parse(anybandui_tuning_schema);
	(*frontend_engine_ptr->p_path_build)(path, sizeof(path), (*frontend_engine_ptr->p_ANGBAND_DIR_GAMEDATA), "constants.txt");
	ang_file *f = (*frontend_engine_ptr->p_file_open)(path, MODE_READ, FTYPE_TEXT);
	if (!f)
		goto failed;
	while ((*frontend_engine_ptr->p_file_getl)(f, line, sizeof(line))) {
		if (!*line || *line == '#')
			continue;
		char *parts[8], *next = line;
		int n = 0;
		while (next && n < 8) {
			parts[n++] = next;
			next = strchr(next, ':');
			if (next)
				*next++ = 0;
		}
		if (n < 3)
			continue;
		int tier = -1;
		for (int i = 0; i < 4; ++i)
			if (streq(parts[0], tiers[i]))
				tier = i;
		char id[120];
		if (tier >= 0)
			(*frontend_engine_ptr->p_strnfmt)(id, sizeof(id), "%s:%d", parts[0], counts[tier]++);
		else
			(*frontend_engine_ptr->p_strnfmt)(id, sizeof(id), "%s:%s", parts[0], parts[1]);
		cJSON *e = tuning_entry(id), *value;
		if (!e) {
			(*frontend_engine_ptr->p_file_close)(f);
			goto failed;
		}
		if (tier >= 0) {
			value = cJSON_CreateArray();
			for (int i = 1; i < n - 1; ++i)
				cJSON_AddItemToArray(value, cJSON_CreateNumber(atoi(parts[i])));
			cJSON_AddItemToArray(value, cJSON_CreateString(parts[n - 1]));
		} else
			value = cJSON_CreateNumber(atoi(parts[2]));
		cJSON_AddItemToObject(e, "default", value);
	}
	(*frontend_engine_ptr->p_file_close)(f);
	cJSON *e;
	cJSON_ArrayForEach(
	        e,
	        tuning_entries) if (!cJSON_GetObjectItem(e, "default")) goto failed;
	return true;
failed:
	cJSON_Delete(tuning_entries);
	tuning_entries = NULL;
	(*frontend_engine_ptr->p_my_strcpy)(tuning_error,
	          "The installed constants.txt does not match the tuning editor "
	          "schema.",
	          sizeof(tuning_error));
	return false;
}
static cJSON *tuning_defaults(void)
{
	cJSON *values = cJSON_CreateObject(), *e;
	cJSON_ArrayForEach(e, tuning_entries) cJSON_AddItemToObject(
	        values, str(e, "id"),
	        cJSON_Duplicate(cJSON_GetObjectItem(e, "default"), true));
	return values;
}
static cJSON *tuning_read(const char *path, unsigned long *hash)
{
	*hash = 2166136261u;
	if (!(*frontend_engine_ptr->p_file_exists)(path))
		return cJSON_CreateObject();
	ang_file *f = (*frontend_engine_ptr->p_file_open)(path, MODE_READ, FTYPE_TEXT);
	char line[2048];
	if (!f)
		return NULL;
	char *text = (*frontend_engine_ptr->p_mem_zalloc)(65536);
	size_t used = 0;
	while ((*frontend_engine_ptr->p_file_getl)(f, line, sizeof(line))) {
		size_t n = strlen(line);
		if (used + n + 2 >= 65536) {
			(*frontend_engine_ptr->p_file_close)(f);
			(*frontend_engine_ptr->p_mem_free)(text);
			return NULL;
		}
		memcpy(text + used, line, n);
		used += n;
		text[used++] = '\n';
	}
	(*frontend_engine_ptr->p_file_close)(f);
	for (size_t i = 0; i < used; ++i)
		*hash = (*hash ^ (unsigned char)text[i]) * 16777619u;
	cJSON *j = cJSON_Parse(text);
	(*frontend_engine_ptr->p_mem_free)(text);
	if (!cJSON_IsObject(j)) {
		cJSON_Delete(j);
		return NULL;
	}
	return j;
}
static bool tuning_integer(const cJSON *v, int min, int max)
{
	return cJSON_IsNumber(v) && v->valuedouble == v->valueint &&
	       v->valueint >= min && v->valueint <= max;
}
static bool tuning_overlay(cJSON *values, const cJSON *changes,
                           bool structural_only)
{
	if (!cJSON_IsObject(changes))
		return false;
	const cJSON *v;
	cJSON_ArrayForEach(v, changes)
	{
		cJSON *e = tuning_entry(v->string);
		if (!e || (structural_only &&
		           !cJSON_IsTrue(cJSON_GetObjectItem(e, "structural"))))
			return false;
		cJSON *def = cJSON_GetObjectItem(e, "default");
		bool valid = true;
		if (cJSON_IsTrue(cJSON_GetObjectItem(e, "tier"))) {
			int n = cJSON_GetArraySize(def);
			valid = cJSON_IsArray(v) && cJSON_GetArraySize(v) == n;
			for (int i = 0; valid && i < n - 1; ++i)
				valid = tuning_integer(cJSON_GetArrayItem(v, i),
				                       i == 0 ? (n == 3 ? 1 : -1) : 0,
				                       i == 0 ? 10000 : 100);
			const char *message = str(e, "unused");
			if (valid) {
				const cJSON *m = cJSON_GetArrayItem(v, n - 1);
				message = cJSON_IsString(m) ? m->valuestring : "";
				valid = streq(message, "HIT_GOOD") ||
				        streq(message, "HIT_GREAT") ||
				        streq(message, "HIT_SUPERB") ||
				        streq(message, "HIT_HI_GREAT") ||
				        streq(message, "HIT_HI_SUPERB");
			}
		} else {
			int lo = num(e, "minimum", 0), hi = num(e, "maximum", 0);
			valid = tuning_integer(v, lo, hi);
			if (lo == hi)
				valid = cJSON_Compare(v, def, true);
			if (strstr(v->string, "chance-range") && v->valueint % 100)
				valid = false;
		}
		if (!valid) {
			(*frontend_engine_ptr->p_strnfmt)(tuning_error, sizeof(tuning_error),
			        "Invalid value for %s. Check the displayed range and "
			        "combat tier ordering.",
			        str(e, "label"));
			return false;
		}
		cJSON_ReplaceItemInObject(values, v->string, cJSON_Duplicate(v, true));
	}
	return true;
}
static char *tuning_text(const cJSON *values)
{
	char *text = (*frontend_engine_ptr->p_mem_zalloc)(65536), line[512];
	size_t used = 0;
	cJSON *e;
	if (num(values, "world:feeling-need", 0) >
	    num(values, "world:feeling-total", 0)) {
		(*frontend_engine_ptr->p_my_strcpy)(tuning_error,
		          "Feeling squares needed cannot exceed the total feeling "
		          "squares.",
		          sizeof(tuning_error));
		(*frontend_engine_ptr->p_mem_free)(text);
		return NULL;
	}
	if (num(values, "obj-make:default-lamp", 0) >
	    num(values, "obj-make:fuel-lamp", 0)) {
		(*frontend_engine_ptr->p_my_strcpy)(tuning_error,
		          "Starting lantern fuel cannot exceed maximum lantern fuel.",
		          sizeof(tuning_error));
		(*frontend_engine_ptr->p_mem_free)(text);
		return NULL;
	}
	cJSON_ArrayForEach(e, tuning_entries)
	{
		const cJSON *v = cJSON_GetObjectItem(values, str(e, "id"));
		if (cJSON_IsArray(v)) {
			int n = cJSON_GetArraySize(v);
			const char *message_token = cJSON_GetArrayItem(v, n - 1)->valuestring;
			if (n == 4)
				(*frontend_engine_ptr->p_strnfmt)(line, sizeof(line), "%s:%d:%d:%d:%s\n", str(e, "group"),
				        cJSON_GetArrayItem(v, 0)->valueint,
				        cJSON_GetArrayItem(v, 1)->valueint,
				        cJSON_GetArrayItem(v, 2)->valueint, message_token);
			else
				(*frontend_engine_ptr->p_strnfmt)(line, sizeof(line), "%s:%d:%d:%s\n", str(e, "group"),
				        cJSON_GetArrayItem(v, 0)->valueint,
				        cJSON_GetArrayItem(v, 1)->valueint, message_token);
		} else
			(*frontend_engine_ptr->p_strnfmt)(line, sizeof(line), "%s:%d\n", str(e, "id"), v->valueint);
		size_t n = strlen(line);
		if (used + n + 1 >= 65536) {
			(*frontend_engine_ptr->p_mem_free)(text);
			return NULL;
		}
		memcpy(text + used, line, n);
		used += n;
	}
	if (!validate_constants_text(text)) {
		(*frontend_engine_ptr->p_my_strcpy)(tuning_error,
		          "Angband rejected these values. Critical tier cutoffs must "
		          "increase (the last cutoff is unused).",
		          sizeof(tuning_error));
		(*frontend_engine_ptr->p_mem_free)(text);
		return NULL;
	}
	return text;
}
static bool tuning_write(const char *path, const cJSON *values)
{
	char temp[1100], backup[1100];
	(*frontend_engine_ptr->p_strnfmt)(temp, sizeof(temp), "%s.tmp", path);
	(*frontend_engine_ptr->p_strnfmt)(backup, sizeof(backup), "%s.bak", path);
	char *text = cJSON_Print(values);
	if (!text)
		return false;
	ang_file *f = (*frontend_engine_ptr->p_file_open)(temp, MODE_WRITE, FTYPE_TEXT);
	if (!f) {
		cJSON_free(text);
		return false;
	}
	bool ok = (*frontend_engine_ptr->p_file_putf)(f, "%s\n", text);
	if (!(*frontend_engine_ptr->p_file_close)(f))
		ok = false;
	cJSON_free(text);
	if (!ok) {
		(*frontend_engine_ptr->p_file_delete)(temp);
		return false;
	}
	bool existed = (*frontend_engine_ptr->p_file_exists)(path);
	if (existed) {
		(*frontend_engine_ptr->p_file_delete)(backup);
		if (!(*frontend_engine_ptr->p_file_move)(path, backup)) {
			(*frontend_engine_ptr->p_file_delete)(temp);
			return false;
		}
	}
	if (!(*frontend_engine_ptr->p_file_move)(temp, path)) {
		if (existed)
			(*frontend_engine_ptr->p_file_move)(backup, path);
		(*frontend_engine_ptr->p_file_delete)(temp);
		return false;
	}
	return true;
}
static cJSON *tuning_saved(unsigned long *hash, bool *valid)
{
	char path[1024];
	tuning_path(path, sizeof(path), NULL);
	cJSON *changes = tuning_read(path, hash), *values = tuning_defaults();
	*valid = changes && tuning_overlay(values, changes, false);
	cJSON_Delete(changes);
	if (*valid) {
		char *text = tuning_text(values);
		*valid = text != NULL;
		(*frontend_engine_ptr->p_mem_free)(text);
	}
	if (!*valid) {
		cJSON_Delete(values);
		values = tuning_defaults();
	}
	return values;
}
static cJSON *tuning_catalog(void)
{
	unsigned long hash;
	bool valid;
	cJSON *values = tuning_saved(&hash, &valid), *out = cJSON_CreateObject();
	cJSON_AddItemToObject(out, "entries",
	                      cJSON_Duplicate(tuning_entries, true));
	cJSON_AddItemToObject(out, "values", values);
	counter(out, "revision", hash);
	if (tuning_active_values)
		cJSON_AddItemToObject(out, "active_values",
		                      cJSON_Duplicate(tuning_active_values, true));
	if (!valid)
		string(out, "warning",
		       "Saved overrides could not be read. Showing stock values; "
		       "Restore defaults or edit a value and save to replace the "
		       "invalid file.");
	return out;
}
static void tuning_request(const char *id, const char *method,
                           const cJSON *params)
{
	tuning_error[0] = 0;
	if (!tuning_init()) {
		error(id, "invalid_data", tuning_error);
		return;
	}
	if (streq(method, "tuning.get")) {
		response(id, tuning_catalog());
		return;
	}
	char path[1024], revision[32];
	unsigned long hash;
	bool valid;
	cJSON *current = tuning_saved(&hash, &valid);
	cJSON_Delete(current);
	(*frontend_engine_ptr->p_strnfmt)(revision, sizeof(revision), "%lu", hash);
	if (!streq(str(params, "revision"), revision)) {
		error(id, "stale_revision",
		      "Tuning changed on disk. Reopen Settings before saving.");
		return;
	}
	cJSON *values = tuning_defaults();
	if (!tuning_overlay(values, cJSON_GetObjectItem(params, "values"), false)) {
		cJSON_Delete(values);
		error(id, "invalid_argument",
		      *tuning_error ? tuning_error : "Unknown tuning setting.");
		return;
	}
	char *text = tuning_text(values);
	if (!text) {
		cJSON_Delete(values);
		error(id, "invalid_argument", tuning_error);
		return;
	}
	(*frontend_engine_ptr->p_mem_free)(text);
	cJSON *overrides = cJSON_CreateObject(), *e;
	cJSON_ArrayForEach(e, tuning_entries)
	{
		const cJSON *v = cJSON_GetObjectItem(values, str(e, "id"));
		if (!cJSON_Compare(v, cJSON_GetObjectItem(e, "default"), true))
			cJSON_AddItemToObject(overrides, str(e, "id"),
			                      cJSON_Duplicate(v, true));
	}
	tuning_path(path, sizeof(path), NULL);
	bool ok = tuning_write(path, overrides);
	cJSON_Delete(values);
	cJSON_Delete(overrides);
	if (!ok)
		error(id, "io_error",
		      "Could not save tuning. Previous overrides were preserved.");
	else
		response(id, tuning_catalog());
}
static bool tuning_prepare(const char *name, bool load)
{
	if (!tuning_init())
		return false;
	unsigned long hash;
	bool valid;
	cJSON *values = tuning_saved(&hash, &valid);
	if (!valid) {
		cJSON_Delete(values);
		(*frontend_engine_ptr->p_my_strcpy)(tuning_error,
		          "Invalid saved tuning. Open Settings > Game tuning and "
		          "restore or correct it.",
		          sizeof(tuning_error));
		return false;
	}
	char path[1024];
	tuning_path(path, sizeof(path), name);
	cJSON *profile = NULL, *e;
	if (load && (*frontend_engine_ptr->p_file_exists)(path)) {
		profile = tuning_read(path, &hash);
		cJSON_ArrayForEach(e, tuning_entries) if (
		        cJSON_IsTrue(cJSON_GetObjectItem(e, "structural")) &&
		        !cJSON_GetObjectItem(profile, str(e, "id")))
		{
			cJSON_Delete(profile);
			profile = NULL;
			break;
		}
		if (!profile || !tuning_overlay(values, profile, true)) {
			cJSON_Delete(values);
			cJSON_Delete(profile);
			(*frontend_engine_ptr->p_my_strcpy)(tuning_error,
			          "This save's tuning profile is invalid. Restore its "
			          "tuning backup before loading.",
			          sizeof(tuning_error));
			return false;
		}
	} else {
		profile = cJSON_CreateObject();
		cJSON_ArrayForEach(e, tuning_entries) if (
		        cJSON_IsTrue(cJSON_GetObjectItem(e, "structural")))
		{
			const cJSON *v = load ? cJSON_GetObjectItem(e, "default")
			                      : cJSON_GetObjectItem(values, str(e, "id"));
			cJSON_AddItemToObject(profile, str(e, "id"),
			                      cJSON_Duplicate(v, true));
		}
		tuning_overlay(values, profile, true);
	}
	char *text = tuning_text(values);
	cJSON_Delete(tuning_active_values);
	tuning_active_values = cJSON_Duplicate(values, true);
	cJSON_Delete(values);
	if (!text) {
		cJSON_Delete(profile);
		return false;
	}
	if (!tuning_write(path, profile)) {
		(*frontend_engine_ptr->p_mem_free)(text);
		cJSON_Delete(profile);
		(*frontend_engine_ptr->p_my_strcpy)(tuning_error,
		          "Could not preserve this character's tuning profile.",
		          sizeof(tuning_error));
		return false;
	}
	cJSON_Delete(profile);
	custom_constants_text = text;
	return true;
}
