/*
    unicode.c - Part of psiconv, a PSION 5 file formats converter
    Copyright (c) 2003-2014  Frodo Looijaard <frodo@frodo.looijaard.name>

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
#include "error.h"

#include "unicode.h"
#include "parse_routines.h"
#include "generate_routines.h"

#include <string.h>
#include <stdlib.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif



psiconv_ucs2 table_cp1252[0x100] =
  {
  /* 0x00 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0006, 0x0007,
  /* 0x08 */ 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
  /* 0x10 */ 0x00a0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  /* 0x18 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  /* 0x20 */ 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
  /* 0x28 */ 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
  /* 0x30 */ 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  /* 0x38 */ 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
  /* 0x40 */ 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
  /* 0x48 */ 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
  /* 0x50 */ 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
  /* 0x58 */ 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
  /* 0x60 */ 0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
  /* 0x68 */ 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
  /* 0x70 */ 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
  /* 0x78 */ 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x0000,
  /* 0x80 */ 0x20ac, 0x0000, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
  /* 0x88 */ 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017d, 0x0000,
  /* 0x90 */ 0x0000, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
  /* 0x98 */ 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x0000, 0x017e, 0x0178,
  /* 0xa0 */ 0x0000, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
  /* 0xa8 */ 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
  /* 0xb0 */ 0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
  /* 0xb8 */ 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
  /* 0xc0 */ 0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
  /* 0xd8 */ 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
  /* 0xd0 */ 0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
  /* 0xe8 */ 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
  /* 0xe0 */ 0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
  /* 0xc8 */ 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
  /* 0xf0 */ 0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
  /* 0xf8 */ 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff 
  };



/* TODO: Check the charset number, select the correct one */
extern int psiconv_unicode_select_characterset(const psiconv_config config,
                                               int charset)
{
  switch(charset) {
    case 0: config->unicode = psiconv_bool_true; 
	    break;
    case 1: config->unicode = psiconv_bool_false;
            memcpy(config->unicode_table,table_cp1252,
		   sizeof(psiconv_ucs2) * 0x100);
	    break;
    default: return -1;
  }
  return 0;
}


psiconv_ucs2 psiconv_unicode_read_char(const psiconv_config config,
                                       psiconv_buffer buf,
				       int lev,psiconv_u32 off,
				       int *length,int *status)
{
  psiconv_u8 char1,char2,char3;
  psiconv_ucs2 result=0;
  int res;
  int len=0;

  char1 = psiconv_read_u8(config,buf,lev,off+len,&res);
  if (res)
    goto ERROR;
  len ++;

  if (config->unicode) {
    if (char1 >= 0xf0) {
      res = PSICONV_E_PARSE;
      goto ERROR;
    } else if (char1 < 0x80)
      result = char1;
    else {
      char2 = psiconv_read_u8(config,buf,lev,off+len,&res);
      len ++;
      if ((char2 & 0xc0) != 0x80) {
        res = PSICONV_E_PARSE;
        goto ERROR;
      }
      if (char1 < 0xe0) 
	result = ((char1 & 0x1f) << 6) | (char2 & 0x3f);
      else {
        char3 = psiconv_read_u8(config,buf,lev,off+len,&res);
	len ++;
	if ((char3 & 0xc0) != 0x80) {
	  res = PSICONV_E_PARSE;
	  goto ERROR;
	}
	result = ((char1 & 0x0f) << 12) | ((char2 & 0x3f) << 6) | 
	         (char3 & 0x3f);
      }
    }
  } else
    result = config->unicode_table[char1]?config->unicode_table[char1]:
                                          config->unknown_unicode_char;
ERROR:
  if (length)
    *length = len;
  if (status)
    *status = res;
  return result;
}

int psiconv_unicode_write_char(const psiconv_config config,
                               psiconv_buffer buf,
			       int lev, psiconv_ucs2 value)
{
  int i;
  int res=0;

  if (config->unicode) {
    if (value < 0x80) {
      if ((res = psiconv_write_u8(config,buf,lev,value)))
	goto ERROR;
    } else if (value < 0x800) {
      if ((res = psiconv_write_u8(config,buf,lev,0xc0 | (value >> 6))))
	goto ERROR;
      if ((res = psiconv_write_u8(config,buf,lev,0x80 | (value & 0x3f))))
	goto ERROR;
    } else {
      if ((res = psiconv_write_u8(config,buf,lev,0xe0 | (value >> 12))))
	goto ERROR;
      if ((res = psiconv_write_u8(config,buf,lev,0x80 | ((value >> 6) & 0x3f))))
	goto ERROR;
      if ((res = psiconv_write_u8(config,buf,lev,0x80 | (value & 0x3f))))
	goto ERROR;
    }
  } else {
    for (i = 0; i < 256; i++) 
      if (config->unicode_table[i] == value)
	break;
    if ((res = psiconv_write_u8(config,buf,lev,
	                        i == 256?config->unknown_epoc_char:i)))
      goto ERROR;
  }
ERROR:
  return res;
}

int psiconv_unicode_strlen(const psiconv_ucs2 *input)
{
  int i = 0;
  while (input[i])
    i++;
  return i;
}

psiconv_ucs2 *psiconv_unicode_strdup(const psiconv_ucs2 *input)
{
  psiconv_ucs2 *output;
  int i = 0;

  if (!(output = malloc(sizeof(*output) * 
	                 (1 + psiconv_unicode_strlen(input)))))
     return NULL;
  while ((output[i] = input[i]))
    i++;
  return output;
}

int psiconv_unicode_strcmp(const psiconv_ucs2 *str1, const psiconv_ucs2 *str2)
{
  int i = 0;
  while (str1[i] && str2[i]) {
    if (str1[i] < str2[i])
      return -1;
    if (str1[i] > str2[i])
      return 1;
    i++;
  }
  if (str1[i] < str2[i])
    return -1;
  else if (str1[i] > str2[i])
     return 1;
  else
    return 0;
}


psiconv_ucs2 *psiconv_unicode_empty_string(void)
{
  psiconv_ucs2 *result;
  result = malloc(sizeof(psiconv_ucs2));
  if (result)
    result[0] = 0;
  return result;
}


psiconv_ucs2 *psiconv_unicode_from_list(psiconv_list input)
{
  psiconv_ucs2 *result;
  int i;
  psiconv_ucs2 *character;

  if (!(result = malloc(sizeof(psiconv_ucs2) * (psiconv_list_length(input)+1))))
    goto ERROR1;
  for (i = 0; i < psiconv_list_length(input); i++) {
    if (!(character = psiconv_list_get(input,i)))
      goto ERROR2;
    result[i] = *character;
  }
  result[i] = 0;
  return result;

ERROR2:
  free(result);
ERROR1:
  return NULL;
}


psiconv_ucs2 *psiconv_unicode_strstr(const psiconv_ucs2 *haystack, 
                                     const psiconv_ucs2 *needle)
{
  int i,j,haystack_len,needle_len;
  haystack_len = psiconv_unicode_strlen(haystack);
  needle_len = psiconv_unicode_strlen(needle);



  for (i = 0; i < haystack_len - needle_len + 1; i++) {
    for (j = 0; j < needle_len; j++) 
      if (haystack[i+j] != needle[j])
	break;
    if (j == needle_len)
      return (psiconv_ucs2 *) haystack+i;
  }
  return NULL;
}
