/**
 * \file player-birth.h
 * \brief Character creation
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

#ifndef PLAYER_BIRTH_H
#define PLAYER_BIRTH_H

#include "cmd-core.h"

__declspec(dllimport) extern void player_init(struct player *p);
__declspec(dllimport) extern void player_generate(struct player *p, const struct player_race *r,
                            const struct player_class *c, bool old_history);
__declspec(dllimport) extern char *get_history(struct history_chart *h);
__declspec(dllimport) extern void wield_all(struct player *p);
__declspec(dllimport) extern bool player_make_simple(const char *nrace, const char *nclass,
	const char *nplayer);

__declspec(dllimport) void do_cmd_birth_init(struct command *cmd);
__declspec(dllimport) void do_cmd_birth_reset(struct command *cmd);
__declspec(dllimport) void do_cmd_choose_race(struct command *cmd);
__declspec(dllimport) void do_cmd_choose_class(struct command *cmd);
__declspec(dllimport) void do_cmd_buy_stat(struct command *cmd);
__declspec(dllimport) void do_cmd_sell_stat(struct command *cmd);
__declspec(dllimport) void do_cmd_reset_stats(struct command *cmd);
__declspec(dllimport) void do_cmd_refresh_stats(struct command *cmd);
__declspec(dllimport) void do_cmd_roll_stats(struct command *cmd);
__declspec(dllimport) void do_cmd_prev_stats(struct command *cmd);
__declspec(dllimport) void do_cmd_choose_name(struct command *cmd);
__declspec(dllimport) void do_cmd_choose_history(struct command *cmd);
__declspec(dllimport) void do_cmd_accept_character(struct command *cmd);

__declspec(dllimport) char *find_roman_suffix_start(const char *buf);

#endif /* !PLAYER_BIRTH_H */


