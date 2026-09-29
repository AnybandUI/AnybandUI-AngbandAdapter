/* Read bundled artwork mappings without importing user preferences, commands,
 * colours, inscriptions or window flags into a read-only presentation pass. */
static bool tile_pref_file(const char *directory, const char *name, int depth)
{
	char path[1024], line[1024];
	bool ok = true;
	if (depth > 8 || strchr(name, '/') || strchr(name, '\\') ||
	    strstr(name, ".."))
		return false;
	(*frontend_engine_ptr->p_path_build)(path, sizeof(path), directory, name);
	ang_file *f = (*frontend_engine_ptr->p_file_open)(path, MODE_READ, FTYPE_TEXT);
	if (!f)
		return false;
	struct parser *p = (*frontend_engine_ptr->p_init_parse_prefs)(false);
	while ((*frontend_engine_ptr->p_file_getl)(f, line, sizeof(line))) {
		if ((*frontend_engine_ptr->p_prefix)(line, "%:")) {
			struct prefs_data *d = (*frontend_engine_ptr->p_parser_priv)(p);
			if (!d->bypass && !tile_pref_file(directory, line + 2, depth + 1))
				ok = false;
		} else if ((*frontend_engine_ptr->p_prefix)(line, "?:") || (*frontend_engine_ptr->p_prefix)(line, "monster:") ||
		           (*frontend_engine_ptr->p_prefix)(line, "object:") || (*frontend_engine_ptr->p_prefix)(line, "feat:") ||
		           (*frontend_engine_ptr->p_prefix)(line, "trap:") || (*frontend_engine_ptr->p_prefix)(line, "flavor:")) {
			if ((*frontend_engine_ptr->p_parser_parse)(p, line) != PARSE_ERROR_NONE)
				ok = false;
		}
	}
	(*frontend_engine_ptr->p_file_close)(f);
	(*frontend_engine_ptr->p_mem_free)((*frontend_engine_ptr->p_parser_priv)(p));
	(*frontend_engine_ptr->p_parser_destroy)(p);
	return ok;
}
static bool anybandui_tile_pref_file(const char *directory, const char *name)
{
	return tile_pref_file(directory, name, 0);
}

/* Tile visuals are a second, read-only presentation of the known map. The
 * terminal and all gameplay continue to use their original ASCII visuals. */
struct anybandui_visual_bank {
	uint8_t *ma, *ka, *fa, *terrain_a[LIGHTING_MAX], *trap_a[LIGHTING_MAX];
	wchar_t *mc, *kc, *fc, *terrain_c[LIGHTING_MAX], *trap_c[LIGHTING_MAX];
};
static struct anybandui_visual_bank anybandui_tile_bank;
static int anybandui_tiles_id, anybandui_tiles_loaded;
static bool anybandui_tiles_failed;

