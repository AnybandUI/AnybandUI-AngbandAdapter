/**
 * \file z-textblock.h
 * \brief Text output bugger (?NRM) code
 *
 * Copyright (c) 2010 Andi Sidwell
 * Copyright (c) 2011 Peter Denison
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

#ifndef INCLUDED_Z_TEXTBLOCK_H
#define INCLUDED_Z_TEXTBLOCK_H

#include "z-file.h"

/** Opaque text_block type */
typedef struct textblock textblock;

/* Optional structure over the same text and colours used by terminal output.
 * Strings and spans are owned by the block; appending may invalidate pointers. */
struct textblock_span {
	const char *section, *label, *value;
	size_t start, length;
	uint8_t attr;
};
void textblock_end_section(textblock *tb, const char *section);
void textblock_append_field(textblock *tb, const char *section,
	const char *label, const char *value, uint8_t attr);
const struct textblock_span *textblock_spans(const textblock *tb, size_t *count);


__declspec(dllimport) textblock *textblock_new(void);
void textblock_free(textblock *tb);


__declspec(dllimport) void textblock_append(textblock *tb, const char *fmt, ...)
	ATTRIBUTE ((format (printf, 2, 3)));
__declspec(dllimport) void textblock_append_c(textblock *tb, uint8_t attr, const char *fmt, ...)
	ATTRIBUTE ((format (printf, 3, 4)));
__declspec(dllimport) void textblock_append_pict(textblock *tb, uint8_t attr, int c);
void textblock_append_textblock(textblock *tb, const textblock *tba);

__declspec(dllimport) const wchar_t *textblock_text(textblock *tb);
__declspec(dllimport) const uint8_t *textblock_attrs(textblock *tb);

__declspec(dllimport) size_t textblock_calculate_lines(textblock *tb, size_t **line_starts,
								 size_t **line_lengths, size_t width);

__declspec(dllimport) void textblock_to_file(textblock *tb, ang_file *f, int indent, int wrap_at);

extern __declspec(dllimport) ang_file *text_out_file;
extern __declspec(dllimport) void (*text_out_hook)(uint8_t a, const char *str);
extern __declspec(dllimport) int text_out_wrap;
extern __declspec(dllimport) int text_out_indent;
extern __declspec(dllimport) int text_out_pad;

__declspec(dllimport) extern void text_out_to_file(uint8_t attr, const char *str);
__declspec(dllimport) extern void text_out(const char *fmt, ...)
	ATTRIBUTE ((format (printf, 1, 2)));
__declspec(dllimport) extern void text_out_c(uint8_t a, const char *fmt, ...)
	ATTRIBUTE ((format (printf, 2, 3)));
__declspec(dllimport) extern void text_out_e(const char *fmt, ...)
	ATTRIBUTE ((format (printf, 1, 2)));

typedef void (*text_writer)(ang_file *f);
__declspec(dllimport) errr text_lines_to_file(const char *path, text_writer writer);

#endif /* INCLUDED_Z_TEXTBLOCK_H */


