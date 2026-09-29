/**
 * \file obj-util.h
 * \brief Object utilities
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

#ifndef OBJECT_UTIL_H
#define OBJECT_UTIL_H

/* Maximum number of scroll titles generated */
#define MAX_TITLES	 50

/* An item's pval (for charges, amount of gold, etc) is limited to int16_t */
#define MAX_PVAL  32767

struct player;

__declspec(dllimport) void flavor_init(void);
__declspec(dllimport) void flavor_set_all_aware(void);
__declspec(dllimport) int16_t object_weight_one(const struct object *obj);
__declspec(dllimport) int object_to_hit(const struct object *obj);
__declspec(dllimport) int object_to_dam(const struct object *obj);
__declspec(dllimport) int object_to_ac(const struct object *obj);
__declspec(dllimport) void object_flags(const struct object *obj, bitflag flags[OF_SIZE]);
__declspec(dllimport) void object_flags_known(const struct object *obj, bitflag flags[OF_SIZE]);
__declspec(dllimport) bool object_test(item_tester tester, const struct object *o);
bool item_test(item_tester tester, int item);
__declspec(dllimport) bool is_unknown(const struct object *obj);
__declspec(dllimport) unsigned check_for_inscrip(const struct object *obj, const char *inscrip);
__declspec(dllimport) unsigned check_for_inscrip_with_int(const struct object *obj, const char *insrip, int *ival);
__declspec(dllimport) struct object_kind *lookup_kind(int tval, int sval);
__declspec(dllimport) struct object_kind *objkind_byid(int kidx);
__declspec(dllimport) const struct artifact *lookup_artifact_name(const char *name);
__declspec(dllimport) struct ego_item *lookup_ego_item(const char *name, int tval, int sval);
__declspec(dllimport) int lookup_sval(int tval, const char *name);
__declspec(dllimport) void object_short_name(char *buf, size_t max, const char *name);
__declspec(dllimport) int compare_items(const struct object *o1, const struct object *o2);
__declspec(dllimport) uint8_t convert_depth_to_origin(int depth);
__declspec(dllimport) bool obj_has_charges(const struct object *obj);
__declspec(dllimport) bool obj_can_zap(const struct object *obj);
__declspec(dllimport) bool obj_is_activatable(const struct object *obj);
__declspec(dllimport) bool obj_can_activate(const struct object *obj);
__declspec(dllimport) bool obj_can_refill(const struct object *obj);
__declspec(dllimport) bool obj_kind_can_browse(const struct object_kind *kind);
__declspec(dllimport) bool obj_can_browse(const struct object *obj);
__declspec(dllimport) bool obj_can_cast_from(const struct object *obj);
__declspec(dllimport) bool obj_can_study(const struct object *obj);
__declspec(dllimport) bool obj_can_takeoff(const struct object *obj);
__declspec(dllimport) bool obj_can_throw(const struct object *obj);
__declspec(dllimport) bool obj_can_wear(const struct object *obj);
__declspec(dllimport) bool obj_can_fire(const struct object *obj);
__declspec(dllimport) bool obj_is_throwing(const struct object *obj);
__declspec(dllimport) bool obj_is_known_artifact(const struct object *obj);
__declspec(dllimport) bool obj_has_inscrip(const struct object *obj);
__declspec(dllimport) bool obj_has_flag(const struct object *obj, int flag);
__declspec(dllimport) bool obj_is_useable(const struct object *obj);
__declspec(dllimport) struct effect *object_effect(const struct object *obj);
__declspec(dllimport) bool obj_needs_aim(const struct object *obj);
__declspec(dllimport) bool obj_can_fail(const struct object *o);

__declspec(dllimport) int get_use_device_chance(const struct object *obj);
__declspec(dllimport) void distribute_charges(struct object *source, struct object *dest, int amt,
		bool dest_new);
__declspec(dllimport) int number_charging(const struct object *obj);
__declspec(dllimport) bool recharge_timeout(struct object *obj);
__declspec(dllimport) bool verify_object(const char *prompt, const struct object *obj,
		const struct player *p);
__declspec(dllimport) void print_custom_message(const struct object *obj, const char *string,
		int msg_type, const struct player *p);

__declspec(dllimport) bool is_artifact_created(const struct artifact *art);
__declspec(dllimport) bool is_artifact_seen(const struct artifact *art);
__declspec(dllimport) bool is_artifact_everseen(const struct artifact *art);
__declspec(dllimport) void mark_artifact_created(const struct artifact *art, bool created);
__declspec(dllimport) void mark_artifact_seen(const struct artifact *art, bool seen);
__declspec(dllimport) void mark_artifact_everseen(const struct artifact *art, bool seen);

#endif /* OBJECT_UTIL_H */


