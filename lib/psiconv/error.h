/*
    error.h - Part of psiconv, a PSION 5 file formats converter
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

#ifndef PSICONV_ERROR_H
#define PSICONV_ERROR_H

#include <psiconv/general.h>
#include <psiconv/configuration.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* These functions print error, warning, progress and debug information to
 *    stderr */
extern void psiconv_fatal(psiconv_config config,int level, psiconv_u32 off, 
                          const char *format,...);
extern void psiconv_error(psiconv_config config,int level, psiconv_u32 off,
                          const char *format,...);
extern void psiconv_warn(psiconv_config config,int level, psiconv_u32 off,
                         const char *format,...);
extern void psiconv_progress(psiconv_config config,int level, psiconv_u32 off,
                             const char *format,...);
extern void psiconv_debug(psiconv_config config,int level, psiconv_u32 off,
                          const char *format,...);

#define PSICONV_VERB_DEBUG 5
#define PSICONV_VERB_PROGRESS 4
#define PSICONV_VERB_WARN 3
#define PSICONV_VERB_ERROR 2
#define PSICONV_VERB_FATAL 1

#define PSICONV_E_OK    0
#define PSICONV_E_OTHER 1
#define PSICONV_E_NOMEM 2
#define PSICONV_E_PARSE 3
#define PSICONV_E_GENERATE 4

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PSICONV_ERROR_H */
