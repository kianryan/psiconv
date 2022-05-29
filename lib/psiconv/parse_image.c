/*
    parse_image.c - Part of psiconv, a PSION 5 file formats converter
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

#include <stdlib.h>

#include "parse_routines.h"
#include "error.h"
#include "image.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

/* Extreme debugging info */
#undef LOUD

static int psiconv_decode_rle8 (const psiconv_config config, int lev,
                         psiconv_u32 off,
                         const psiconv_pixel_bytes encoded,
			 psiconv_pixel_bytes *decoded);

static int psiconv_decode_rle12 (const psiconv_config config, int lev,
                         psiconv_u32 off,
                         const psiconv_pixel_bytes encoded,
			 psiconv_pixel_bytes *decoded);

static int psiconv_decode_rle16 (const psiconv_config config, int lev,
                         psiconv_u32 off,
                         const psiconv_pixel_bytes encoded,
			 psiconv_pixel_bytes *decoded);

static int psiconv_decode_rle24 (const psiconv_config config, int lev,
                         psiconv_u32 off,
                         const psiconv_pixel_bytes encoded,
			 psiconv_pixel_bytes *decoded);

static int psiconv_bytes_to_pixel_data(const psiconv_config config,
                                int lev, psiconv_u32 off,
				const psiconv_pixel_bytes bytes,
				psiconv_pixel_ints *pixels,
				int colordepth, int xsize, int ysize);

static int psiconv_pixel_data_to_floats (const psiconv_config config, int lev,
                                  psiconv_u32 off, 
			 	  const psiconv_pixel_ints pixels,
				  psiconv_pixel_floats_t *floats,
				  int colordepth, int color,
				  int redbits, int bluebits, int greenbits,
				  const psiconv_pixel_floats_t palet);



int psiconv_parse_jumptable_section(const psiconv_config config,
                                    const psiconv_buffer buf,int lev,
                                    psiconv_u32 off, int *length,
                                    psiconv_jumptable_section *result)
{
  int res = 0;
  int len = 0;
  psiconv_u32 listlen,temp;
  int i;

  psiconv_progress(config,lev+1,off+len,"Going to read the jumptable section");
  if (!((*result) = psiconv_list_new(sizeof(psiconv_u32))))
    goto ERROR1;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the list length");
  listlen = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"List length: %08x",listlen);
  len += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read the list");
  for (i = 0; i < listlen; i++) {
    temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR2;
    if ((res = psiconv_list_add(*result,&temp)))
      goto ERROR2;
    psiconv_debug(config,lev+3,off+len,"Offset: %08x",temp);
    len += 4;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of jumptable section "
                   "(total length: %08x)", len);

  return 0;

