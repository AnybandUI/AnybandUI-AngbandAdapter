/**
 * \file ui-map-presentation.c
 * \brief Read-only map presentation shared by frontends
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

#include "angband.h"
#include "cave.h"
#include "grafmode.h"
#include "init.h"
#include "mon-predicate.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-timed.h"
#include "trap.h"
#include "ui-map.h"
#include "ui-object.h"
#include "ui-output.h"
#include "ui-prefs.h"
#include "ui-term.h"


/**
 * Get the graphics of a listed trap.
 *
 * We should probably have better handling of stacked traps, but that can
 * wait until we do, in fact, have stacked traps under normal conditions.
 * Return true if it's a web
 */
static bool get_trap_graphics(struct chunk *c, const struct frontend_grid_data *g, int *a,
							  wchar_t *w)
{
    /* Trap is visible */
    if (trf_has(g->trap->flags, TRF_VISIBLE) ||
		trf_has(g->trap->flags, TRF_GLYPH) ||
		trf_has(g->trap->flags, TRF_WEB)) {
		/* Get the graphics */
		*a = trap_x_attr[g->lighting][g->trap->kind->tidx];
		*w = trap_x_char[g->lighting][g->trap->kind->tidx];
    }

	return trf_has(g->trap->flags, TRF_WEB);
}

/**
 * Apply text lighting effects
 */
static void grid_get_attr(const struct frontend_grid_data *g, int *a)
{
	/* Save the high-bit, since it's used for attr inversion in GCU */
	int a0 = *a & 0x80;

	/* Remove the high bit so we can add it back again at the end */
	*a = (*a & 0x7F);

	/* Play with fg colours for terrain affected by torchlight */
	if (feat_is_torch(g->f_idx)) {
		/* Brighten if torchlit, darken if out of LoS, super dark for UNLIGHT */
		switch (g->lighting) {
			case LIGHTING_TORCH: *a = get_color(*a, ATTR_LIGHT, 1); break;
			case LIGHTING_LIT: *a = get_color(*a, ATTR_DARK, 1); break;
			case LIGHTING_DARK: *a = get_color(*a, ATTR_DARK, 2); break;
			default: break;
		}
	}

	/* Add the attr inversion back for GCU */
	if (a0) {
		*a = a0 | *a;
	}
	/* Hybrid or block walls */
	if (use_graphics == GRAPHICS_NONE && feat_is_wall(g->f_idx)) {
		if (OPT(player, hybrid_walls))
			*a = *a + (MULT_BG * BG_DARK);
		else if (OPT(player, solid_walls))
			*a = *a + (MULT_BG * BG_SAME);
	}
}

/**
 * This function takes a pointer to a grid info struct describing the
 * contents of a grid location (as obtained through the function map_info)
 * and fills in the character and attr pairs for display.
 *
 * The result contains the top glyph and the separate terrain, trap, object,
 * and actor layers.  Missing layers are zero.  Hallucinated glyphs come from
 * the supplied noise, so this function neither rolls dice nor changes state.
 *
 * Any lighting effects are also applied to these pairs, clear monsters allow
 * the underlying colour or feature to show through (ATTR_CLEAR and
 * CHAR_CLEAR), multi-hued colour-changing (ATTR_MULTI) is applied, and so on.
 * Technically, the flag "CHAR_MULTI" is supposed to indicate that a monster
 * looks strange when examined, but this flag is currently ignored.
 *
 * NOTES:
 * This is called pretty frequently, whenever a grid on the map display
 * needs updating, so don't overcomplicate it.
 *
 * The "zero" entry in the feature/object/monster arrays are
 * used to provide "special" attr/char codes, with "monster zero" being
 * used for the player attr/char, "object zero" being used for the "pile"
 * attr/char, and "feature zero" being used for the "darkness" attr/char.
 *
 * TODO:
 * The transformations for tile colors, or brightness for the 16x16
 * tiles should be handled differently.  One possibility would be to
 * extend feature_type with attr/char definitions for the different states.
 * This will probably be done outside of the current text->graphics mappings
 * though.
 */
void map_present(const struct frontend_grid_data *g, const struct map_noise *noise,
	struct map_presentation *result)
{
	struct feature *feat = &f_info[g->f_idx];
	struct grid_layer *layers = result->layers;
	int *ap = &result->top.attr, *tap = &layers[MAP_TERRAIN].attr;
	wchar_t *cp = &result->top.chr, *tcp = &layers[MAP_TERRAIN].chr;

	int a = feat_x_attr[g->lighting][feat->fidx];
	wchar_t c = feat_x_char[g->lighting][feat->fidx];
	bool skip_objects = false;

	memset(result, 0, sizeof(*result));
	/* Get the colour for ASCII */
	if (use_graphics == GRAPHICS_NONE)
		grid_get_attr(g, &a);

	/* Save the terrain info for the transparency effects */
	(*tap) = a;
	(*tcp) = c;

