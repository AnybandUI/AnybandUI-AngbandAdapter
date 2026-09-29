/**
 * \file game-world.h
 * \brief Game core management of the game world
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

#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "cave.h"

struct level {
	int depth;
	char *name;
	char *up;
	char *down;
	struct level *next;
};

extern __declspec(dllimport) uint16_t daycount;
extern __declspec(dllimport) uint32_t seed_randart;
extern __declspec(dllimport) uint32_t seed_flavor;
extern __declspec(dllimport) int32_t turn;
extern __declspec(dllimport) bool character_generated;
extern __declspec(dllimport) bool character_dungeon;
extern __declspec(dllimport) const uint8_t extract_energy[200];
extern __declspec(dllimport) struct level *world;

__declspec(dllimport) struct level *level_by_name(const char *name);
__declspec(dllimport) struct level *level_by_depth(int depth);
__declspec(dllimport) bool is_daytime(void);
__declspec(dllimport) int turn_energy(int speed);
__declspec(dllimport) void play_ambient_sound(void);
__declspec(dllimport) void process_world(struct chunk *c);
__declspec(dllimport) void on_new_level(void);
__declspec(dllimport) void process_player(void);
__declspec(dllimport) void run_game_loop(void);

#endif /* !GAME_WORLD_H */


