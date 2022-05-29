/*
    common.h - Part of psiconv, a PSION 5 file formats converter
    Copyright (c) 1999-2014  Frodo Looijaard <frodo@frodo.looijaard.name>

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

/* Declarations only needed for the parser. If you want to parse, just
   include this. */

#ifndef PSICONV_COMMON_H
#define PSICONV_COMMON_H

#include <psiconv/general.h>
#include <psiconv/configuration.h>
#include <psiconv/unicode.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ***************
   * misc.c *
   *************** */

/* This function returns a copy of a Unicode string, converted to plain ASCII.
   Anything codepage dependent (> 128) is sanitized away.
   You should free this string yourself when you are
   done with it. Returns NULL if there is not enough memory left. */
extern char *psiconv_make_printable(const psiconv_config config,
                                    const psiconv_string_t s);


/* **************
   * checkuid.c *
   ************** */

extern psiconv_u32 psiconv_checkuid(psiconv_u32 uid1,
                                    psiconv_u32 uid2,psiconv_u32 uid3);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* def PSICONV_COMMON_H */
