/**
 * \file z-util.h
 * \brief Low-level string handling and other utilities.
 *
 * Copyright (c) 1997-2005 Ben Harrison, Robert Ruehlmann.
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

#ifndef INCLUDED_Z_UTIL_H
#define INCLUDED_Z_UTIL_H

#include "h-basic.h"


struct my_rational { unsigned int n, d; }; /* numerator and denominator */


/**
 * ------------------------------------------------------------------------
 * Available variables
 * ------------------------------------------------------------------------ */


/**
 * The name of the program.
 */
extern __declspec(dllimport) char *argv0;


/**
 * Aux functions
 */
extern __declspec(dllimport) size_t (*text_mbcs_hook)(wchar_t *dest, const char *src, int n);
extern __declspec(dllimport) int (*text_wctomb_hook)(char *s, wchar_t wchar);
extern __declspec(dllimport) int (*text_wcsz_hook)(void);
extern __declspec(dllimport) int (*text_iswprint_hook)(wint_t wc);
extern __declspec(dllimport) wchar_t *(*text_wcschr_hook)(const wchar_t *wcs, wchar_t wc);
extern __declspec(dllimport) size_t (*text_wcslen_hook)(const wchar_t *s);
extern __declspec(dllimport) void (*plog_aux)(const char *);
extern __declspec(dllimport) void (*quit_aux)(const char *);


/**
 * ------------------------------------------------------------------------
 * Available Functions
 * ------------------------------------------------------------------------ */


/**
 * Return "s" (or not) depending on whether n is singular.
 */
#define PLURAL(n)		((n) == 1 ? "" : "s")

/**
 * Return the verb form matching the given count
 */
#define VERB_AGREEMENT(count, singular, plural)    (((count) == 1) ? (singular) : (plural))


/**
 * Count the number of characters in a UTF-8 encoded string
 */
__declspec(dllimport) size_t utf8_strlen(const char *s);

/**
 * Clip a null-terminated UTF-8 string 's' to 'n' unicode characters.
 * e.g. utf8_clipto("example", 4) will clip after 'm', resulting in 'exam'.
 */
__declspec(dllimport) void utf8_clipto(char *s, size_t n);

/**
 * Advance a pointer to a UTF-8 buffer by a given number of Unicode code points.
 */
__declspec(dllimport) char *utf8_fskip(char *s, size_t n, char *lim);

/**
 * Decrement a pointer to a UTF-8 buffer by a given number of Unicode code
 * points.
 */
__declspec(dllimport) char *utf8_rskip(char *s, size_t n, char *lim);

/**
 * Convert a sequence of UTF-32 values, in the native byte order, to UTF-8.
 */
__declspec(dllimport) size_t utf32_to_utf8(char *out, size_t n_out, const uint32_t *in, size_t n_in,
	size_t *pn_cnvt);

/**
 * Return whether a given UTF-32 value corresponds to a printable character.
 */
__declspec(dllimport) bool utf32_isprint(uint32_t v);

/**
 * Case insensitive comparison between two strings
 */
__declspec(dllimport) extern int my_stricmp(const char *s1, const char *s2);

/**
 * Case insensitive comparison between two strings, up to n characters long.
 */
__declspec(dllimport) extern int my_strnicmp(const char *a, const char *b, int n);

/**
 * Case-insensitive strstr
 */
__declspec(dllimport) extern char *my_stristr(const char *string, const char *pattern);

/**
 * Copy up to 'bufsize'-1 characters from 'src' to 'buf' and NULL-terminate
 * the result.  The 'buf' and 'src' strings may not overlap.
 *
 * Returns: strlen(src).  This makes checking for truncation
 * easy.  Example:
 *   if (my_strcpy(buf, src, sizeof(buf)) >= sizeof(buf)) ...;
 *
 * This function should be equivalent to the strlcpy() function in BSD.
 */
__declspec(dllimport) extern size_t my_strcpy(char *buf, const char *src, size_t bufsize);

