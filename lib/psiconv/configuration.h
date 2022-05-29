/*
    configuration.h - Part of psiconv, a PSION 5 file formats converter
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

#ifndef PSICONV_CONFIG_H
#define PSICONV_CONFIG_H

#include <psiconv/general.h>
#include <psiconv/data.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void psiconv_error_handler_t (int kind, psiconv_u32 off,
                                      const char *message);

typedef struct psiconv_config_s
{
  int verbosity;
  int colordepth;
  int redbits;    /* Only needed when color is used and no palet */
  int greenbits;  /* Only needed when color is used and no palet */
  int bluebits;   /* Only needed when color is used and no palet */
  psiconv_bool_t color;
  psiconv_error_handler_t *error_handler;
  psiconv_u8 unknown_epoc_char;
  psiconv_ucs2 unknown_unicode_char;
  psiconv_ucs2 unicode_table[0x100];
  psiconv_bool_t unicode;
} *psiconv_config;

extern psiconv_config psiconv_config_default(void);

extern void psiconv_config_read(const char *extra_config_files,
                                psiconv_config *config);

extern void psiconv_config_free(psiconv_config config);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PSICONV_ERROR_H */
