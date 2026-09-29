/**
 * \file message.h
 * \brief Message handling
 *
 * Copyright (c) 2007 Elly, Andi Sidwell
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef INCLUDED_Z_MSG_H
#define INCLUDED_Z_MSG_H

#include "h-basic.h"

/**
 * Message constants
 */
enum {
	#define MSG(x, s) MSG_##x,
	#include "list-message.h"
	#undef MSG
	SOUND_MAX = MSG_MAX,
};


/* Functions */
__declspec(dllimport) void messages_init(void);
__declspec(dllimport) void messages_free(void);
__declspec(dllimport) uint16_t messages_num(void);
__declspec(dllimport) void message_add(const char *str, uint16_t type);
__declspec(dllimport) const char *message_str(uint16_t age);
__declspec(dllimport) uint16_t message_count(uint16_t age);
__declspec(dllimport) uint16_t message_type(uint16_t age);
__declspec(dllimport) uint8_t message_color(uint16_t age);
__declspec(dllimport) uint8_t message_type_color(uint16_t type);
__declspec(dllimport) void message_color_define(uint16_t type, uint8_t color);
__declspec(dllimport) int message_lookup_by_name(const char *name);
__declspec(dllimport) int message_lookup_by_sound_name(const char *name);
__declspec(dllimport) const char *message_sound_name(int message);
__declspec(dllimport) void sound(int type);
__declspec(dllimport) void bell(void);
__declspec(dllimport) void msg(const char *fmt, ...);
__declspec(dllimport) void msgt(unsigned int type, const char *fmt, ...);


#endif /* !INCLUDED_Z_MSG_H */


