/**
   \file ui-map.h
   \brief Writing level map info to the screen
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

extern void grid_data_as_text(struct frontend_grid_data *g, int *ap, wchar_t *cp,
							  int *tap, wchar_t *tcp);
extern void move_cursor_relative(int y, int x);
extern void print_rel(wchar_t c, uint8_t a, int y, int x);
extern void prt_map(void);
extern void display_map(int *cy, int *cx);
extern void do_cmd_view_map(void);

/* Ordered drawing layers, before occlusion by objects and actors. */
enum map_layer { MAP_TERRAIN, MAP_TRAP, MAP_OBJECT, MAP_ACTOR, MAP_LAYER_MAX };
struct grid_layer {
	int attr;
	wchar_t chr;
};
struct map_noise {
	struct grid_layer object, actor;
};
struct map_presentation {
	struct grid_layer layers[MAP_LAYER_MAX], top;
};
/* Reads known grid information and visual preferences; no RNG or state writes.
 * Hallucinated object/actor glyphs are supplied by the caller. */
void map_present(const struct frontend_grid_data *g, const struct map_noise *noise,
	struct map_presentation *result);