ERROR2:
  psiconv_list_free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Jumptable Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_paint_data_section(const psiconv_config config,
                                     const psiconv_buffer buf,int lev,
                                     psiconv_u32 off, int *length,int isclipart,
                                     psiconv_paint_data_section *result)
{
  int res = 0;
  int len = 0;
  psiconv_u32 size,offset,temp,datasize,color,
              redbits,bluebits,greenbits;
  psiconv_u8 byte;
  int leng,i;
  psiconv_u32 bits_per_pixel,compression;
  psiconv_pixel_bytes bytes,decoded;
  psiconv_pixel_ints pixels;
  psiconv_pixel_floats_t floats,palet;

  psiconv_progress(config,lev+1,off,"Going to read a paint data section");
  if (!((*result) = malloc(sizeof(**result))))
    goto ERROR1;

  if (!(bytes = psiconv_list_new(sizeof(psiconv_u8))))
    goto ERROR2;

  psiconv_progress(config,lev+2,off+len,"Going to read section size");
  size = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Section size: %08x",size);
  len += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read pixel data offset");
  offset = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  if (offset != 0x28) {
    psiconv_error(config,lev+2,off+len,
                 "Paint data section data offset has unexpected value");
    psiconv_debug(config,lev+2,off+len,
                  "Data offset: read %08x, expected %08x",offset,0x28);
    res = -1;
  }
  len += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read picture X size");
  (*result)->xsize = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Picture X size: %08x:",(*result)->xsize);
  len += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read picture Y size");
  (*result)->ysize = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Picture Y size: %08x:",(*result)->ysize);
  len += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read the real picture x size");
  (*result)->pic_xsize = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Picture x size: %f",(*result)->pic_xsize);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the real picture y size");
  (*result)->pic_ysize = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Picture y size: %f",(*result)->pic_ysize);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the number of bits per pixel");
  bits_per_pixel=psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Bits per pixel: %d",bits_per_pixel);
  len += 4;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read whether this is a colour or greyscale picture");
  color = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  if ((color != 0) && (color != 1)) {
    psiconv_warn(config,lev+2,off+len,
	         "Paint data section unknown color type (ignored)");
    psiconv_debug(config,lev+2,off+len,
	         "Color: read %08x, expected %08x or %08x",color,0,1);
    color = 1;
  } else {
    psiconv_debug(config,lev+2,off+len,"Color: %08x (%s picture)",
	          color,(color?"color":"greyscale"));
  }
  len += 4;

  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  if (temp != 00) {
    psiconv_warn(config,lev+2,off+len,
                 "Paint data section prologue has unknown values (ignored)");
    psiconv_debug(config,lev+2,off+len,
                    "read %08x, expected %08x",temp, 0x00);
  }
  len += 4;
  
  psiconv_progress(config,lev+2,off+len,
                   "Going to read whether RLE compression is used");
  compression=psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  if (compression > 4) {
    psiconv_warn(config,lev+2,off+len,"Paint data section has unknown "
                               "compression type, assuming RLE");
    psiconv_debug(config,lev+2,off+len,"Read compression type %d",compression);
    compression = 0;
  }
  psiconv_debug(config,lev+2,off+len,"Compression: %s",
                compression == 4?"RLE24":compression == 3?"RLE16":
		compression == 2?"RLE12":compression == 1?"RLE8":"none");
  len += 4;

  if (isclipart) {
    psiconv_progress(config,lev+2,off+len,"Going to read an unknown long");
    temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR3;
    if (temp != 0xffffffff) {
      psiconv_warn(config,lev+2,off+len,
                   "Paint data section prologue has unknown values (ignoring)");
      psiconv_debug(config,lev+2,off+len,
                    "Read %08x, expected %08x",temp, 0xffffffff);
    }
    len += 4;
    psiconv_progress(config,lev+2,off+len,"Going to read a second unknown long");
    temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR3;
    if (temp != 0x44) {
      psiconv_warn(config,lev+2,off+len,
                   "Paint data section prologue has unknown values (ignoring)");
      psiconv_debug(config,lev+2,off+len,
                    "read %08x, expected %08x",temp, 0x44);
    }
    len += 4;
  }

  len = offset;
  datasize = size - len;
  if (isclipart)
    len += 8;

  if (color || (bits_per_pixel != 2)) 
    psiconv_warn(config,lev+2,off+len,
	         "All image types except 2-bit greyscale are experimental!");

  psiconv_progress(config,lev+2,off+len,"Going to read the pixel data");
  for (i = 0; i < datasize; i++) {
    byte = psiconv_read_u8(config,buf,lev+2,off+len+i,&res);
#ifdef LOUD
    psiconv_debug(config,lev+2,off+len+i,
	          "Pixel byte %04x of %04x has value %02x",
	          i,datasize,byte);
#endif
    if (res)
      goto ERROR3;
    psiconv_list_add(bytes,&byte);
  }
  len += datasize;

  switch(compression) {
    case 1: 
      if ((res = psiconv_decode_rle8(config,lev+2,off+len,bytes,&decoded)))
	goto ERROR3;
      psiconv_list_free(bytes);
      bytes = decoded;
      break;
    case 2: 
      if ((psiconv_decode_rle12(config,lev+2,off+len,bytes,&decoded)))
	goto ERROR3;
      psiconv_list_free(bytes);
      bytes = decoded;
      break;
    case 3: 
      if ((psiconv_decode_rle16(config,lev+2,off+len,bytes,&decoded)))
        goto ERROR3;
      psiconv_list_free(bytes);
      bytes = decoded;
      break;
    case 4: 
      if ((psiconv_decode_rle24(config,lev+2,off+len,bytes,&decoded)))
        goto ERROR3;
      psiconv_list_free(bytes);
      bytes = decoded;
      break;
  }

  if ((res = psiconv_bytes_to_pixel_data(config,lev+2,off+len,bytes,
	                                 &pixels,bits_per_pixel,
                                         (*result)->xsize,(*result)->ysize)))
    goto ERROR3;

  /* Use some heuristics; things may get unexpected around here */
  bluebits = redbits = greenbits = 0;
  palet = psiconv_palet_none;
  if (color) {
    if (bits_per_pixel == 4) 
      palet = psiconv_palet_color_4;
    else if (bits_per_pixel == 8) 
      palet = psiconv_palet_color_8;
    else {
      redbits = (bits_per_pixel+2) / 3;
      greenbits = (bits_per_pixel+2) / 3;
      bluebits = bits_per_pixel - redbits - greenbits;
    }
  }
  if ((res = psiconv_pixel_data_to_floats(config,lev+2,off+len,pixels,
	                                  &floats,bits_per_pixel,color,
					  redbits,greenbits,bluebits,palet)))
    goto ERROR4;
  
  (*result)->red = floats.red;
  (*result)->green = floats.green;
  (*result)->blue = floats.blue;

  psiconv_list_free(bytes);
  psiconv_list_free(pixels);


  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of Paint Data Section (total length: %08x)", len);

  return 0;

