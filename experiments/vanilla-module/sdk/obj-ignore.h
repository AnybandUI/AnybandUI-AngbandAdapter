/**
 * \file obj-ignore.h
 * \brief Item ignoring
 *
 * Copyright (c) 2007 David T. Blackston, Iain McFall, DarkGod, Jeff Greene,
 * David Vestal, Pete Mack, Andi Sidwell.
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
#ifndef OBJ_IGNORE_H
#define OBJ_IGNORE_H

struct player;

/*
 * Used for mapping the values below to names.
 */
typedef struct
{
	unsigned int enum_val;
	const char *name;
} quality_name_struct;

/*
 * List of kinds of item, for pseudo-id and ego ignore.
 */
typedef enum
{
	#define ITYPE(a, b) ITYPE_##a,
	#include "list-ignore-types.h"
	#undef ITYPE

	ITYPE_MAX
} ignore_type_t;


#define ITYPE_SIZE              FLAG_SIZE(ITYPE_MAX)

#define itype_has(f, flag)        	flag_has_dbg(f, ITYPE_SIZE, flag, #f, #flag)
#define itype_on(f, flag)         	flag_on_dbg(f, ITYPE_SIZE, flag, #f, #flag)
#define itype_wipe(f)             	flag_wipe(f, ITYPE_SIZE)

/*
 * The different kinds of quality ignore
 */
enum
{
	IGNORE_NONE,
	IGNORE_BAD,
	IGNORE_AVERAGE,
	IGNORE_GOOD,
	IGNORE_ALL,

	IGNORE_MAX
};


/**
 * Structure to describe ego item short name. 
 */
struct ego_desc {
	int16_t e_idx;
	uint16_t itype;
	const char *short_name;
};

/*
 * Ignore flags
 */
#define IGNORE_IF_AWARE	0x01
#define IGNORE_IF_UNAWARE	0x02



extern __declspec(dllimport) quality_name_struct quality_values[IGNORE_MAX];
extern __declspec(dllimport) quality_name_struct quality_choices[ITYPE_MAX];
extern __declspec(dllimport) bool **ego_ignore_types;


/* obj-ignore.c */
__declspec(dllimport) void ignore_birth_init(void);
__declspec(dllimport) void rune_autoinscribe(struct player *p, int i);
__declspec(dllimport) const char *get_autoinscription(struct object_kind *kind, bool aware);
__declspec(dllimport) int apply_autoinscription(struct player *p, struct object *obj);
__declspec(dllimport) int remove_autoinscription(int16_t kind);
__declspec(dllimport) int add_autoinscription(int16_t kind, const char *inscription, bool aware);
__declspec(dllimport) void autoinscribe_ground(struct player *p);
__declspec(dllimport) void autoinscribe_pack(struct player *p);
__declspec(dllimport) void object_ignore_flavor_of(const struct object *obj);
__declspec(dllimport) ignore_type_t ignore_type_of(const struct object *obj);
__declspec(dllimport) uint8_t ignore_level_of(const struct object *obj);
__declspec(dllimport) bool ego_has_ignore_type(struct ego_item *ego, ignore_type_t itype);
__declspec(dllimport) void kind_ignore_clear(struct object_kind *kind);
__declspec(dllimport) void ego_ignore(struct object *obj);
__declspec(dllimport) void ego_ignore_clear(struct object *obj);
__declspec(dllimport) void ego_ignore_toggle(int e_idx, int itype);
__declspec(dllimport) bool ego_is_ignored(int e_idx, int itype);
__declspec(dllimport) bool kind_is_ignored_aware(const struct object_kind *kind);
__declspec(dllimport) bool kind_is_ignored_unaware(const struct object_kind *kind);
__declspec(dllimport) void kind_ignore_when_aware(struct object_kind *kind);
__declspec(dllimport) void kind_ignore_when_unaware(struct object_kind *kind);
__declspec(dllimport) bool object_is_ignored(const struct object *obj);
__declspec(dllimport) bool ignore_item_ok(const struct player *p, const struct object *obj);
__declspec(dllimport) bool ignore_known_item_ok(const struct player *p, const struct object *obj);
__declspec(dllimport) void ignore_drop(struct player *p);
__declspec(dllimport) const char *ignore_name_for_type(ignore_type_t type);

extern __declspec(dllimport) uint8_t ignore_level[];
extern __declspec(dllimport) const size_t ignore_size;

#endif /* !OBJ_IGNORE_H */


