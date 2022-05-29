/*
    parse_aux.c - Part of psiconv, a PSION 5 file formats converter
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

#include "config.h"
#include "compat.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif


char *psiconv_make_printable(const psiconv_config config,
                             const psiconv_string_t input)
{
  int i;
  char *output;

  if (!(output = malloc(sizeof(*output) * 
	                (psiconv_unicode_strlen(input) + 1)))) {
    return NULL;
  }

  for (i = 0; i < psiconv_unicode_strlen(input); i ++)
    if (input[i] < 0x20 || input[i] >= 0x7f)
      output[i] = '.';
    else
      output[i] = input[i];
  output[i] = 0;
  return output;
}
