/**
 * \file player-quest.h
 * \brief Quest-related variables and functions
 *
 * Copyright (c) 2013 Angband developers
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

#ifndef QUEST_H
#define QUEST_H

/* Quest list */
extern __declspec(dllimport) struct quest *quests;

/* Functions */
__declspec(dllimport) bool is_quest(struct player *p, int level);
__declspec(dllimport) void player_quests_reset(struct player *p);
__declspec(dllimport) void player_quests_free(struct player *p);
__declspec(dllimport) bool quest_check(struct player *p, const struct monster *m);
extern __declspec(dllimport) struct file_parser quests_parser;


#endif /* QUEST_H */


