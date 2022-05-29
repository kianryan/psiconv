/*
    general.h - Part of psiconv, a PSION 5 file formats converter
    Copyright (c) 2004-2014  Frodo Looijaard <frodo@frodo.looijaard.name>

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

#ifndef GENERAL_H
#define GENERAL_H

#include <psiconv/list.h>
#include <psiconv/configuration.h>
#include <psiconv/unicode.h>
#include "psiconv.h"

/* Several routines to output text.
   The text is appended to a psiconv_list of bytes (u8), in the specified
   encoding. A single character may add several bytes to the list, in some
   encodings. */


/* Output a single UCS2 character */
extern void output_char(psiconv_config config, psiconv_list list,
                         psiconv_ucs2 character, encoding enc);

/* Output a string of UCS2 characters */
extern void output_string(psiconv_config config, psiconv_list list,
                              psiconv_ucs2 *string, encoding enc);


/* Output a string of ASCII chars to the list. Only characters between
   0x20 and 0x7e (inclusive) may be used. */
extern void output_simple_chars(psiconv_config config, psiconv_list list,
                            char *string, encoding enc);


#endif /* GENERAL_H */