ERROR4:
  psiconv_list_free(pixels);
ERROR3:
  psiconv_list_free(bytes);
ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Paint Data Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sketch_section(const psiconv_config config,
                                 const psiconv_buffer buf, int lev,
                                 psiconv_u32 off, int *length,
                                 psiconv_sketch_section *result)
{
  int res=0;
  int len=0;
  psiconv_u32 temp;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read the sketch section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read the displayed hor. size");
  (*result)->displayed_xsize = psiconv_read_u16(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Displayed hor. size: %04x",
                (*result)->displayed_xsize);
  len += 0x02;
  psiconv_progress(config,lev+2,off+len,"Going to read displayed ver. size");
  (*result)->displayed_ysize = psiconv_read_u16(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Displayed ver. size: %04x",
                (*result)->displayed_ysize);
  len += 0x02;

  psiconv_progress(config,lev+2,off+len,"Going to read the data hor. offset");
  (*result)->picture_data_x_offset = psiconv_read_u16(config,buf,lev+2,off + len,
					&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Data hor. offset: %04x",
                (*result)->picture_data_x_offset);
  len += 0x02;
  psiconv_progress(config,lev+2,off+len,"Going to read the data ver. offset");
  (*result)->picture_data_y_offset = psiconv_read_u16(config,buf,lev+2,off + len,
						&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Data ver. offset: %04x",
                (*result)->picture_data_y_offset);
  len += 0x02;

  psiconv_progress(config,lev+2,off+len,"Going to read the displayed hor. offset");
  (*result)->displayed_size_x_offset = psiconv_read_u16(config,buf,lev+2,off + len,
							&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Displayed hor. offset: %04x",
                (*result)->displayed_size_x_offset);
  len += 0x02;
  psiconv_progress(config,lev+2,off+len,"Going to read the displayed ver. offset");
  (*result)->displayed_size_y_offset = psiconv_read_u16(config,buf,lev+2,off + len,
					  &res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Displayed ver. offset: %04x",
                (*result)->displayed_size_y_offset);
  len += 0x02;

  psiconv_progress(config,lev+2,off+len,"Going to read the form hor. size");
  (*result)->form_xsize = psiconv_read_u16(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Form hor. size: %04x",
                (*result)->form_xsize);
  len += 0x02;
  psiconv_progress(config,lev+2,off+len,"Going to read form ver. size");
  (*result)->form_ysize = psiconv_read_u16(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Form ver. size: %04x",
                (*result)->form_ysize);
  len += 0x02;
    
  psiconv_progress(config,lev+2,off+len,"Going to skip 1 word of zeros");
  temp = psiconv_read_u16(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0) {
    psiconv_warn(config,lev+2,off+len,
                 "Unexpected value in sketch section preamble (ignored)");
    psiconv_debug(config,lev+2,off+len,"Read %04x, expected %04x",
                  temp,0);
  }
  off += 0x02;

  psiconv_progress(config,lev+2,off+len,"Going to read the picture data");
  if ((res = psiconv_parse_paint_data_section(config,buf,lev+2,off+len,&leng,0,
                                          &((*result)->picture))))
    goto ERROR2;
  off += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the hor. magnification");
  (*result)->magnification_x = psiconv_read_u16(config,buf,lev+2,off+len,&res)/1000.0;
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Form hor. magnification: %f",
                (*result)->magnification_x);
  len += 0x02;
  psiconv_progress(config,lev+2,off+len,"Going to read the ver. magnification");
  (*result)->magnification_y = psiconv_read_u16(config,buf,lev+2,off+len,&res)/1000.0;
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Form ver. magnification: %f",
                (*result)->magnification_y);
  len += 0x02;

  psiconv_progress(config,lev+2,off+len,"Going to read the left cut");
  temp = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR3;
  (*result)->cut_left = (temp * 6.0) / (*result)->displayed_xsize;
  psiconv_debug(config,lev+2,off+len,"Left cut: raw %08x, real: %f",
                temp,(*result)->cut_left);
  len += 0x04;
  psiconv_progress(config,lev+2,off+len,"Going to read the right cut");
  temp = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR3;
  (*result)->cut_right = (temp * 6.0) / (*result)->displayed_xsize;
  psiconv_debug(config,lev+2,off+len,"Right cut: raw %08x, real: %f",
                temp,(*result)->cut_right);
  len += 0x04;
  psiconv_progress(config,lev+2,off+len,"Going to read the top cut");
  temp = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR3;
  (*result)->cut_top = (temp * 6.0) / (*result)->displayed_ysize;
  psiconv_debug(config,lev+2,off+len,"Top cut: raw %08x, real: %f",
                temp,(*result)->cut_top);
  len += 0x04;
  psiconv_progress(config,lev+2,off+len,"Going to read the bottom cut");
  temp = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR3;
  (*result)->cut_bottom = (temp * 6.0) / (*result)->displayed_ysize;
  psiconv_debug(config,lev+2,off+len,"Bottom cut: raw %08x, real: %f",
                temp,(*result)->cut_bottom);
  len += 0x04;
  
  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sketch section (total length: %08x)", len);

  return res;
