/**
 * \file savefile.h
 * \brief Savefile loading and saving main routines
 *
 * Copyright (c) 2009 Andi Sidwell <andi@takkaria.org>
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
#ifndef INCLUDED_SAVEFILE_H
#define INCLUDED_SAVEFILE_H

#define FINISHED_CODE 255
#define ITEM_VERSION	5
#define EGO_ART_KNOWN 0xffffffff

/**
 * ------------------------------------------------------------------------
 * Savefile API
 * ------------------------------------------------------------------------ */

/**
 * Global "we've just saved" variable
 */
extern __declspec(dllimport) bool character_saved;

/**
 * Save to the given location.  Returns true on success, false otherwise.
 */
__declspec(dllimport) bool savefile_save(const char *path);

/**
 * Load the savefile given.  Returns true on succcess, false otherwise.
 */
__declspec(dllimport) bool savefile_load(const char *path, bool cheat_death);

/**
 * Try to get a description for this savefile.
 */
__declspec(dllimport) const char *savefile_get_description(const char *path);

/**
 * Fill the given buffer with the panic save equivalent for a savefile.
 */
__declspec(dllimport) void savefile_get_panic_name(char *buffer, size_t len, const char *path);


/**
 * ------------------------------------------------------------------------
 * Detailed saving and loading functions
 * ------------------------------------------------------------------------ */


/* Utility */
__declspec(dllimport) void note(const char *msg);

/* Writing bits */
__declspec(dllimport) void wr_byte(uint8_t v);
__declspec(dllimport) void wr_u16b(uint16_t v);
__declspec(dllimport) void wr_s16b(int16_t v);
__declspec(dllimport) void wr_u32b(uint32_t v);
__declspec(dllimport) void wr_s32b(int32_t v);
__declspec(dllimport) void wr_string(const char *str);
__declspec(dllimport) void pad_bytes(int n);

/* Reading bits */
__declspec(dllimport) void rd_byte(uint8_t *ip);
__declspec(dllimport) void rd_u16b(uint16_t *ip);
__declspec(dllimport) void rd_s16b(int16_t *ip);
__declspec(dllimport) void rd_u32b(uint32_t *ip);
__declspec(dllimport) void rd_s32b(int32_t *ip);
__declspec(dllimport) void rd_string(char *str, int max);
__declspec(dllimport) void strip_bytes(int n);



/* load.c */
__declspec(dllimport) int rd_randomizer(void);
__declspec(dllimport) int rd_options(void);
__declspec(dllimport) int rd_messages(void);
__declspec(dllimport) int rd_monster_memory(void);
__declspec(dllimport) int rd_object_memory(void);
__declspec(dllimport) int rd_quests(void);
__declspec(dllimport) int rd_artifacts(void);
__declspec(dllimport) int rd_player(void);
__declspec(dllimport) int rd_ignore(void);
__declspec(dllimport) int rd_misc(void);
__declspec(dllimport) int rd_player_hp(void);
__declspec(dllimport) int rd_player_spells(void);
__declspec(dllimport) int rd_gear(void);
__declspec(dllimport) int rd_stores(void);
__declspec(dllimport) int rd_dungeon(void);
__declspec(dllimport) int rd_chunks(void);
__declspec(dllimport) int rd_objects(void);
__declspec(dllimport) int rd_monsters(void);
int rd_monster_groups(void);
__declspec(dllimport) int rd_history(void);
__declspec(dllimport) int rd_traps(void);
__declspec(dllimport) int rd_null(void);

/* save.c */
__declspec(dllimport) void wr_description(void);
__declspec(dllimport) void wr_randomizer(void);
__declspec(dllimport) void wr_options(void);
__declspec(dllimport) void wr_messages(void);
__declspec(dllimport) void wr_monster_memory(void);
__declspec(dllimport) void wr_object_memory(void);
__declspec(dllimport) void wr_quests(void);
__declspec(dllimport) void wr_artifacts(void);
__declspec(dllimport) void wr_player(void);
__declspec(dllimport) void wr_ignore(void);
__declspec(dllimport) void wr_misc(void);
__declspec(dllimport) void wr_player_hp(void);
__declspec(dllimport) void wr_player_spells(void);
void wr_randarts(void);
__declspec(dllimport) void wr_gear(void);
__declspec(dllimport) void wr_stores(void);
__declspec(dllimport) void wr_dungeon(void);
__declspec(dllimport) void wr_chunks(void);
__declspec(dllimport) void wr_objects(void);
__declspec(dllimport) void wr_monsters(void);
void wr_monster_groups(void);
void wr_ghost(void);
__declspec(dllimport) void wr_history(void);
__declspec(dllimport) void wr_traps(void);


#endif /* INCLUDED_SAVEFILE_H */


