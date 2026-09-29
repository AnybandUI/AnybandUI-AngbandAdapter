/* The wire representation is adapter-owned; the engine only supplies layers. */
struct map_visual {
	int terrain_attr, trap_attr, object_attr, actor_attr;
	wchar_t terrain_char, trap_char, object_char, actor_char;
	int feature, lighting;
	bool seen, hallucinated, player;
};

static void anybandui_map_visual(const struct grid_data *g,
    const struct grid_layer *layers, struct map_visual *visual)
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

static void map_visual_readonly(struct loc grid, struct map_visual *visual)
{
	struct grid_data g;
	struct grid_layer layers[MAP_LAYER_MAX] = {{0}};
	int a, ta;
	wchar_t c, tc;
	/* Stable visual noise, deliberately independent of the engine RNG. */
	unsigned seed =
	        ((unsigned)grid.x * 73856093u ^ (unsigned)grid.y * 19349663u) | 1u;
	map_info_readonly(grid, &g);
	grid_data_as_text_layers(&g, &a, &c, &ta, &tc, layers, seed);
	anybandui_map_visual(&g, layers, visual);
}