	/* There is a trap in this grid, and we are not hallucinating */
	if (g->trap && (!g->hallucinate)) {
	    /* Change graphics to indicate visible traps, skip objects if a web */
	    skip_objects = get_trap_graphics(cave, g, &a, &c);
	}

	if (layers && g->trap && !g->hallucinate) {
		layers[MAP_TRAP].attr = a;
		layers[MAP_TRAP].chr = c;
	}

	if (!skip_objects) {
		/* If there's an object, deal with that. */
		if (g->unseen_money) {

			/* $$$ gets an orange star*/
			a = object_kind_attr(unknown_gold_kind);
			c = object_kind_char(unknown_gold_kind);

		} else if (g->unseen_object) {

			/* Everything else gets a red star */
			a = object_kind_attr(unknown_item_kind);
			c = object_kind_char(unknown_item_kind);

		} else if (g->first_kind) {
			if (g->hallucinate) {
				/* Just pick a random object to display. */
				a = noise->object.attr;
				c = noise->object.chr;
			} else if (g->multiple_objects) {
				/* Get the "pile" feature instead */
				a = object_kind_attr(pile_kind);
				c = object_kind_char(pile_kind);
			} else {
				/* Normal attr and char */
				a = object_kind_attr(g->first_kind);
				c = object_kind_char(g->first_kind);
			}
		}
	}

	if (layers && !skip_objects &&
			(g->first_kind || g->unseen_money || g->unseen_object)) {
		layers[MAP_OBJECT].attr = a;
		layers[MAP_OBJECT].chr = c;
	}

	/* Handle monsters, the player and trap borders */
	if (g->m_idx > 0) {
		if (g->hallucinate) {
			/* Just pick a random monster to display. */
			a = noise->actor.attr;
			c = noise->actor.chr;
		} else if (!monster_is_camouflaged(cave_monster(cave, g->m_idx)))	{
			struct monster *mon = cave_monster(cave, g->m_idx);

			uint8_t da;
			wchar_t dc;

			/* Desired attr & char */
			da = monster_x_attr[mon->race->ridx];
			dc = monster_x_char[mon->race->ridx];

			/* Special handling of attrs and/or chars */
			if (da & 0x80) {
				/* Special attr/char codes */
				a = da;
				c = dc;
			} else if (OPT(player, purple_uniques) &&
					monster_is_shape_unique(mon)) {
				/* Turn uniques purple if desired (violet, actually) */
				a = COLOUR_VIOLET;
				c = dc;
			} else if (rf_has(mon->race->flags, RF_ATTR_MULTI) ||
					   rf_has(mon->race->flags, RF_ATTR_FLICKER) ||
					   rf_has(mon->race->flags, RF_ATTR_RAND)) {
				/* Multi-hued monster */
				a = mon->attr ? mon->attr : da;
				c = dc;
			} else if (!flags_test(mon->race->flags, RF_SIZE,
								   RF_ATTR_CLEAR, RF_CHAR_CLEAR, FLAG_END)) {
				/* Normal monster (not "clear" in any way) */
				a = da;
				/* Desired attr & char. da is not used, should a be set to it?*/
				/*da = monster_x_attr[mon->race->ridx];*/
				dc = monster_x_char[mon->race->ridx];
				c = dc;
			} else if (a & 0x80) {
				/* Bizarre grid under monster */
				a = da;
				c = dc;
			} else if (!rf_has(mon->race->flags, RF_CHAR_CLEAR)) {
				/* Normal char, Clear attr, monster */
				c = dc;
			} else if (!rf_has(mon->race->flags, RF_ATTR_CLEAR)) {
				/* Normal attr, Clear char, monster */
				a = da;
			}

		}
	} else if (g->is_player) {
		struct monster_race *race = &r_info[0];

		/* Get the "player" attr */
		a = monster_x_attr[race->ridx];
		if ((OPT(player, hp_changes_color)) && !(a & 0x80)) {
			switch(player->chp * 10 / player->mhp)
			{
			case 10:
			case  9:
			{
				a = COLOUR_WHITE;
				break;
			}
			case  8:
			case  7:
			{
				a = COLOUR_YELLOW;
				break;
			}
			case  6:
			case  5:
			{
				a = COLOUR_ORANGE;
				break;
			}
			case  4:
			case  3:
			{
				a = COLOUR_L_RED;
				break;
			}
			case  2:
			case  1:
			case  0:
			{
				a = COLOUR_RED;
				break;
			}
			default:
			{
				a = COLOUR_WHITE;
				break;
			}
			}
		}

		/* Get the "player" char */
		c = monster_x_char[race->ridx];
	}

	if (layers) {
		layers[MAP_TERRAIN].attr = *tap;
		layers[MAP_TERRAIN].chr = *tcp;
		if (g->is_player || (g->m_idx && (g->hallucinate ||
				!monster_is_camouflaged(cave_monster(cave, g->m_idx))))) {
			layers[MAP_ACTOR].attr = a;
			layers[MAP_ACTOR].chr = c;
		}
	}
	/* Result */
	(*ap) = a;
	(*cp) = c;
}

