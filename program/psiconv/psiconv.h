/*
    psiconv.h - Part of psiconv, a PSION 5 file formats converter
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

#ifndef PSICONV_H
#define PSICONV_H

#include <psiconv/data.h>
#include <psiconv/configuration.h>

#define FORMAT_WORD             0x01
#define FORMAT_TEXTED           0x02
#define FORMAT_CLIPART_SINGLE   0x04
#define FORMAT_CLIPART_MULTIPLE 0x08
#define FORMAT_MBM_SINGLE       0x10
#define FORMAT_MBM_MULTIPLE     0x20
#define FORMAT_SKETCH           0x40

typedef enum
{ 
  ENCODING_UTF8,
  ENCODING_UCS2,
  ENCODING_PSION,
  ENCODING_ASCII,
  ENCODING_ASCII_HTML
} encoding;

typedef int output_function(const psiconv_config config,
                            psiconv_list list, const psiconv_file file,
                            const char *type,
			    const encoding encoding_type);

typedef struct fileformat_s {
  const char *name;
  const char *description;
  int supported_format;
  output_function *output;
} *fileformat;

psiconv_list fileformat_list; /* of struct psiconv_fileformat */


#endif /* PSICONV_H */
