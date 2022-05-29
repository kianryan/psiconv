/*
    image.h - Part of psiconv, a PSION 5 file formats converter
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

/* This file contains definitions used internally by
   generate_image.c and parse_image.c */

#ifndef PSICONV_IMAGE_H
#define PSICONV_IMAGE_H

#include <psiconv/configuration.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
 
typedef psiconv_list psiconv_pixel_bytes; /* psiconv_u8 */
typedef psiconv_list psiconv_pixel_ints; /* of psiconv_u32 */

typedef struct psiconv_pixel_float_s
{
  psiconv_u32 length;
  float *red;
  float *green;
  float *blue;
} psiconv_pixel_floats_t;

extern psiconv_pixel_floats_t psiconv_palet_none, psiconv_palet_color_4, 
                              psiconv_palet_color_8;

#ifdef __cplusplus
}
#endif /* __cplusplus */
                                                                                
#endif /* def PSICONV_IMAGE_H */
