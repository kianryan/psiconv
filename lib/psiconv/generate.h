/*
    generate.h - Part of psiconv, a PSION 5 file formats converter
    Copyright (c) 2000-2014  Frodo Looijaard <frodo@frodo.looijaard.name>

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

#ifndef PSICONV_GENERATE_H
#define PSICONV_GENERATE_H

#include <psiconv/general.h>
#include <psiconv/configuration.h>
#include <psiconv/error.h>
#include <psiconv/data.h>
#include <psiconv/common.h>
#include <psiconv/list.h>
#include <psiconv/buffer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Generate a Psion file. If its return-value is non-zero, something has
   gone horribly wrong (badly corrupted data, or out of memory, usually),
   and *buf is undefined and unallocated; in normal cases, memory is
   allocated to it and it is up to you to free it.
*/
extern int psiconv_write(psiconv_config config, psiconv_buffer *buf,
                         const psiconv_file value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* def PSICONV_GENERATE_H */
