/**
 * \file obj-pile.h
 * \brief Deal with piles of objects
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

#include "cave.h"
#include "player.h"

#define OBJECT_LIST_SIZE  128
#define OBJECT_LIST_INCR  128

/**
 * Modes for stacking by object_similar()/object_stackable()/object_mergeable()
 */
typedef enum
{
	OSTACK_NONE    = 0x00, /* No options (this does NOT mean no stacking) */
	OSTACK_STORE   = 0x01, /* Store stacking */
	OSTACK_PACK    = 0x02, /* Inventory and home */
	OSTACK_LIST    = 0x04, /* Object list */
	OSTACK_MONSTER = 0x08, /* Monster carrying objects */
	OSTACK_FLOOR   = 0x10, /* Floor stacking */
	OSTACK_QUIVER  = 0x20  /* Quiver */
} object_stack_t;

/**
 * Modes for floor scanning by scan_floor()
 */
typedef enum
{
	OFLOOR_NONE    = 0x00, /* No options */
	OFLOOR_TEST    = 0x01, /* Verify item tester */
	OFLOOR_SENSE   = 0x02, /* Sensed or known items only */
	OFLOOR_TOP     = 0x04, /* Only the top item */
	OFLOOR_VISIBLE = 0x08, /* Visible items only */
} object_floor_t;

__declspec(dllimport) struct object *object_new(void);
__declspec(dllimport) void object_free(struct object *obj);
__declspec(dllimport) void object_delete(struct chunk *c, struct chunk *p_c,
				   struct object **obj_address);
__declspec(dllimport) void object_pile_free(struct chunk *c, struct chunk *p_c, struct object *obj);

__declspec(dllimport) void pile_insert(struct object **pile, struct object *obj);
__declspec(dllimport) void pile_insert_end(struct object **pile, struct object *obj);
__declspec(dllimport) void pile_excise(struct object **pile, struct object *obj);
__declspec(dllimport) struct object *pile_last_item(struct object *const pile);
__declspec(dllimport) bool pile_contains(const struct object *top, const struct object *obj);

__declspec(dllimport) bool object_similar(const struct object *obj1, const struct object *obj2,
	object_stack_t mode);
__declspec(dllimport) bool object_stackable(const struct object *obj1, const struct object *obj2,
					  object_stack_t mode);
__declspec(dllimport) bool object_mergeable(const struct object *obj1, const struct object *obj2,
					object_stack_t mode);
__declspec(dllimport) void object_origin_combine(struct object *obj1, const struct object *obj2);
__declspec(dllimport) void object_absorb_partial(struct object *obj1, struct object *obj2,
	object_stack_t mode1, object_stack_t mode2);
__declspec(dllimport) void object_absorb(struct object *obj1, struct object *obj2);
__declspec(dllimport) void object_wipe(struct object *obj);
__declspec(dllimport) void object_copy(struct object *obj1, const struct object *obj2);
__declspec(dllimport) void object_copy_amt(struct object *dest, struct object *src, int amt);
__declspec(dllimport) struct object *object_split(struct object *src, int amt);
__declspec(dllimport) struct object *floor_object_for_use(struct player *p, struct object *obj,
	int num, bool message, bool *none_left);
__declspec(dllimport) bool floor_carry(struct chunk *c, struct loc grid, struct object *drop,
				 bool *note);
__declspec(dllimport) void drop_near(struct chunk *c, struct object **dropped, int chance,
			   struct loc grid, bool verbose, bool prefer_pile);
__declspec(dllimport) void push_object(struct loc grid);
__declspec(dllimport) void floor_item_charges(struct object *obj);
__declspec(dllimport) int scan_floor(struct object **items, int max_size, struct player *p,
		object_floor_t mode, item_tester tester);
__declspec(dllimport) int scan_distant_floor(struct object **items, int max_size, struct player *p,
		struct loc grid);
__declspec(dllimport) int scan_items(struct object **item_list, size_t item_list_max,
		struct player *p, int mode, item_tester tester);
__declspec(dllimport) bool item_is_available(struct object *obj);


