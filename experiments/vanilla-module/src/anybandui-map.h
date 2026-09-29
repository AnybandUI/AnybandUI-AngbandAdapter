/* The frontend derives map layers from the engine's existing knowledge state. */
struct map_visual {
	int terrain_attr, trap_attr, object_attr, actor_attr;
	wchar_t terrain_char, trap_char, object_char, actor_char;
	int feature, lighting;
	bool seen, hallucinated, player;
};

static void anybandui_map_visual(const struct frontend_grid_data *g,
                                 const struct grid_layer *layers,
                                 struct map_visual *visual)
{
	visual->terrain_attr = layers[MAP_TERRAIN].attr;
	visual->terrain_char = layers[MAP_TERRAIN].chr;
	visual->trap_attr = layers[MAP_TRAP].attr;
	visual->trap_char = layers[MAP_TRAP].chr;
	visual->object_attr = layers[MAP_OBJECT].attr;
	visual->object_char = layers[MAP_OBJECT].chr;
	visual->actor_attr = layers[MAP_ACTOR].attr;
	visual->actor_char = layers[MAP_ACTOR].chr;
	visual->feature = g->f_idx;
	visual->lighting = g->lighting;
	visual->seen = g->in_view;
	visual->hallucinated = g->hallucinate;
	visual->player = g->is_player;
}

/* Private visual randomness. Repeated snapshots in a turn are identical. */
static unsigned anybandui_visual_random(unsigned *state, unsigned bound)
{
	*state ^= *state << 13;
	*state ^= *state >> 17;
	*state ^= *state << 5;
	return *state % bound;
}
static void map_visual_readonly(struct loc grid, struct map_visual *visual)
{
	struct frontend_grid_data g;
	struct map_noise noise = {{0}};
	struct map_presentation result;
	unsigned seed =
	        ((unsigned)grid.x * 73856093u ^ (unsigned)grid.y * 19349663u ^
	         (unsigned)turn * 83492791u) |
	        1u;
	map_info_readonly(grid, &g);
	if (g.hallucinate && !g.m_idx && !g.first_kind) {
		if (!anybandui_visual_random(&seed, 128) && g.f_idx != FEAT_PERM)
			g.m_idx = 1;
		else if (!anybandui_visual_random(&seed, 128) && g.f_idx != FEAT_PERM)
			g.first_kind = k_info;
		else
			g.hallucinate = false;
	}
	if (g.hallucinate) {
		if (g.first_kind && !g.unseen_money && !g.unseen_object) {
			for (;;) {
				struct object_kind *kind =
				        &k_info[1 + anybandui_visual_random(&seed,
				                                            z_info->k_max - 1)];
				if (!kind->name)
					continue;
				noise.object.attr = kind_x_attr[kind->kidx];
				noise.object.chr = kind_x_char[kind->kidx];
				if (noise.object.attr && noise.object.chr)
					break;
			}
		}
		if (g.m_idx) {
			for (;;) {
				struct monster_race *race =
				        &r_info[anybandui_visual_random(&seed, z_info->r_max)];
				if (!race->name)
					continue;
				noise.actor.attr = monster_x_attr[race->ridx];
				noise.actor.chr = monster_x_char[race->ridx];
				break;
			}
		}
	}
	map_present(&g, &noise, &result);
	anybandui_map_visual(&g, result.layers, visual);
}


