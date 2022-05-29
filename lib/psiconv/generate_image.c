/*
    generate_image.c - Part of psiconv, a PSION 5 file formats converter
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

#include "generate_routines.h"
#include "error.h"
#include "list.h"
#include "image.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif


static int psiconv_collect_pixel_data(psiconv_pixel_ints *pixels,
                               int xsize,int ysize, 
                               const psiconv_pixel_floats_t data,
                               int colordepth,int color,
			       int redbits,int greenbits,int bluebits,
			       const psiconv_pixel_floats_t palet);
static int psiconv_pixel_data_to_bytes(const psiconv_config config,int lev,
                                psiconv_pixel_bytes *bytes, int xsize,
                                int ysize, const psiconv_pixel_ints pixels, 
				int colordepth);
static int psiconv_encode_rle8(const psiconv_config config, 
                        const psiconv_pixel_bytes plain_bytes,
			psiconv_pixel_bytes *encoded_bytes);
static int psiconv_encode_rle12(const psiconv_config config, 
                        const psiconv_pixel_bytes plain_bytes,
			psiconv_pixel_bytes *encoded_bytes);
static int psiconv_encode_rle16(const psiconv_config config, 
                        const psiconv_pixel_bytes plain_bytes,
			psiconv_pixel_bytes *encoded_bytes);
static int psiconv_encode_rle24(const psiconv_config config, 
                        const psiconv_pixel_bytes plain_bytes,
			psiconv_pixel_bytes *encoded_bytes);

int psiconv_write_paint_data_section(const psiconv_config config,
                                     psiconv_buffer buf, int lev,
                                     const psiconv_paint_data_section value,
				     int is_clipart)
{
  int res,colordepth,i;
  psiconv_pixel_ints ints;
  psiconv_pixel_floats_t floats,palet;
  psiconv_list bytes,bytes_rle;
  psiconv_u8 *byteptr,encoding;

  psiconv_progress(config,lev,0,"Writing paint data section");

  /* First, we check whether we can cope with the current configuration.
     If not, we stop at once */
  if ((config->colordepth != 2) && (config->colordepth != 4) &&
      (config->colordepth != 8) && (config->colordepth != 12) &&
      (config->colordepth != 16) && (config->colordepth != 24)) {
    psiconv_error(config,lev,0,
	         "Unsupported color depth (%d); try 2, 4, 8, 16 or 24",
		 config->colordepth);
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

 if ((config->color) && 
     (config->bluebits || config->redbits || config->greenbits) &&
     (config->bluebits+config->redbits+config->greenbits!=config->colordepth)) {
   psiconv_error(config,lev,0,
                "Sum of red (%d), green  (%d) and blue (%d) bits should be "
		"equal to the color depth (%d)",
		config->redbits,config->greenbits,config->bluebits,
		config->colordepth);
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

 if (config->color && 
     !(config->redbits || config->greenbits || config->bluebits) &&
     (config->colordepth != 4) && (config->colordepth != 8)) {
   psiconv_error(config,lev,0,
                "Current color depth (%d) has no palet associated with it",
		config->colordepth);
   res = -PSICONV_E_GENERATE;
   goto ERROR1;
 }

 if (config->color || (config->colordepth != 2))
   psiconv_warn(config,lev,0,
                "All image types except 2-bit greyscale are experimental!");


  if (!value) {
    psiconv_error(config,lev,0,"Null paint data section");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  floats.red = value->red;
  floats.green = value->green;
  floats.blue = value->blue;
  floats.length = value->xsize * value->ysize;

  palet = psiconv_palet_none;
  if ((config->color) && (config->redbits == 0) && (config->greenbits == 0) &&
      (config->bluebits == 0))
    switch (config->colordepth) {
      case 4: palet = psiconv_palet_color_4; break;
      case 8: palet = psiconv_palet_color_8; break;
      default: palet = psiconv_palet_none; break;
    }

  if ((res = psiconv_collect_pixel_data(&ints,value->xsize,
	                        value->ysize,floats,
	                        config->colordepth,config->color,
				config->redbits,config->greenbits,
				config->bluebits,palet))) {
    psiconv_error(config,lev,0,"Error collecting pixel data");
    goto ERROR1;
  }

  if ((res = psiconv_pixel_data_to_bytes(config,lev+1,&bytes,value->xsize,
	                                 value->ysize,ints,
					 config->colordepth))) {
    psiconv_error(config,lev,0,"Error translating pixel data to bytes");
    goto ERROR2;
  }


  switch (config->colordepth) {
    case 2:
    case 4:
    case 8:
      encoding = 0x01;
      if ((res = psiconv_encode_rle8(config,bytes,&bytes_rle))) {
        psiconv_error(config,lev,0,"Error encoding RLE8");
	goto ERROR3;
      }
      break;
    case 12:
      encoding = 0x02;
      if ((res = psiconv_encode_rle12(config,bytes,&bytes_rle))) {
        psiconv_error(config,lev,0,"Error encoding RLE12");
	goto ERROR3;
      }
      break;
    case 16:
      encoding = 0x03;
      if ((res = psiconv_encode_rle16(config,bytes,&bytes_rle))) {
        psiconv_error(config,lev,0,"Error encoding RLE16");
	goto ERROR3;
      }
      break;
    case 24:
      encoding = 0x04;
      if ((res = psiconv_encode_rle24(config,bytes,&bytes_rle))) {
        psiconv_error(config,lev,0,"Error encoding RLE24");
	goto ERROR3;
      }
      break;
    default:
      encoding = 0x00;
  }
  if (encoding) {
    if (psiconv_list_length(bytes_rle) < psiconv_list_length(bytes)) {
      psiconv_list_free(bytes);
      bytes = bytes_rle;
    } else {
      psiconv_list_free(bytes_rle);
      encoding = 0x00;
    }
  }

  if ((res = psiconv_write_u32(config,buf,lev+1,
	                       0x28+psiconv_list_length(bytes))))
    goto ERROR3;
  if ((res = psiconv_write_u32(config,buf,lev+1,0x28)))
    goto ERROR3;
  if ((res = psiconv_write_u32(config,buf,lev+1,value->xsize)))
    goto ERROR3;
  if ((res = psiconv_write_u32(config,buf,lev+1,value->ysize)))
    goto ERROR3;
  if ((res = psiconv_write_length(config,buf,lev+1,value->pic_xsize)))
    goto ERROR3;
  if ((res = psiconv_write_length(config,buf,lev+1,value->pic_ysize)))
    goto ERROR3;
  colordepth = config->colordepth;
  if ((res = psiconv_write_u32(config,buf,lev+1,colordepth)))
    goto ERROR3;
  if ((res = psiconv_write_u32(config,buf,lev+1,(config->color?1:0))))
    goto ERROR3;
  if ((res = psiconv_write_u32(config,buf,lev+1,0)))
    goto ERROR3;
  if ((res = psiconv_write_u32(config,buf,lev+1,encoding)))
    goto ERROR3;
  if (is_clipart) {
    if ((res = psiconv_write_u32(config,buf,lev+1,0xffffffff)))
      goto ERROR3;
    if ((res = psiconv_write_u32(config,buf,lev+1,0x00000044)))
      goto ERROR3;
  }
  for (i = 0; i < psiconv_list_length(bytes); i++) {
    if (!(byteptr = psiconv_list_get(bytes,i)))
      goto ERROR3;
    if ((res = psiconv_write_u8(config,buf,lev+1,*byteptr)))
      goto ERROR3;
  }

ERROR3:
  psiconv_list_free(bytes);
ERROR2:
  psiconv_list_free(ints);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of paint data section failed");
  else
    psiconv_progress(config,lev,0,"End of paint data section");
  return res;
}

/* Translate the floating point RGB information into pixel values.
   The palet is optional; without it, we just use the
   colordepth. With a large palet this is not very fast, but it will do for
   now. For greyscale pictures, just use the palet. */
int psiconv_collect_pixel_data(psiconv_pixel_ints *pixels,int xsize,int ysize, 
                               const psiconv_pixel_floats_t data,
                               int colordepth,int color,
			       int redbits,int bluebits,int greenbits,
			       const psiconv_pixel_floats_t palet)
{
  int res,x,y,i;
  psiconv_u32 index,pixel;
  float p_red,p_green,p_blue,dist,new_dist;

  if (!(*pixels = psiconv_list_new(sizeof(psiconv_u32)))) {
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  for (y = 0; y < ysize; y++) {
    for (x = 0; x < xsize; x++) {
      index = y*xsize+x;
      p_red = data.red[index];
      p_green = data.green[index];
      p_blue = data.blue[index];
      if (!palet.length) {
	if (color) 
	  pixel = (((psiconv_u32) (p_red * (1 << redbits) + 0.5)) 
	                                      << (greenbits+bluebits)) +
	          (((psiconv_u32) (p_green * (1 << greenbits) + 0.5))
		                              << bluebits) +
	          ((psiconv_u32) (p_blue * (1 << bluebits) + 0.5));
	else
	  pixel = (0.212671 * p_red + 0.715160 * p_green + 0.072169 * p_blue) * ((1 << colordepth) * 0.999);
      } else {
	dist = 4; /* Max distance is 3, so this is safe */
	pixel = -1;
	for (i = 0; i < palet.length; i++) {
	  new_dist = (p_red - palet.red[i]) * (p_red - palet.red[i]) +
                     (p_green - palet.green[i]) * (p_green - palet.green[i]) +
		     (p_blue - palet.blue[i]) * (p_blue - palet.blue[i]);
	  if (new_dist < dist) {
	    pixel = i;
	    dist = new_dist;
	  }
	}
      }
      if ((res = psiconv_list_add(*pixels,&pixel)))
	goto ERROR2;
    }
  }
  return 0;

ERROR2:
  psiconv_list_free(*pixels);
ERROR1:
  return res;
}

int psiconv_pixel_data_to_bytes(const psiconv_config config,int lev,
                                psiconv_pixel_bytes *bytes, int xsize,
                                int ysize, const psiconv_pixel_ints pixels, 
				int colordepth)
{
  int res;
  int x,y;

  psiconv_u32 inputdata;
  psiconv_u8 outputbyte;
  psiconv_u32 *pixelptr;
  int inputbitsleft,outputbitnr,bitsfit,outputbytenr;


  if (!bytes) {
    psiconv_error(config,lev,0,"NULL pixel data");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }
  if (!pixels) {
    psiconv_error(config,lev,0,"NULL pixel data");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }
  if (psiconv_list_length(pixels) != xsize * ysize) {
    psiconv_error(config,lev,0,"Pixel number is not correct");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(*bytes = psiconv_list_new(sizeof(psiconv_u8)))) {
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }


  outputbitnr = 0;
  outputbyte = 0;
  for (y = 0; y < ysize; y++) {
    outputbytenr = 0;
    for (x = 0; x < xsize; x++) {
      if (!(pixelptr = psiconv_list_get(pixels,y*xsize+x))) {
	psiconv_error(config,lev,0,"Data structure corruption");
	res = -PSICONV_E_NOMEM;
	goto ERROR2;
      }
      inputbitsleft = colordepth;
      inputdata = *pixelptr;
      while (inputbitsleft) {
	bitsfit = (inputbitsleft+outputbitnr<=8?inputbitsleft:8-outputbitnr);
	outputbyte |= (inputdata & ((1 << bitsfit) - 1)) << outputbitnr;
	inputdata = inputdata >> bitsfit;
	inputbitsleft -= bitsfit;
	outputbitnr += bitsfit;
	if (outputbitnr == 8) {
	  if ((res = psiconv_list_add(*bytes,&outputbyte)))
	    goto ERROR2;
	  outputbitnr = 0;
	  outputbyte = 0;
	  outputbytenr ++;
	}
      }
    }
    /* Always end lines on a long border */
    if (outputbitnr != 0) {
      if ((res = psiconv_list_add(*bytes,&outputbyte)))
	goto ERROR2;
      outputbitnr = 0;
      outputbyte = 0;
      outputbytenr ++;
    }

    while (outputbytenr % 0x04) {
      if ((res = psiconv_list_add(*bytes,&outputbyte)))
	goto ERROR2;
      outputbytenr ++;
    }
  }
  
  return 0;

ERROR2:
  psiconv_list_free(*bytes);
ERROR1:
  return res;
}

/* RLE8 encoding:
     Marker bytes followed by one or more data bytes.
     Marker value 0x00-0x7f: repeat the next data byte (marker+1) times
     Marker value 0xff-0x80: (0x100-marker) data bytes follow */
int psiconv_encode_rle8(const psiconv_config config, 
                        const psiconv_pixel_bytes plain_bytes,
			psiconv_pixel_bytes *encoded_bytes)
{
  int res,i,j,len;
  psiconv_u8 *entry,*next;
  psiconv_u8 temp;

  if (!(*encoded_bytes = psiconv_list_new(sizeof(*entry)))) {
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  for (i = 0; i < psiconv_list_length(plain_bytes);) {
    if (!(entry = psiconv_list_get(plain_bytes,i))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (!(next = psiconv_list_get(plain_bytes,i+1))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (i == psiconv_list_length(plain_bytes) - 2) {
      temp = 0xfe;
      if ((res = psiconv_list_add(*encoded_bytes,&temp)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,next)))
	goto ERROR2;
      i +=2;
    } else if (*next == *entry) {
      len = 1;
      while ((*next == *entry) && 
	     (i+len + 2 < psiconv_list_length(plain_bytes)) &&
	     len < 0x80) {
	len ++;
	if (!(next = psiconv_list_get(plain_bytes,i+len))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
	}
      }
      temp = len - 1;
      if ((res = psiconv_list_add(*encoded_bytes,&temp)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry)))
	goto ERROR2;
      i += len;
    } else {
      len = 1;
      while ((*next != *entry) && 
	     (i+len+1 < psiconv_list_length(plain_bytes)) &&
	     len < 0x80) {
	len ++;
	entry = next;
	if (!(next = psiconv_list_get(plain_bytes,i+len))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
        }
      }
      len --;
      temp = 0x100 - len;
      if ((res = psiconv_list_add(*encoded_bytes,&temp)))
        goto ERROR2;
      for (j = 0; j < len; j++) {
        if (!(next = psiconv_list_get(plain_bytes,i+j))) {
  	  res = -PSICONV_E_NOMEM;
  	  goto ERROR2;
        }
        if ((res = psiconv_list_add(*encoded_bytes,next)))
         goto ERROR2;
      }
      i += len;
    }
  }
  return 0;

ERROR2:
  psiconv_list_free(*encoded_bytes);
ERROR1:
  return res;
}

/* RLE12 encoding:
     Word based. The 12 least significant bits contain the pixel colors.
     the 4 most signigicant bits are the number of repetitions minus 1 */
int psiconv_encode_rle12(const psiconv_config config, 
                        const psiconv_pixel_bytes plain_bytes,
			psiconv_pixel_bytes *encoded_bytes)
{
  typedef psiconv_list psiconv_word_data; /* of psiconv_u16 */
  psiconv_word_data data;
  int res,i,len,location;
  psiconv_u16 *word_entry,*word_next;
  psiconv_u16 word_data;
  psiconv_u8 byte_temp;
  psiconv_u8 *byte_entry;
  

  /* First extract the 12-bit values to encode */
  if (!(data = psiconv_list_new(sizeof(psiconv_u16)))) {
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  for (i = 0; i < psiconv_list_length(plain_bytes); i++) {
    if (!(byte_entry = psiconv_list_get(plain_bytes,i))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    location = 0;
    if (location == 0) {
      word_data = *byte_entry;
      location ++;
    } else if (location == 1) {
      word_data = (word_data << 4) + (*byte_entry & 0x0f);
      if ((res = psiconv_list_add(data,&word_data)))
	goto ERROR2;
      word_data = *byte_entry >> 4;
      location ++;
    } else {
      word_data = (word_data << 8) + *byte_entry;
      if ((res = psiconv_list_add(data,&word_data)))
	goto ERROR2;
      location = 0;
    }
  }

  if (!(*encoded_bytes = psiconv_list_new(sizeof(psiconv_u8)))) {
    res = -PSICONV_E_NOMEM;
    goto ERROR2;
  }

  for (i = 0; i < psiconv_list_length(data);) {
    if (!(word_entry = psiconv_list_get(data,i))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR3;
    }

    if (!(word_next = psiconv_list_get(data,i+1))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR3;
    }

    if (i == psiconv_list_length(data) - 2) {
      byte_temp = *word_entry && 0xff;
      if ((res = psiconv_list_add(*encoded_bytes,&byte_temp)))
        goto ERROR3;
      byte_temp = *word_entry >> 8;
      if ((res = psiconv_list_add(*encoded_bytes,&byte_temp)))
        goto ERROR3;
      byte_temp = *word_next && 0xff;
      if ((res = psiconv_list_add(*encoded_bytes,&byte_temp)))
        goto ERROR3;
      byte_temp = *word_next >> 8;
      if ((res = psiconv_list_add(*encoded_bytes,&byte_temp)))
        goto ERROR3;
      i += 2;
    }
    
    len = 0;
    while ((*word_entry == *word_next) && (len < 16) && 
	   (i+len+1 < psiconv_list_length(data))) {
      len ++;
      if (!(word_next = psiconv_list_get(data,i+len))) {
	res = -PSICONV_E_NOMEM;
	goto ERROR3;
      }
    }

    byte_temp = *word_entry && 0xff;
    if ((res = psiconv_list_add(*encoded_bytes,&byte_temp)))
      goto ERROR3;
    byte_temp = (*word_entry >> 8) + ((len - 1) << 4);
    if ((res = psiconv_list_add(*encoded_bytes,&byte_temp)))
      goto ERROR3;
    i += len;
  }
  return 0;

ERROR3:
  psiconv_list_free(*encoded_bytes);
ERROR2:
  psiconv_list_free(data);
ERROR1:
  return res;
}

/* RLE16 encoding:
     Marker bytes followed by one or more data words.
     Marker value 0x00-0x7f: repeat the next data word (marker+1) times
     Marker value 0xff-0x80: (0x100-marker) data words follow */
int psiconv_encode_rle16(const psiconv_config config, 
                        const psiconv_pixel_bytes plain_bytes,
			psiconv_pixel_bytes *encoded_bytes)
{
  int res,i,j,len;
  psiconv_u8 *entry1,*entry2,*next1,*next2;
  psiconv_u8 temp;

  if (!(*encoded_bytes = psiconv_list_new(sizeof(*entry1)))) {
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  for (i = 0; i < psiconv_list_length(plain_bytes);) {
    if (!(entry1 = psiconv_list_get(plain_bytes,i))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (!(entry2 = psiconv_list_get(plain_bytes,i+1))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (!(next1 = psiconv_list_get(plain_bytes,i+2))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (!(next2 = psiconv_list_get(plain_bytes,i+3))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (i == psiconv_list_length(plain_bytes) - 4) {
      temp = 0xfe;
      if ((res = psiconv_list_add(*encoded_bytes,&temp)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry1)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry2)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,next1)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,next2)))
	goto ERROR2;
      i +=4;
    } else if ((*next1 == *entry1) && (*next2 == *entry2)) {
      len = 0;
      while (((*next1 == *entry1) && (*next2 == *entry2)) && 
	     (i+2*len + 4 < psiconv_list_length(plain_bytes)) &&
	     len < 0x80) {
	len ++;
	if (!(next1 = psiconv_list_get(plain_bytes,i+len*2))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
	}
	if (!(next2 = psiconv_list_get(plain_bytes,i+len*2+1))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
	}
      }
      temp = len - 1;
      if ((res = psiconv_list_add(*encoded_bytes,&temp)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry1)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry2)))
	goto ERROR2;
      i += len*2;
    } else {
      len = 1;
      while (((*next1 != *entry1) || (*next2 != *entry2))&& 
	     (i+len*2+4 < psiconv_list_length(plain_bytes)) &&
	     len < 0x80) {
	len ++;
	entry1 = next1;
	entry2 = next2;
	if (!(next1 = psiconv_list_get(plain_bytes,i+len*2))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
        }
	if (!(next2 = psiconv_list_get(plain_bytes,i+len*2+1))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
        }
      }
      len --;
      temp = 0x100 - len;
      if ((res = psiconv_list_add(*encoded_bytes,&temp)))
        goto ERROR2;
      for (j = 0; j < len; j++) {
        if (!(next1 = psiconv_list_get(plain_bytes,i+j*2))) {
  	  res = -PSICONV_E_NOMEM;
  	  goto ERROR2;
        }
        if (!(next2 = psiconv_list_get(plain_bytes,i+j*2+1))) {
  	  res = -PSICONV_E_NOMEM;
  	  goto ERROR2;
        }
        if ((res = psiconv_list_add(*encoded_bytes,next1)))
         goto ERROR2;
        if ((res = psiconv_list_add(*encoded_bytes,next2)))
         goto ERROR2;
      }
      i += len*2;
    }
  }
  return 0;

ERROR2:
  psiconv_list_free(*encoded_bytes);
ERROR1:
  return res;
}

/* RLE24 encoding:
     Marker bytes followed by one or more data byte-triplets.
     Marker value 0x00-0x7f: repeat the next data byte-triplets (marker+1) times
     Marker value 0xff-0x80: (0x100-marker) data byte-triplets follow */
int psiconv_encode_rle24(const psiconv_config config, 
                        const psiconv_pixel_bytes plain_bytes,
			psiconv_pixel_bytes *encoded_bytes)
{
  int res,i,j,len;
  psiconv_u8 *entry1,*entry2,*entry3,*next1,*next2,*next3;
  psiconv_u8 temp;

  if (!(*encoded_bytes = psiconv_list_new(sizeof(*entry1)))) {
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  for (i = 0; i < psiconv_list_length(plain_bytes);) {
    if (!(entry1 = psiconv_list_get(plain_bytes,i))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (!(entry2 = psiconv_list_get(plain_bytes,i+1))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (!(entry3 = psiconv_list_get(plain_bytes,i+2))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (!(next1 = psiconv_list_get(plain_bytes,i+3))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (!(next2 = psiconv_list_get(plain_bytes,i+4))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (!(next3 = psiconv_list_get(plain_bytes,i+5))) {
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    if (i == psiconv_list_length(plain_bytes) - 6) {
      temp = 0xfe;
      if ((res = psiconv_list_add(*encoded_bytes,&temp)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry1)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry2)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry3)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,next1)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,next2)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,next3)))
	goto ERROR2;
      i +=4;
    } else if ((*next1 == *entry1) && (*next2 == *entry2) && 
	       (*next3 == *entry3)) {
      len = 0;
      while (((*next1 == *entry1) && (*next2 == *entry2) && 
	      (*next3 == *entry3)) &&
	     (i+3*len + 6 < psiconv_list_length(plain_bytes)) &&
	     len < 0x80) {
	len ++;
	if (!(next1 = psiconv_list_get(plain_bytes,i+len*3))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
	}
	if (!(next2 = psiconv_list_get(plain_bytes,i+len*3+1))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
	}
	if (!(next3 = psiconv_list_get(plain_bytes,i+len*3+2))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
	}
      }
      temp = len - 1;
      if ((res = psiconv_list_add(*encoded_bytes,&temp)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry1)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry2)))
	goto ERROR2;
      if ((res = psiconv_list_add(*encoded_bytes,entry3)))
	goto ERROR2;
      i += len*3;
    } else {
      len = 1;
      while (((*next1 != *entry1) || (*next2 != *entry2) || 
	      (*next3 != *entry3)) && 
	     (i+len*3+6 < psiconv_list_length(plain_bytes)) &&
	     len < 0x80) {
	len ++;
	entry1 = next1;
	entry2 = next2;
	entry3 = next3;
	if (!(next1 = psiconv_list_get(plain_bytes,i+len*3))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
        }
	if (!(next2 = psiconv_list_get(plain_bytes,i+len*3+1))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
        }
	if (!(next3 = psiconv_list_get(plain_bytes,i+len*3+2))) {
	  res = -PSICONV_E_NOMEM;
	  goto ERROR2;
        }
      }
      len --;
      temp = 0x100 - len;
      if ((res = psiconv_list_add(*encoded_bytes,&temp)))
        goto ERROR2;
      for (j = 0; j < len; j++) {
        if (!(next1 = psiconv_list_get(plain_bytes,i+j*3))) {
  	  res = -PSICONV_E_NOMEM;
  	  goto ERROR2;
        }
        if (!(next2 = psiconv_list_get(plain_bytes,i+j*3+1))) {
  	  res = -PSICONV_E_NOMEM;
  	  goto ERROR2;
        }
        if (!(next2 = psiconv_list_get(plain_bytes,i+j*3+2))) {
  	  res = -PSICONV_E_NOMEM;
  	  goto ERROR2;
        }
        if ((res = psiconv_list_add(*encoded_bytes,next1)))
         goto ERROR2;
        if ((res = psiconv_list_add(*encoded_bytes,next2)))
         goto ERROR2;
        if ((res = psiconv_list_add(*encoded_bytes,next3)))
         goto ERROR2;
      }
      i += len*3;
    }
  }
  return 0;

ERROR2:
  psiconv_list_free(*encoded_bytes);
ERROR1:
  return res;
}


int psiconv_write_sketch_section(const psiconv_config config,
                                 psiconv_buffer buf, int lev,
				 const psiconv_sketch_section value)
{
  int res;

  psiconv_progress(config,lev,0,"Writing sketch section");
  if (!value) {
    psiconv_error(config,lev,0,"NULL sketch section");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }
  
  if ((res = psiconv_write_u16(config,buf,lev+1,value->displayed_xsize)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,value->displayed_ysize)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,value->picture_data_x_offset)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,value->picture_data_y_offset)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,value->displayed_size_x_offset)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,value->displayed_size_y_offset)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,value->form_xsize)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,value->form_ysize)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,0x0000)))
    goto ERROR1;
  if ((res = psiconv_write_paint_data_section(config,buf,lev+1,value->picture,0)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,value->magnification_x * 0x03e8)))
    goto ERROR1;
  if ((res = psiconv_write_u16(config,buf,lev+1,value->magnification_y * 0x03e8)))
    goto ERROR1;
  if ((res = psiconv_write_u32(config,buf,lev+1,value->cut_left * 0x0c * 
	                                  value->displayed_xsize)))
    goto ERROR1;
  if ((res = psiconv_write_u32(config,buf,lev+1,value->cut_right * 0x0c * 
	                                  value->displayed_xsize)))
    goto ERROR1;
  if ((res = psiconv_write_u32(config,buf,lev+1,value->cut_top * 0x0c * 
	                                  value->displayed_ysize)))
    goto ERROR1;
  if ((res = psiconv_write_u32(config,buf,lev+1,value->cut_bottom * 0x0c * 
	                                  value->displayed_ysize)))
    goto ERROR1;

ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of sketch section failed");
  else
    psiconv_progress(config,lev,0,"End of sketch section");
  return res;
}

