/*
    unicode.h - Part of psiconv, a PSION 5 file formats converter
    Copyright (c) 2003-2014  Frodo Looijaard <frodo@frodo.looijaard.name>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef PSICONV_UNICODE_H
#define PSICONV_UNICODE_H

#include <psiconv/general.h>
#include <psiconv/configuration.h>
#include <psiconv/buffer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* A simple unicode implementation, using UCS-2 (16-bit encoding).
   Unicode strings are arrays of psiconv_u16 characters, zero-terminated.
   Note that there is a lot in Unicode we do not support; for example,
   we assume a single Unicode codepoint corresponds with a single character.
   For EPOC, that should be enough */

extern int psiconv_unicode_select_characterset(const psiconv_config config,
                                               int charset);

/* Translate a single character to a unicode character, using the
   translation tables in config */
extern psiconv_ucs2 psiconv_unicode_read_char(const psiconv_config config,
                                              psiconv_buffer buf,
					      int lev,psiconv_u32 off,
					      int *length,
                                              int *status);

extern int psiconv_unicode_write_char(const psiconv_config config,
                                               psiconv_buffer buf,
					       int lev,
					       psiconv_ucs2 value);


/* Compute the length of a unicode string */
extern int psiconv_unicode_strlen(const psiconv_ucs2 *input);

/* Duplicate a unicode string */
extern psiconv_ucs2 *psiconv_unicode_strdup(const psiconv_ucs2 *input);

/* Compare two unicode strings. Ordering as in Unicode codepoints! */
extern int psiconv_unicode_strcmp(const psiconv_ucs2 *str1, const psiconv_ucs2 *str2);

/* Return a newly allocated empty string */
extern psiconv_ucs2 *psiconv_unicode_empty_string(void);

/* Convert a psiconv_list of psiconv_ucs2 characters to a string */
extern psiconv_ucs2 *psiconv_unicode_from_list(psiconv_list input);

/* Look for needle in haystack, return pointer to found location */
extern psiconv_ucs2 *psiconv_unicode_strstr(const psiconv_ucs2 *haystack,
                                            const psiconv_ucs2 *needle);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PSICONV_ERROR_H */
