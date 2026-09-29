/**
 * \file: obj-gear.h
 * \brief management of inventory, equipment and quiver
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2014 Nick McConnell
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

#ifndef OBJECT_GEAR_H
#define OBJECT_GEAR_H

#include "player.h"

/**
 * Player equipment slot types
 */
enum
{
	#define EQUIP(a, b, c, d, e, f) EQUIP_##a,
	#include "list-equip-slots.h"
	#undef EQUIP
	EQUIP_MAX
};

__declspec(dllimport) int slot_by_name(struct player *p, const char *name);
__declspec(dllimport) bool slot_type_is(struct player *p, int slot, int type);
__declspec(dllimport) struct object *slot_object(struct player *p, int slot);
__declspec(dllimport) struct object *equipped_item_by_slot_name(struct player *p, const char *name);
__declspec(dllimport) int object_slot(struct player_body body, const struct object *obj);
__declspec(dllimport) bool object_is_equipped(struct player_body body, const struct object *obj);
__declspec(dllimport) bool object_is_carried(struct player *p, const struct object *obj);
__declspec(dllimport) bool object_is_in_quiver(struct player *p, const struct object *obj);
__declspec(dllimport) uint16_t object_pack_total(struct player *p, const struct object *obj,
	bool ignore_inscrip, struct object **first);
__declspec(dllimport) int pack_slots_used(const struct player *p);
__declspec(dllimport) const char *equip_mention(struct player *p, int slot);
__declspec(dllimport) const char *equip_describe(struct player *p, int slot);
__declspec(dllimport) int wield_slot(const struct object *obj);
__declspec(dllimport) bool minus_ac(struct player *p);
__declspec(dllimport) char gear_to_label(struct player *p, struct object *obj);
__declspec(dllimport) struct object *gear_last_item(struct player *p);
__declspec(dllimport) void gear_insert_end(struct player *p, struct object *obj);
__declspec(dllimport) struct object *gear_object_for_use(struct player *p, struct object *obj,
	int num, bool message, bool *none_left);
__declspec(dllimport) int inven_carry_num(const struct player *p, const struct object *obj);
__declspec(dllimport) bool inven_carry_okay(const struct object *obj);
__declspec(dllimport) void inven_item_charges(struct object *obj);
__declspec(dllimport) void inven_carry(struct player *p, struct object *obj, bool absorb,
				 bool message);
__declspec(dllimport) void inven_wield(struct object *obj, int slot);
__declspec(dllimport) void inven_takeoff(struct object *item);
__declspec(dllimport) void inven_drop(struct object *obj, int amt);
__declspec(dllimport) void combine_pack(struct player *p);
__declspec(dllimport) bool pack_is_full(void);
__declspec(dllimport) bool pack_is_overfull(void);
__declspec(dllimport) void pack_overflow(struct object *obj);
__declspec(dllimport) int preferred_quiver_slot(const struct object *obj);


#endif /* OBJECT_GEAR_H */


