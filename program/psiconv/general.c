/*
    general.c - Part of psiconv, a PSION 5 file formats converter
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

#include "psiconv.h"
#include "general.h"
#include <psiconv/list.h>
#include <psiconv/unicode.h>
#include <psiconv/error.h>
#include <stdlib.h>
#include <string.h>

/* Output a UCS2 character in one of the supported encodings. */
void output_char(psiconv_config config, psiconv_list list, 
                psiconv_ucs2 character, encoding enc)
{
  psiconv_u8 temp;
  psiconv_u8 *byteptr;
  int res,i;
  psiconv_buffer buf;
#define TEMPSTR_LEN 80
  char tempstr[TEMPSTR_LEN];

  if (enc == ENCODING_UCS2) {
    temp = character >> 8;
    if ((res = psiconv_list_add(list,&temp))) {
      fputs("Out of memory error\n",stderr);
      exit(1);
    }
    temp = character & 0xff;
    if ((res = psiconv_list_add(list,&temp))) {
      fputs("Out of memory error\n",stderr);
      exit(1);
    }
  } else if (enc == ENCODING_UTF8) {
    if (character < 0x80) {
      temp = character;
      if ((res = psiconv_list_add(list,&temp))) {
	fputs("Out of memory error\n",stderr);
	exit(1);
      }
    } else if (character < 0x800) {
      temp = 0xc0 + (character >> 6);
      if ((res = psiconv_list_add(list,&temp))) {
	fputs("Out of memory error\n",stderr);
	exit(1);
      }
      temp = 0x80 + (character & 0x3f);
      if ((res = psiconv_list_add(list,&temp))) {
	fputs("Out of memory error\n",stderr);
	exit(1);
      }
    } else {
      temp = 0xe0 + (character >> 12);
      if ((res = psiconv_list_add(list,&temp))) {
	fputs("Out of memory error\n",stderr);
	exit(1);
      }
      temp = 0x80 + ((character >> 6) & 0x3f);
      if ((res = psiconv_list_add(list,&temp))) {
	fputs("Out of memory error\n",stderr);
	exit(1);
      }
      temp = 0x80 + (character  & 0x3f);
      if ((res = psiconv_list_add(list,&temp))) {
	fputs("Out of memory error\n",stderr);
	exit(1);
      }
    }
  } else if (enc == ENCODING_ASCII) {
    if (character == 0xa0)
      temp = ' ';
    else if (character >= 0x80)
      temp = '?';
    else
      temp = character;
    if ((res = psiconv_list_add(list,&temp))) {
      fputs("Out of memory error\n",stderr);
      exit(1);
    }
  } else if (enc == ENCODING_ASCII_HTML) {
    if (character >= 0x80) {
      snprintf(tempstr,TEMPSTR_LEN,"&#x%x;",character);
      output_simple_chars(config,list,tempstr,enc);
    } else {
      temp = character;
      if ((res = psiconv_list_add(list,&temp))) {
	fputs("Out of memory error\n",stderr);
	exit(1);
      }
    }
  } else if (enc == ENCODING_PSION) {
    if (!(buf = psiconv_buffer_new())) {
      fputs("Out of memory error\n",stderr);
      exit(1);
    }
    psiconv_unicode_write_char(config,buf,0,character);
    for (i = 0; i < psiconv_buffer_length(buf); i++) {
      if (!(byteptr = psiconv_buffer_get(buf,i))) {
        fputs("Internal memory corruption\n",stderr);
        exit(1);
      }
      if ((res = psiconv_list_add(list,byteptr))) {
	fputs("Out of memory error\n",stderr);
	exit(1);
      }
    }
    psiconv_buffer_free(buf);
  }
}

void output_string(psiconv_config config, psiconv_list list, 
                          psiconv_ucs2 *string, encoding enc)
{
  int i = 0;

  while (string[i]) {
    output_char(config,list,string[i],enc);
    i++;
  }
}

void output_simple_chars(psiconv_config config, psiconv_list list,
                        char *string, encoding enc)
{
  psiconv_ucs2 *ucs_string;
  int i;

  if (!(ucs_string = malloc(sizeof(*ucs_string) * (strlen(string) + 1)))) {
    fputs("Out of memory error",stderr);
    exit(1);
  }
  for (i = 0; i < strlen(string); i++)  {
    if ((string[i] != '\n') && ((string[i] < 0x20) || (string[i] > 0x7e))) {
      fprintf(stderr,"output_simple_chars unknown char: %02x",string[i]);
      exit(1);
    }
    ucs_string[i] = string[i];
  }
  ucs_string[i] = string[i];
  output_string(config,list,ucs_string,enc);
  free(ucs_string);
}
