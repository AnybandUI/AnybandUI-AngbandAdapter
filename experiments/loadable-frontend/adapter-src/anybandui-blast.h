/* Adapter-only preview of Angband ball geometry. Keep this aligned with
 * project() when updating this engine adapter; never execute effects to
 * preview. */
static bool project_blast_terrain(struct chunk *c, struct loc centre,
                                  struct loc grid, int flg)
{
	if ((flg & PROJECT_THRU) || (*frontend_engine_ptr->p_square_ispassable)(c, grid)) {
		if (!(*frontend_engine_ptr->p_square_isprojectable)(c, grid)) {
			int i;
			for (i = 0; i < 8; ++i) {
				struct loc adj = (*frontend_engine_ptr->p_loc_sum)(grid, (*frontend_engine_ptr->p_ddgrid_ddd)[i]);
				if ((*frontend_engine_ptr->p_square_in_bounds)(c, adj) && (*frontend_engine_ptr->p_los)(c, centre, adj))
					return true;
			}
			return false;
		}
	} else if (!(*frontend_engine_ptr->p_square_isprojectable)(c, grid))
		return false;
	return true;
}

/* Targeted player balls pass monsters and stop before walls. The caller
 * supplies either the real chunk (tests) or the remembered chunk (UI); no RNG
 * or mutation. The same 255-grid cap and row order as project() are
 * intentional. */
static int anybandui_ball_area(struct chunk *c, struct loc start,
                               struct loc target, int radius,
                               struct loc *centre, struct loc *grids)
{
	struct loc path[512];
	int n, i, x, y, count = 1;
	radius = MIN(MAX(radius, 0), (*frontend_engine_ptr->p_z_info)->max_range);
	*centre = start;
	n = (*frontend_engine_ptr->p_project_path)(c, path, (*frontend_engine_ptr->p_z_info)->max_range, start, target, 0);
	for (i = 0; i < n; ++i) {
		if (!(*frontend_engine_ptr->p_square_in_bounds)(c, path[i]) || !(*frontend_engine_ptr->p_square_ispassable)(c, path[i]))
			break;
		*centre = path[i];
	}
	n = i;
	grids[0] = *centre;
	for (y = centre->y - radius; y <= centre->y + radius; ++y) {
		for (x = centre->x - radius; x <= centre->x + radius; ++x) {
			struct loc grid = (*frontend_engine_ptr->p_loc)(x, y);
			bool on_path = false;
			if (count >= 255)
				return count;
			if ((*frontend_engine_ptr->p_loc_eq)(grid, *centre) || !(*frontend_engine_ptr->p_square_in_bounds)(c, grid))
				continue;
			if (!project_blast_terrain(c, *centre, grid, 0) ||
			    (*frontend_engine_ptr->p_distance)(*centre, grid) > radius)
				continue;
			for (i = 0; i < n; ++i)
				if ((*frontend_engine_ptr->p_loc_eq)(grid, path[i])) {
					on_path = true;
					break;
				}
			if (on_path || (*frontend_engine_ptr->p_los)(c, *centre, grid))
				grids[count++] = grid;
		}
	}
	return count;
}

/* Known-effect metadata only; never roll dice or execute an effect to preview.
 */
static int anybandui_blast_radius(void)
{
	const struct effect *e;
	int radius = 0;
	if (!aim_effect || !(*frontend_engine_ptr->p_player) || (*frontend_engine_ptr->p_player)->timed[TMD_CONFUSED] ||
	    (*frontend_engine_ptr->p_player)->timed[TMD_IMAGE] || (*frontend_engine_ptr->p_player)->timed[TMD_BLIND])
		return 0;
	for (e = aim_effect; e; e = e->next) {
		if (e->index == EF_RANDOM || e->index == EF_SELECT)
			return 0;
		if (e->index == EF_BALL) {
			int r = e->radius ? e->radius : 2;
			if (e->other > 0)
				r += (*frontend_engine_ptr->p_player)->lev / e->other;
			radius = MAX(radius, r);
		}
	}
	return MIN(radius, (*frontend_engine_ptr->p_z_info)->max_range);
}

static void anybandui_blast_preview(const char *id, const cJSON *p)
{
	const cJSON *jx = cJSON_GetObjectItem(p, "x"),
	            *jy = cJSON_GetObjectItem(p, "y");
	const int radius = anybandui_blast_radius();
	struct loc grid = (*frontend_engine_ptr->p_loc)(num(p, "x", -1), num(p, "y", -1)), centre, cells[255];
	cJSON *out, *tiles;
	int i, n;
	if (!streq(str(p, "context"), context_text)) {
		error(id, "stale_revision", "Aiming context changed.");
		return;
	}
	if (!radius ||
	    (!textui_aiming &&
	     (!target_ui_current || !(target_ui_current->mode & TARGET_KILL)))) {
		error(id, "busy", "No known ball spell is being aimed.");
		return;
	}
	if (!cJSON_IsNumber(jx) || !cJSON_IsNumber(jy) ||
	    jx->valuedouble != jx->valueint || jy->valuedouble != jy->valueint ||
	    !(*frontend_engine_ptr->p_square_in_bounds_fully)((*frontend_engine_ptr->p_player)->cave, grid)) {
		error(id, "invalid_argument", "Invalid target tile.");
		return;
	}
	out = cJSON_CreateObject();
	tiles = cJSON_AddArrayToObject(out, "tiles");
	string(out, "context", context_text);
	number(out, "x", grid.x);
	number(out, "y", grid.y);
	number(out, "radius", radius);
	/* All geometry reads the remembered chunk, including paths and LOS. */
	n = anybandui_ball_area((*frontend_engine_ptr->p_player)->cave, (*frontend_engine_ptr->p_player)->grid, grid, radius, &centre,
	                        cells);
	for (i = 0; i < n; ++i) {
		int xy[2] = {cells[i].x, cells[i].y};
		if ((*frontend_engine_ptr->p_square_isknown)((*frontend_engine_ptr->p_player)->cave, cells[i]))
			cJSON_AddItemToArray(tiles, ints(xy, 2));
	}
	response(id, out);
}
