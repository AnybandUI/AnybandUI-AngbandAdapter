/**
 * \file obj-curse.h
 * \brief functions to deal with object curses
 *
 * Copyright (c) 2016 Nick McConnell
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
#ifndef INCLUDED_OBJ_CURSE_H
#define INCLUDED_OBJ_CURSE_H

#include "object.h"

extern __declspec(dllimport) struct curse *curses;

void init_curse_knowledge(void);
__declspec(dllimport) int lookup_curse(const char *name);
__declspec(dllimport) void copy_curses(struct object *obj, int *source);
__declspec(dllimport) bool curses_are_equal(const struct object *obj1, const struct object *obj2);
__declspec(dllimport) bool append_object_curse(struct object *obj, int pick, int power);
__declspec(dllimport) bool remove_object_curse(struct object *obj, int pick, bool message);
__declspec(dllimport) void check_artifact_curses(struct artifact *art);
__declspec(dllimport) bool artifact_curse_conflicts(struct artifact *art, int pick);
__declspec(dllimport) bool append_artifact_curse(struct artifact *art, int pick, int power);
__declspec(dllimport) bool do_curse_effect(int i, struct object *obj);
__declspec(dllimport) int16_t modify_weight_for_curse(int i, int16_t weight);
__declspec(dllimport) void apply_curse_attributes(int i, struct object *obj);

#endif /* !INCLUDED_OBJ_CURSE_H */