int psiconv_write_clipart_section(const psiconv_config config,
                                  psiconv_buffer buf, int lev,
				  const psiconv_clipart_section value)
{
  int res;

  psiconv_progress(config,lev,0,"Writing clipart section");
  if (!value) {
    psiconv_error(config,lev,0, "NULL Clipart Section");
    res = -PSICONV_E_GENERATE;
    goto ERROR;
  }
  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_CLIPART_ITEM)))
    goto ERROR;
  if ((res = psiconv_write_u32(config,buf,lev+1,0x00000002)))
    goto ERROR;
  if ((res = psiconv_write_u32(config,buf,lev+1,0x00000000)))
    goto ERROR;
  if ((res = psiconv_write_u32(config,buf,lev+1,0x00000000)))
    goto ERROR;
  if ((res = psiconv_write_u32(config,buf,lev+1,0x0000000C)))
    goto ERROR;
  if ((res = psiconv_write_paint_data_section(config,buf,lev+1,value->picture,1)))
    goto ERROR;

ERROR:
  if (res)
    psiconv_error(config,lev,0,"Writing of clipart section failed");
  else
    psiconv_progress(config,lev,0,"End of clipart section");
  return res;
}

int psiconv_write_jumptable_section(const psiconv_config config,
                                    psiconv_buffer buf, int lev,
				    const psiconv_jumptable_section value)
{
  int res,i;
  psiconv_u32 *offset_ptr;

  psiconv_progress(config,lev,0,"Writing jumptable section");

  if (!value) {
    psiconv_error(config,lev,0,"NULL Jumptable Section");
    res = -PSICONV_E_GENERATE;
    goto ERROR;
  }
  if ((res = psiconv_write_u32(config,buf,lev+1,psiconv_list_length(value))))
    goto ERROR;
  for (i = 0; i < psiconv_list_length(value); i++) {
    if (!(offset_ptr = psiconv_list_get(value,i))) {
      psiconv_error(config,lev,0,"Massive memory corruption");
      res = -PSICONV_E_NOMEM;
      goto ERROR;
    }
    if ((res = psiconv_write_offset(config,buf,lev+1,*offset_ptr)))
      goto ERROR;
  }

ERROR:
  if (res)
    psiconv_error(config,lev,0,"Writing of jumptable section failed");
  else
    psiconv_progress(config,lev,0,"End of jumptable section");
  return res;
}
