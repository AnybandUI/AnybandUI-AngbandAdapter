/**
 * \file cave-map.c
 * \brief Lighting and map management functions
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
#include "init.h"
#include "monster.h"
#include "mon-predicate.h"
#include "mon-util.h"
#include "obj-ignore.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "trap.h"


static void map_info_internal(struct loc grid, struct frontend_grid_data *g,
                              bool readonly)
{
	struct object *obj;

	assert(grid.x < cave->width);
	assert(grid.y < cave->height);

	/* Default "clear" values, others will be set later where appropriate. */
	g->grid = grid;
	g->first_kind = NULL;
	g->trap = NULL;
	g->multiple_objects = false;
	g->lighting = LIGHTING_LIT;
	g->unseen_object = false;
	g->unseen_money = false;

	/* Use real feature (remove later) */
	g->f_idx = square(cave, grid)->feat;
	if (f_info[g->f_idx].mimic)
		g->f_idx = (uint32_t) (f_info[g->f_idx].mimic - f_info);

	g->in_view = (square_isseen(cave, grid)) ? true : false;
	g->is_player = (square(cave, grid)->mon < 0) ? true : false;
	g->m_idx = (g->is_player) ? 0 : square(cave, grid)->mon;
	g->hallucinate = player->timed[TMD_IMAGE] ? true : false;

	if (g->in_view) {
		bool lit = square_islit(cave, grid);

		if (sqinfo_has(square(cave, grid)->info, SQUARE_CLOSE_PLAYER)) {
			if (player_has(player, PF_UNLIGHT) &&
					player->state.cur_light <= 1) {
				g->lighting = (lit) ?
					LIGHTING_LOS : LIGHTING_DARK;
			} else if (lit) {
				g->lighting = (OPT(player, view_yellow_light)) ?
					LIGHTING_TORCH : LIGHTING_LOS;
			}
		} else if (lit) {
			g->lighting = LIGHTING_LOS;
		}

		/* Remember seen feature */
		if (!readonly) square_memorize(cave, grid);
	} else if (!square_isknown(cave, grid)) {
		g->f_idx = FEAT_NONE;
	} else if (square_isglow(cave, grid)) {
		g->lighting = LIGHTING_LIT;
	}

	/* Use known feature */
	g->f_idx = square(player->cave, grid)->feat;
	if (f_info[g->f_idx].mimic)
		g->f_idx = (uint32_t) (f_info[g->f_idx].mimic - f_info);

	/* There is a known trap in this square */
	if (square_trap(player->cave, grid) && square_isknown(cave, grid)) {
		struct trap *trap = square(player->cave, grid)->trap;

		/* Scan the square trap list */
		while (trap) {
			if (trf_has(trap->flags, TRF_TRAP) ||
				trf_has(trap->flags, TRF_GLYPH) ||
				trf_has(trap->flags, TRF_WEB)) {
				/* Accept the trap - only if not disabled, maybe we need
				 * a special graphic for this */
				if (!trap->timeout) {
					g->trap = trap;
					break;
				}
			}
			trap = trap->next;
		}
	}

	/* Objects */
	for (obj = square_object(player->cave, grid); obj; obj = obj->next) {
		if (obj->kind == unknown_gold_kind) {
			g->unseen_money = true;
		} else if (obj->kind == unknown_item_kind) {
			g->unseen_object = true;
		} else if (ignore_known_item_ok(player, obj)) {
			/* Item stays hidden */
		} else if (!g->first_kind) {
			g->first_kind = obj->kind;
		} else {
			g->multiple_objects = true;
			break;
		}
	}

	/* Monsters */
	if (g->m_idx > 0) {
		/* If the monster isn't "visible", make sure we don't list it.*/
		struct monster *mon = cave_monster(cave, g->m_idx);
		if (!monster_is_visible(mon)) g->m_idx = 0;
	}

	/* Rare random hallucination on non-outer walls */
	if (g->hallucinate && g->m_idx == 0 && g->first_kind == 0 && !readonly) {
		if (one_in_(128) && (int) g->f_idx != FEAT_PERM)
			g->m_idx = 1;
		else if (one_in_(128) && (int) g->f_idx != FEAT_PERM)
			/* if hallucinating, we just need first_kind to not be NULL */
			g->first_kind = k_info;
		else
			g->hallucinate = false;
	}

	assert((int) g->f_idx < FEAT_MAX);
	if (!g->hallucinate)
		assert((int)g->m_idx < cave->mon_max);
	/* All other g fields are 'flags', mostly booleans. */
}

void map_info(struct loc grid, struct frontend_grid_data *g)
{
	map_info_internal(grid, g, false);
}

/* Presentation only: do not memorize terrain or consume gameplay randomness. */
void map_info_readonly(struct loc grid, struct frontend_grid_data *g)
{
	map_info_internal(grid, g, true);
}