ERROR3:
  psiconv_free_paint_data_section((*result)->picture);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sketch Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}


int psiconv_parse_clipart_section(const psiconv_config config,
                                  const psiconv_buffer buf,int lev,
                                  psiconv_u32 off, int *length,
                                  psiconv_clipart_section *result)
{
  int res=0;
  int len=0;
  int leng;
  psiconv_u32 temp;

  psiconv_progress(config,lev+1,off+len,"Going to read the clipart section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read the section ID");
  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != PSICONV_ID_CLIPART_ITEM) {
    psiconv_warn(config,lev+2,off+len,
                 "Unexpected value in clipart section preamble (ignored)");
    psiconv_debug(config,lev+2,off+len,"Read %08x, expected %08x",temp,
                  PSICONV_ID_CLIPART_ITEM);
  } else 
    psiconv_debug(config,lev+2,off+len,"Clipart ID: %08x", temp);
  off += 4;
  
  psiconv_progress(config,lev+2,off+len,"Going to read an unknown long");
  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Unexpected value in clipart section preamble (ignored)");
    psiconv_debug(config,lev+2,off+len,"Read %08x, expected %08x",temp,
                  0x02);
  } else 
    psiconv_debug(config,lev+2,off+len,"First unknown long: %08x", temp);
  off += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read a second unknown long");
  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0) {
    psiconv_warn(config,lev+2,off+len,
                 "Unexpected value in clipart section preamble (ignored)");
    psiconv_debug(config,lev+2,off+len,"Read %08x, expected %08x",temp, 0);
  } else 
    psiconv_debug(config,lev+2,off+len,"Second unknown long: %08x", temp);
  off += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read a third unknown long");
  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0) {
    psiconv_warn(config,lev+2,off+len,
                 "Unexpected value in clipart section preamble (ignored)");
    psiconv_debug(config,lev+2,off+len,"Read %08x, expected %08x",temp, 0);
  } else 
    psiconv_debug(config,lev+2,off+len,"Third unknown long: %08x", temp);
  off += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read a fourth unknown long");
  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if ((temp != 0x0c) && (temp != 0x08)) {
    psiconv_warn(config,lev+2,off+len,
                 "Unexpected value in clipart section preamble (ignored)");
    psiconv_debug(config,lev+2,off+len,"Read %08x, expected %08x or %08x",temp,
                   0x0c, 0x08);
  } else 
    psiconv_debug(config,lev+2,off+len,"Fourth unknown long: %08x", temp);
  off += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read the Paint Data Section");
  if ((res = psiconv_parse_paint_data_section(config,buf,lev+2,off+len,&leng,1,
                                          &((*result)->picture))))
    goto ERROR2;
  len += leng;

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of clipart section (total length: %08x)", len);
  return 0;

ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Font failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_decode_rle8 (const psiconv_config config, int lev, psiconv_u32 off,
                         const psiconv_pixel_bytes encoded,
			 psiconv_pixel_bytes *decoded)
{
  int res=0;
  psiconv_u8 *marker,*value;
  int i,j;

  psiconv_progress(config,lev+1,off,"Going to decode the RLE8 encoding");
  if (!(*decoded = psiconv_list_new(sizeof(psiconv_u8))))
    goto ERROR1;

  for (i = 0; i < psiconv_list_length(encoded);) {
#ifdef LOUD
    psiconv_progress(config,lev+2,off,"Going to read marker byte at %04x",i);
#endif
    if (!(marker = psiconv_list_get(encoded,i)))
      goto ERROR2;
#ifdef LOUD
    psiconv_debug(config,lev+2,off,"Marker byte: %02x",*marker);
#endif
    if (*marker < 0x80) {
#ifdef LOUD
      psiconv_debug(config,lev+2,off,"Marker: repeat value byte %02x times",
	            *marker+1); */
      psiconv_progress(config,lev+2,off,"Going to read value byte at %04x",i+1);
#endif
      if (!(value = psiconv_list_get(encoded,i+1)))
	goto ERROR2;
#ifdef LOUD
      psiconv_debug(config,lev+2,off,"Value byte: %02x",*value); 
      psiconv_progress(config,lev+2,off,"Adding %02x pixels %02x",
	               *marker+1,*value);
#endif
      for (j = 0; j < *marker + 1; j++)
        if ((res = psiconv_list_add(*decoded,value)))
	  goto ERROR2;
      i += 2;
    } else {
#ifdef LOUD
      psiconv_debug(config,lev+2,off,"Marker: %02x value bytes follow",
	            0x100 - *marker);
#endif
      for (j = 0; j < (0x100 - *marker); j++) {
#ifdef LOUD
	psiconv_progress(config,lev+2,off,"Going to read value byte at %04x",
	                 i+j+1);
#endif
	if (!(value = psiconv_list_get(encoded,i+j+1)))
	  goto ERROR2;
#ifdef LOUD
	 psiconv_debug(config,lev+2,off,"Value: %02x",*value);
#endif
	if ((res = psiconv_list_add(*decoded,value)))
	  goto ERROR2;
      }
      i += (0x100 - *marker) + 1;
    }
  }
  psiconv_progress(config,lev,off,
                   "End of RLE8 decoding process");
  return 0;

ERROR2:
  psiconv_list_free(*decoded);
ERROR1:
  psiconv_error(config,lev+1,off,"Decoding of RLE8 failed");
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}	

