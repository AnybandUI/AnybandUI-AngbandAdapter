/**
 * \file target.h
 * \brief Targetting code
 *
 * Copyright (c) 1997-2007 Angband contributors
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

#ifndef TARGET_H
#define TARGET_H

#include "mon-predicate.h"

/**
 * Bit flags for target_set()
 *
 *	KILL: Target monsters
 *	LOOK: Describe grid fully
 *	XTRA: Currently unused flag (NOT USED)
 *	GRID: Select from all grids (NOT USED)
 * QUIET: Prevent targeting messages.
 */
#define TARGET_KILL   0x01
#define TARGET_LOOK   0x02
#define TARGET_XTRA   0x04
#define TARGET_GRID   0x08
#define TARGET_QUIET  0x10

struct target {
	struct loc grid;
	int midx;
};

__declspec(dllimport) void look_mon_desc(char *buf, size_t max, int m_idx);
__declspec(dllimport) bool target_able(struct monster *m);
__declspec(dllimport) bool target_okay(void);
__declspec(dllimport) bool target_set_monster(struct monster *mon);
__declspec(dllimport) void target_set_location(int y, int x);
__declspec(dllimport) bool target_is_set(void);
__declspec(dllimport) void target_fix(void);
__declspec(dllimport) void target_release(void);
__declspec(dllimport) int cmp_distance(const void *a, const void *b);
__declspec(dllimport) int16_t target_pick(int y1, int x1, int dy, int dx, struct point_set *targets);
__declspec(dllimport) bool target_accept(int y, int x);
__declspec(dllimport) void coords_desc(char *buf, int size, int y, int x);
__declspec(dllimport) void target_get(struct loc *grid);
__declspec(dllimport) struct monster *target_get_monster(void);
__declspec(dllimport) bool target_sighted(void);
__declspec(dllimport) struct point_set *target_get_monsters(int mode, monster_predicate pred,
	bool restrict_to_panel);
__declspec(dllimport) bool target_set_closest(int mode, monster_predicate pred);

#endif /* !TARGET_H */


