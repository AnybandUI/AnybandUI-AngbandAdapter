/**
 * \file effects.h
 * \brief effect handling
 *
 * Copyright (c) 2007 Andi Sidwell
 * Copyright (c) 2014 Ben Semmler, Nick McConnell
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
#ifndef INCLUDED_EFFECTS_H
#define INCLUDED_EFFECTS_H

#include "source.h"
#include "object.h"
#include "player-attack.h"
#include "cmds.h"


/* Types of effect */
typedef enum
{
	EF_NONE,
	#define EFFECT(x, a, b, c, d, e, f)	EF_##x,
	#include "list-effects.h"
	#undef EFFECT
	EF_MAX
} effect_index;

/*** Functions ***/

__declspec(dllimport) void free_effect(struct effect *source);
__declspec(dllimport) bool effect_valid(const struct effect *effect);
__declspec(dllimport) bool effect_aim(const struct effect *effect);
__declspec(dllimport) const char *effect_info(const struct effect *effect);
__declspec(dllimport) const char *effect_desc(const struct effect *effect);
__declspec(dllimport) effect_index effect_lookup(const char *name);
__declspec(dllimport) int effect_subtype(int index, const char *type);
__declspec(dllimport) extern expression_base_value_f effect_value_base_by_name(const char *name);
__declspec(dllimport) bool effect_do(struct effect *effect,
	struct source origin,
	struct object *obj,
	bool *ident,
	bool aware,
	int dir,
	int beam,
	int boost,
	struct command *cmd);
__declspec(dllimport) void effect_simple(int index,
	struct source origin,
	const char *dice_string,
	int subtype,
	int radius,
	int other,
	int y,
	int x,
	bool *ident);
__declspec(dllimport) int recharge_failure_chance(const struct object *obj, int strength);

#endif /* INCLUDED_EFFECTS_H */