static void anybandui_visual_swap(struct anybandui_visual_bank *b)
{
#define SWAP_VISUAL(field, global, type)                                       \
	do {                                                                       \
		type *tmp = global;                                                    \
		global = b->field;                                                     \
		b->field = tmp;                                                        \
	} while (0)
	SWAP_VISUAL(ma, (*frontend_engine_ptr->p_monster_x_attr), uint8_t);
	SWAP_VISUAL(mc, (*frontend_engine_ptr->p_monster_x_char), wchar_t);
	SWAP_VISUAL(ka, (*frontend_engine_ptr->p_kind_x_attr), uint8_t);
	SWAP_VISUAL(kc, (*frontend_engine_ptr->p_kind_x_char), wchar_t);
	SWAP_VISUAL(fa, (*frontend_engine_ptr->p_flavor_x_attr), uint8_t);
	SWAP_VISUAL(fc, (*frontend_engine_ptr->p_flavor_x_char), wchar_t);
	for (int i = 0; i < LIGHTING_MAX; ++i) {
		SWAP_VISUAL(terrain_a[i], (*frontend_engine_ptr->p_feat_x_attr)[i], uint8_t);
		SWAP_VISUAL(terrain_c[i], (*frontend_engine_ptr->p_feat_x_char)[i], wchar_t);
		SWAP_VISUAL(trap_a[i], (*frontend_engine_ptr->p_trap_x_attr)[i], uint8_t);
		SWAP_VISUAL(trap_c[i], (*frontend_engine_ptr->p_trap_x_char)[i], wchar_t);
	}
#undef SWAP_VISUAL
}
static void anybandui_tiles_free(void)
{
	struct anybandui_visual_bank *b = &anybandui_tile_bank;
	(*frontend_engine_ptr->p_mem_free)(b->ma);
	(*frontend_engine_ptr->p_mem_free)(b->mc);
	(*frontend_engine_ptr->p_mem_free)(b->ka);
	(*frontend_engine_ptr->p_mem_free)(b->kc);
	(*frontend_engine_ptr->p_mem_free)(b->fa);
	(*frontend_engine_ptr->p_mem_free)(b->fc);
	for (int i = 0; i < LIGHTING_MAX; ++i) {
		(*frontend_engine_ptr->p_mem_free)(b->terrain_a[i]);
		(*frontend_engine_ptr->p_mem_free)(b->terrain_c[i]);
		(*frontend_engine_ptr->p_mem_free)(b->trap_a[i]);
		(*frontend_engine_ptr->p_mem_free)(b->trap_c[i]);
	}
	memset(b, 0, sizeof(*b));
	anybandui_tiles_loaded = 0;
}
static bool anybandui_tiles_prepare(void)
{
	graphics_mode *mode = (*frontend_engine_ptr->p_get_graphics_mode)(anybandui_tiles_id),
	              *old_mode = (*frontend_engine_ptr->p_current_graphics_mode);
	int old_graphics = (*frontend_engine_ptr->p_use_graphics), max_flavor = 0;
	uint8_t old_proj_a[PROJ_MAX][BOLT_MAX];
	wchar_t old_proj_c[PROJ_MAX][BOLT_MAX];
	struct anybandui_visual_bank *b = &anybandui_tile_bank;
	if (!anybandui_tiles_id || !mode || anybandui_tiles_failed)
		return false;
	if (anybandui_tiles_loaded == anybandui_tiles_id)
		return true;
	anybandui_tiles_free();
	for (struct flavor *f = (*frontend_engine_ptr->p_flavors); f; f = f->next)
		max_flavor = MAX(max_flavor, f->fidx);
#define ALLOC_VISUAL(field, count, type)                                       \
	b->field = (*frontend_engine_ptr->p_mem_zalloc)((count) * sizeof(type))
	ALLOC_VISUAL(ma, (*frontend_engine_ptr->p_z_info)->r_max, uint8_t);
	ALLOC_VISUAL(mc, (*frontend_engine_ptr->p_z_info)->r_max, wchar_t);
	ALLOC_VISUAL(ka, (*frontend_engine_ptr->p_z_info)->k_max, uint8_t);
	ALLOC_VISUAL(kc, (*frontend_engine_ptr->p_z_info)->k_max, wchar_t);
	ALLOC_VISUAL(fa, max_flavor + 1, uint8_t);
	ALLOC_VISUAL(fc, max_flavor + 1, wchar_t);
	for (int i = 0; i < LIGHTING_MAX; ++i) {
		ALLOC_VISUAL(terrain_a[i], FEAT_MAX, uint8_t);
		ALLOC_VISUAL(terrain_c[i], FEAT_MAX, wchar_t);
		ALLOC_VISUAL(trap_a[i], (*frontend_engine_ptr->p_z_info)->trap_max, uint8_t);
		ALLOC_VISUAL(trap_c[i], (*frontend_engine_ptr->p_z_info)->trap_max, wchar_t);
	}
#undef ALLOC_VISUAL
	memcpy(old_proj_a, (*frontend_engine_ptr->p_proj_to_attr), sizeof(old_proj_a));
	memcpy(old_proj_c, (*frontend_engine_ptr->p_proj_to_char), sizeof(old_proj_c));
	anybandui_visual_swap(b);
	(*frontend_engine_ptr->p_use_graphics) = anybandui_tiles_id;
	(*frontend_engine_ptr->p_current_graphics_mode) = mode;
	(*frontend_engine_ptr->p_reset_visuals)(false);
	anybandui_tiles_failed = !anybandui_tile_pref_file(mode->path, mode->pref);
	anybandui_visual_swap(b);
	(*frontend_engine_ptr->p_use_graphics) = old_graphics;
	(*frontend_engine_ptr->p_current_graphics_mode) = old_mode;
	memcpy((*frontend_engine_ptr->p_proj_to_attr), old_proj_a, sizeof(old_proj_a));
	memcpy((*frontend_engine_ptr->p_proj_to_char), old_proj_c, sizeof(old_proj_c));
	if (anybandui_tiles_failed)
		return false;
	anybandui_tiles_loaded = anybandui_tiles_id;
	return true;
}
static void anybandui_capture_tiles(cJSON *view, int ox, int oy, int width,
                                    int height)
{
	if (!anybandui_tiles_prepare())
		return;
	graphics_mode *old_mode = (*frontend_engine_ptr->p_current_graphics_mode);
	int old_graphics = (*frontend_engine_ptr->p_use_graphics);
	cJSON *rows = cJSON_CreateArray();
	anybandui_visual_swap(&anybandui_tile_bank);
	(*frontend_engine_ptr->p_use_graphics) = anybandui_tiles_id;
	(*frontend_engine_ptr->p_current_graphics_mode) = (*frontend_engine_ptr->p_get_graphics_mode)(anybandui_tiles_id);
	for (int y = 0; y < height; ++y) {
		cJSON *row = cJSON_CreateArray();
		for (int x = 0; x < width; ++x) {
			struct map_visual v;
			map_visual_readonly((*frontend_engine_ptr->p_loc)(x + ox, y + oy), &v);
			int layers[8] = {v.terrain_char, v.terrain_attr, v.trap_char,
			                 v.trap_attr,    v.object_char,  v.object_attr,
			                 v.actor_char,   v.actor_attr};
			cJSON_AddItemToArray(row, ints(layers, 8));
		}
		cJSON_AddItemToArray(rows, row);
	}
	anybandui_visual_swap(&anybandui_tile_bank);
	(*frontend_engine_ptr->p_use_graphics) = old_graphics;
	(*frontend_engine_ptr->p_current_graphics_mode) = old_mode;
	number(view, "tileset", anybandui_tiles_id);
	cJSON_AddItemToObject(view, "tiles", rows);
}
