/**
 * \file player-spell.h
 * \brief Spell and prayer casting/praying
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

__declspec(dllimport) void player_spells_init(struct player *p);
__declspec(dllimport) void player_spells_free(struct player *p);
__declspec(dllimport) struct magic_realm *class_magic_realms(const struct player_class *c,
									   int *count);
__declspec(dllimport) const struct class_book *object_kind_to_book(const struct object_kind *kind);
__declspec(dllimport) const struct class_book *player_object_to_book(const struct player *p,
	const struct object *obj);
__declspec(dllimport) const struct class_spell *spell_by_index(const struct player *p, int index);
__declspec(dllimport) int spell_collect_from_book(const struct player *p, const struct object *obj,
	int **spells);
__declspec(dllimport) int spell_book_count_spells(const struct player *p, const struct object *obj,
	bool (*tester)(const struct player *p, int spell_index));
__declspec(dllimport) bool spell_okay_list(const struct player *p,
	bool (*spell_test)(const struct player *p, int spell_index),
	const int spells[], int n_spells);
__declspec(dllimport) bool spell_okay_to_cast(const struct player *p, int spell_index);
__declspec(dllimport) bool spell_okay_to_study(const struct player *p, int spell_index);
__declspec(dllimport) bool spell_okay_to_browse(const struct player *p, int spell_index);
__declspec(dllimport) int16_t spell_chance(int spell_index);
__declspec(dllimport) void spell_learn(int spell_index);
__declspec(dllimport) bool spell_cast(int spell_index, int dir, struct command *cmd);

__declspec(dllimport) extern void get_spell_info(int index, char *buf, size_t len);
extern bool cast_spell(int tval, int index, int dir);
__declspec(dllimport) extern bool spell_needs_aim(int spell_index);



