/**
 * \file ui-player.h
 * \brief character info
 */

#ifndef UI_PLAYER_H
#define UI_PLAYER_H

#include "z-textblock.h"

/* Read-only structured document, also consumed by the character screen. */
textblock *character_sheet(void);
void display_player_stat_info(void);
void display_player_xtra_info(void);
void display_player(int mode);
void write_character_dump(ang_file *fff);
bool dump_save(const char *path);
void do_cmd_change_name(void);

#endif /* !UI_PLAYER_H */