/**
 * Try to append a string to an existing NULL-terminated string, never writing
 * more characters into the buffer than indicated by 'bufsize', and
 * NULL-terminating the buffer.  The 'buf' and 'src' strings may not overlap.
 *
 * my_strcat() returns strlen(buf) + strlen(src).  This makes checking for
 * truncation easy.  Example:
 *   if (my_strcat(buf, src, sizeof(buf)) >= sizeof(buf)) ...;
 *
 * This function should be equivalent to the strlcat() function in BSD.
 */
__declspec(dllimport) extern size_t my_strcat(char *buf, const char *src, size_t bufsize);

/**
 * Capitalise string 'buf'
 */
__declspec(dllimport) void my_strcap(char *buf);

/**
 * Test equality, prefix, suffix
 */
__declspec(dllimport) extern bool streq(const char *s, const char *t);
__declspec(dllimport) extern bool prefix(const char *s, const char *t);
__declspec(dllimport) extern bool prefix_i(const char *s, const char *t);
__declspec(dllimport) extern bool suffix(const char *s, const char *t);
__declspec(dllimport) extern bool suffix_i(const char *s, const char *t);

#define streq(s, t)		(!strcmp(s, t))

/**
 * Skip occurrences of a characters
 */
__declspec(dllimport) extern void strskip(char *s, const char c, const char e);
__declspec(dllimport) extern void strescape(char *s, const char c);

/**
 * Get the integer value of a hex string
 */
__declspec(dllimport) extern int hex_str_to_int(const char *s);

/**
 * Change escaped characters into their literal representation
 */
__declspec(dllimport) extern void strunescape(char *s);

/**
 * Determines if a string is "empty"
 */
__declspec(dllimport) bool contains_only_spaces(const char* s);

/**
 * Check if a char is a vowel
 */
__declspec(dllimport) bool is_a_vowel(int ch);


/**
 * Allow override of the multi-byte to wide char conversion
 */
__declspec(dllimport) size_t text_mbstowcs(wchar_t *dest, const char *src, int n);

/**
 * Convert a wide character to multibyte representation.
 */
__declspec(dllimport) int text_wctomb(char *s, wchar_t wchar);

/**
 * Get the maximum size to store a wide character converted to multibyte.
 */
__declspec(dllimport) int text_wcsz(void);

/**
 * Return whether the given wide character is printable.
 */
__declspec(dllimport) int text_iswprint(wint_t wc);

/**
 * Return pointer to the first occurrence of wc in the wide-character
 * string pointed to by wcs, or NULL if wc does not occur in the
 * string.
 */
__declspec(dllimport) wchar_t *text_wcschr(const wchar_t *wcs, wchar_t wc);

/**
 * Return the number of wide characters in s.
 */
__declspec(dllimport) size_t text_wcslen(const wchar_t *s);

/**
 * Print an error message
 */
__declspec(dllimport) extern void plog(const char *str);

/**
 * Exit, with optional message
 */
__declspec(dllimport) extern void quit(const char *str);


/**
 * Sorting functions
 */
__declspec(dllimport) extern void sort(void *array, size_t nmemb, size_t smemb,
		 int (*comp)(const void *a, const void *b));

/**
 * Create a hash for a string
 */
__declspec(dllimport) uint32_t djb2_hash(const char *str);

/**
 * Mathematical functions
 */
__declspec(dllimport) int add_guardi(int a, int b);
__declspec(dllimport) int sub_guardi(int a, int b);
__declspec(dllimport) int add_guardi16(int16_t a, int16_t b);
__declspec(dllimport) int sub_guardi16(int16_t a, int16_t b);
__declspec(dllimport) int mean(const int *nums, int size, struct my_rational *frac);
__declspec(dllimport) int variance(const int *nums, int size, bool unbiased, bool of_mean,
		struct my_rational *frac);
__declspec(dllimport) unsigned int gcd(unsigned int a, unsigned int b);
__declspec(dllimport) struct my_rational my_rational_construct(unsigned int numerator,
		unsigned int denominator);
__declspec(dllimport) unsigned int my_rational_to_uint(const struct my_rational *a,
		unsigned int scale, unsigned int *remainder);
__declspec(dllimport) struct my_rational my_rational_product(const struct my_rational *a,
		const struct my_rational *b);
__declspec(dllimport) struct my_rational my_rational_sum(const struct my_rational *a,
		const struct my_rational *b);

#endif /* INCLUDED_Z_UTIL_H */