int psiconv_decode_rle12 (const psiconv_config config, int lev, psiconv_u32 off,
                         const psiconv_pixel_bytes encoded,
			 psiconv_pixel_bytes *decoded)
{
  int res=0;
  psiconv_u8 *value0,*value1;
  psiconv_u32 value,repeat;
  int i,j;

  psiconv_progress(config,lev+1,off,"Going to decode the RLE12 encoding");
  if (!(*decoded = psiconv_list_new(sizeof(psiconv_u8))))
    goto ERROR1;

  for (i = 0; i < psiconv_list_length(encoded);) {
    psiconv_progress(config,lev+2,off,"Going to read data word at %04x",i);
    if (!(value0 = psiconv_list_get(encoded,i)))
      goto ERROR2;
    if (!(value1 = psiconv_list_get(encoded,i+1)))
      goto ERROR2;
    psiconv_debug(config,lev+2,off,"Data Word: %04x",*value0 + (*value1 << 8));
    value = *value0 + ((*value1 & 0x0f) << 8);
    repeat = (*value1 >> 4) + 1;
    psiconv_progress(config,lev+2,off,"Adding %02x pixels %03x",
	             repeat,value);
    for (j = 0; j < repeat; j ++) 
      if ((res = psiconv_list_add(*decoded,&value)))
	goto ERROR2;
    i += 2;
  }
  psiconv_progress(config,lev,off,
                   "End of RLE12 decoding process");
  return 0;

ERROR2:
  psiconv_list_free(*decoded);
ERROR1:
  psiconv_error(config,lev+1,off,"Decoding of RLE12 failed");
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_decode_rle16 (const psiconv_config config, int lev, psiconv_u32 off,
                         const psiconv_pixel_bytes encoded,
			 psiconv_pixel_bytes *decoded)
{
  int res=0;
  psiconv_u8 *marker,*value0,*value1;
  psiconv_u32 value;
  int i,j;

  psiconv_progress(config,lev+1,off,"Going to decode the RLE16 encoding");
  if (!(*decoded = psiconv_list_new(sizeof(psiconv_u8))))
    goto ERROR1;

  for (i = 0; i < psiconv_list_length(encoded);) {
    psiconv_progress(config,lev+2,off,"Going to read marker byte at %04x",i);
    if (!(marker = psiconv_list_get(encoded,i)))
      goto ERROR2;
    psiconv_debug(config,lev+2,off,"Marker byte: %02x",*marker);
    if (*marker < 0x80) {
      psiconv_debug(config,lev+2,off,"Marker: repeat value word %02x times",
	            *marker+1);
      psiconv_progress(config,lev+2,off,"Going to read value word at %04x",i+1);
      if (!(value0 = psiconv_list_get(encoded,i+1)))
	goto ERROR2;
      if (!(value1 = psiconv_list_get(encoded,i+2)))
	goto ERROR2;
      value = *value0 + (*value1 << 8);
      psiconv_debug(config,lev+2,off,"Value word: %02x",value);
      psiconv_progress(config,lev+2,off,"Adding %02x pixels %04x",
	               *marker+1,value);
      for (j = 0; j < *marker + 1; j++)
        if ((res = psiconv_list_add(*decoded,&value)))
	  goto ERROR2;
      i += 3;
    } else {
      psiconv_debug(config,lev+2,off,"Marker: %02x value words follow",
	            0x100 - *marker);
      for (j = 0; j < (0x100 - *marker); j++) {
	psiconv_progress(config,lev+2,off,"Going to read value word at %04x",
	                 i+j*2+1);
	if (!(value0 = psiconv_list_get(encoded,i+j*2+1)))
	  goto ERROR2;
	if (!(value1 = psiconv_list_get(encoded,i+j*2+2)))
	  goto ERROR2;
	value = *value0 + (*value1 << 8);
	psiconv_debug(config,lev+2,off,"Value: %04x",value);
	if ((res = psiconv_list_add(*decoded,&value)))
	  goto ERROR2;
      }
      i += (0x100 - *marker)*2 + 1;
    }
  }
  psiconv_progress(config,lev,off,
                   "End of RLE16 decoding process");
  return 0;

ERROR2:
  psiconv_list_free(*decoded);
ERROR1:
  psiconv_error(config,lev+1,off,"Decoding of RLE16 failed");
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_decode_rle24 (const psiconv_config config, int lev, psiconv_u32 off,
                          const psiconv_pixel_bytes encoded,
			  psiconv_pixel_bytes *decoded)
{
  int res=0;
  psiconv_u8 *marker,*value0,*value1,*value2;
  psiconv_u32 value;
  int i,j;

  psiconv_progress(config,lev+1,off,"Going to decode the RLE24 encoding");
  if (!(*decoded = psiconv_list_new(sizeof(psiconv_u8))))
    goto ERROR1;

  for (i = 0; i < psiconv_list_length(encoded);) {
    psiconv_progress(config,lev+2,off,"Going to read marker byte at %04x",i);
    if (!(marker = psiconv_list_get(encoded,i)))
      goto ERROR2;
    psiconv_debug(config,lev+2,off,"Marker byte: %02x",*marker);
    if (*marker < 0x80) {
      psiconv_debug(config,lev+2,off,"Marker: repeat value byte triplet %02x times",
	            *marker+1);
      psiconv_progress(config,lev+2,off,"Going to read value byte triplet at %04x",i+1);
      if (!(value0 = psiconv_list_get(encoded,i+1)))
	goto ERROR2;
      if (!(value1 = psiconv_list_get(encoded,i+2)))
	goto ERROR2;
      if (!(value2 = psiconv_list_get(encoded,i+3)))
	goto ERROR2;
      value = *value0 + (*value1 << 8) + (*value2 << 16);
      psiconv_debug(config,lev+2,off,"Value byte triplet: %06x",value);
      psiconv_progress(config,lev+2,off,"Adding %02x pixels %06x",
	               *marker+1,value);
      for (j = 0; j < *marker + 1; j++)
        if ((res = psiconv_list_add(*decoded,&value)))
	  goto ERROR2;
      i += 4;
    } else {
      psiconv_debug(config,lev+2,off,"Marker: %02x value byte triplets follow",
	            0x100 - *marker);
      for (j = 0; j < (0x100 - *marker); j++) {
	psiconv_progress(config,lev+2,off,"Going to read value byte triplets at %04x",
	                 i+j*3+1);
	if (!(value0 = psiconv_list_get(encoded,i+j*3+1)))
	  goto ERROR2;
	if (!(value1 = psiconv_list_get(encoded,i+j*3+2)))
	  goto ERROR2;
	if (!(value2 = psiconv_list_get(encoded,i+j*3+3)))
	  goto ERROR2;
	value = *value0 + (*value1 << 8) + (*value2 << 16);
	psiconv_debug(config,lev+2,off,"Value: %06x",value);
	if ((res = psiconv_list_add(*decoded,&value)))
	  goto ERROR2;
      }
      i += (0x100 - *marker)*3 + 1;
    }
  }
  psiconv_progress(config,lev,off,
                   "End of RLE24 decoding process");
  return 0;

ERROR2:
  psiconv_list_free(*decoded);
ERROR1:
  psiconv_error(config,lev+1,off,"Decoding of RLE24 failed");
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_bytes_to_pixel_data(const psiconv_config config,
                                int lev, psiconv_u32 off,
				const psiconv_pixel_bytes bytes,
				psiconv_pixel_ints *pixels,
				int colordepth, int xsize, int ysize)
{
  int res=0;
  int ibits,obits,x,y,bits;
  psiconv_u8 input;
  psiconv_u32 nr,output;
  psiconv_u8 *ientry;

  psiconv_progress(config,lev+1,off,"Going to convert the bytes to pixels");
  if (!(*pixels = psiconv_list_new(sizeof(psiconv_u32))))
    goto ERROR1;

  nr = 0;
  for (y = 0; y < ysize; y++) {
    /* New lines will start at longs */
    while (nr % 4)
      nr ++;
    input = 0;
    ibits = 0;
    for (x= 0; x < xsize; x++) {
#ifdef LOUD
      psiconv_progress(config,lev+2,off,
	               "Processing pixel at (x,y) = (%04x,%04x)",x,y);
#endif
      output = 0;
      obits = 0;
      while (obits < colordepth) {
	if (ibits == 0) {
#ifdef LOUD
	  psiconv_progress(config,lev+3,off,
	                   "Going to read byte %08x",nr);
#endif
	  if (!(ientry = psiconv_list_get(bytes,nr)))
	    goto ERROR2;
#ifdef LOUD
	  psiconv_debug(config,lev+3,off,"Byte value: %02x",*ientry);
#endif
	  input = *ientry;
	  ibits = 8;
	  nr ++;
	}
	bits = ibits + obits > colordepth?colordepth-obits:ibits;
	output = output << bits;
        output |= input & ((1 << bits) - 1);
	input = input >> bits;
	ibits -= bits;
  	obits += bits;
      }
#ifdef LOUD
      psiconv_debug(config,lev+2,off,"Pixel value: %08x",output);
#endif
      if ((res = psiconv_list_add(*pixels,&output)))
	goto ERROR2;
    }
  }

  psiconv_progress(config,lev,off,
                   "Converting bytes to pixels completed");
  return 0;


ERROR2:
  psiconv_list_free(*pixels);
ERROR1:
  psiconv_error(config,lev+1,off,"Converting bytes to pixels failed");
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_pixel_data_to_floats (const psiconv_config config, int lev,
                                  psiconv_u32 off, 
			 	  const psiconv_pixel_ints pixels,
				  psiconv_pixel_floats_t *floats,
				  int colordepth, int color,
				  int redbits, int bluebits, int greenbits,
				  const psiconv_pixel_floats_t palet)
{
  int res = 0;
  psiconv_u32 i;
  psiconv_u32 *pixel;

  psiconv_progress(config,lev+1,off,"Going to convert pixels to floats");
  if (!((*floats).red = malloc(psiconv_list_length(pixels) * 
	                    sizeof(*(*floats).red))))
    goto ERROR1;
  if (!((*floats).green = malloc(psiconv_list_length(pixels) *
	                      sizeof(*(*floats).green))))
    goto ERROR2;
  if (!((*floats).blue = malloc(psiconv_list_length(pixels) *
	                     sizeof(*(*floats).blue))))
    goto ERROR3;
  (*floats).length = psiconv_list_length(pixels);

  for (i = 0; i < psiconv_list_length(pixels); i++) {
    if (!(pixel = psiconv_list_get(pixels,i)))
      goto ERROR4;
#ifdef LOUD
    psiconv_progress(config,lev+2,off, "Handling pixel %04x (%04x)",i,*pixel);
#endif
    if (!palet.length) {
      if (color) {
	(*floats).blue[i] = ((float) (*pixel & ((1 << bluebits) - 1))) / 
	             ((1 << bluebits) - 1);
	(*floats).green[i] = ((float) ((*pixel >> bluebits) & 
   		            ((1 << greenbits) - 1))) / ((1 << greenbits) - 1);
	(*floats).red[i] = ((float) ((*pixel >> (bluebits+greenbits)) & 
                            ((1 << redbits) - 1))) / ((1 << redbits) - 1);
      } else {
        (*floats).red[i] = (*floats).green[i] = 
	                (*floats).blue[i] = ((float) *pixel) / 
			                     ((1 << colordepth) - 1);
      }
    } else {
      if (*pixel >= palet.length) {
	psiconv_warn(config,lev+2,off,
	             "Invalid palet color found (using color 0x00)");
	(*floats).red[i] = palet.red[0];
	(*floats).green[i] = palet.green[0];
	(*floats).blue[i] = palet.blue[0];
      } else {
	(*floats).red[i] = palet.red[*pixel];
	(*floats).green[i] = palet.green[*pixel];
	(*floats).blue[i] = palet.blue[*pixel];
      }
    }
#ifdef LOUD
    psiconv_debug(config,lev+2,off, "Pixel: Red (%f), green (%f), blue (%f)",
	          (*floats).red[i],(*floats).green[i],(*floats).blue[i]);
#endif
  }
  psiconv_progress(config,lev+1,off,"Finished converting pixels to floats");
  return 0;

ERROR4:
  free((*floats).blue);
ERROR3:
  free((*floats).green);
ERROR2:
  free((*floats).red);
ERROR1:
  psiconv_error(config,lev+1,off,"Converting pixels to floats failed");
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}



