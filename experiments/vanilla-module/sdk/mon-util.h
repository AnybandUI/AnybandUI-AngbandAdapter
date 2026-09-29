/**
 * \file mon-util.h
 * \brief Functions for monster utilities.
 *
 * Copyright (c) 1997-2007 Ben Harrison, James E. Wilson, Robert A. Koeneke
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

#ifndef MONSTER_UTILITIES_H
#define MONSTER_UTILITIES_H

#include "monster.h"
#include "mon-msg.h"

__declspec(dllimport) const char *describe_race_flag(int flag);
__declspec(dllimport) void create_mon_flag_mask(bitflag *f, ...);
__declspec(dllimport) struct monster_race *lookup_monster(const char *name);
__declspec(dllimport) struct monster_base *lookup_monster_base(const char *name);
__declspec(dllimport) bool match_monster_bases(const struct monster_base *base, ...);
__declspec(dllimport) void update_mon(struct monster *mon, struct chunk *c, bool full);
__declspec(dllimport) void update_monsters(bool full);
__declspec(dllimport) bool monster_carry(struct chunk *c, struct monster *mon, struct object *obj);
__declspec(dllimport) void monster_swap(struct loc grid1, struct loc grid2);
__declspec(dllimport) void monster_wake(struct monster *mon, bool notify, int aware_chance);
__declspec(dllimport) bool monster_can_see(struct chunk *c, struct monster *mon, struct loc grid);
__declspec(dllimport) void become_aware(struct chunk *c, struct monster *m);
__declspec(dllimport) void update_smart_learn(struct monster *mon, struct player *p, int flag,
						int pflag, int element);
__declspec(dllimport) bool find_any_nearby_injured_kin(struct chunk *c, const struct monster *mon);
__declspec(dllimport) struct monster *choose_nearby_injured_kin(struct chunk *c, const struct monster *mon);
__declspec(dllimport) void monster_death(struct monster *mon, struct player *p, bool stats);
__declspec(dllimport) bool mon_take_nonplayer_hit(int dam, struct monster *t_mon,
							enum mon_messages hurt_msg,
							enum mon_messages die_msg);
__declspec(dllimport) bool mon_take_hit(struct monster *mon, struct player *p, int dam, bool *fear,
	const char *note);
__declspec(dllimport) void kill_arena_monster(struct monster *mon);
__declspec(dllimport) void monster_take_terrain_damage(struct monster *mon);
__declspec(dllimport) bool monster_taking_terrain_damage(struct chunk *c, struct monster *mon);
__declspec(dllimport) struct monster *get_commanded_monster(void);
__declspec(dllimport) struct object *get_random_monster_object(struct monster *mon);
__declspec(dllimport) void steal_monster_item(struct monster *mon, int midx);
__declspec(dllimport) bool monster_change_shape(struct monster *mon);
__declspec(dllimport) bool monster_revert_shape(struct monster *mon);

#endif /* MONSTER_UTILITIES_H */


