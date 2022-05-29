/*
    parse_simple.c - Part of psiconv, a PSION 5 file formats converter
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
#include <math.h>

#include "parse_routines.h"
#include "error.h"
#include "unicode.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static psiconv_float_t pow2(int n);
static psiconv_string_t psiconv_read_string_aux(const psiconv_config config,
                                     const psiconv_buffer buf,int lev,
                                     psiconv_u32 off,int *length, int *status,
				     int kind);

/* Very inefficient, but good enough for now. By implementing it ourselves,
   we do not have to link with -lm */
psiconv_float_t pow2(int n)
{
  psiconv_float_t res=1.0;
  int i;

  for (i = 0; i < (n<0?-n:n); i++) 
    res *= 2.0;

  return n<0?1/res:res;
}
psiconv_u8 psiconv_read_u8(const psiconv_config config,const psiconv_buffer buf,int lev,psiconv_u32 off,
                           int *status)
{
  psiconv_u8 *ptr;
  ptr = psiconv_buffer_get(buf,off);
  if (!ptr) {
    psiconv_error(config,lev,off,"Trying byte read past the end of the file");
    if (status)
      *status = -PSICONV_E_PARSE;
    return 0;
  }
  if (status)
    *status = 0;
  return *ptr;
}

psiconv_u16 psiconv_read_u16(const psiconv_config config,const psiconv_buffer buf,int lev,psiconv_u32 off,
                             int *status)
{
  psiconv_u8 *ptr0,*ptr1;
  ptr0 = psiconv_buffer_get(buf,off);
  ptr1 = psiconv_buffer_get(buf,off+1);
  if (!ptr0 || !ptr1) {
    psiconv_error(config,lev,off,"Trying word read past the end of the file");
    if (status)
      *status = -PSICONV_E_PARSE;
    return 0;
  }
  if (status)
    *status = 0;
  return *ptr0 + (*ptr1 << 8);
}

psiconv_u32 psiconv_read_u32(const psiconv_config config,const psiconv_buffer buf,int lev,psiconv_u32 off,
                             int *status)
{
  psiconv_u8 *ptr0,*ptr1,*ptr2,*ptr3;
  ptr0 = psiconv_buffer_get(buf,off);
  ptr1 = psiconv_buffer_get(buf,off+1);
  ptr2 = psiconv_buffer_get(buf,off+2);
  ptr3 = psiconv_buffer_get(buf,off+3);
  if (!ptr0 || !ptr1 || !ptr2 || !ptr3) {
    psiconv_error(config,lev,off,"Trying long read past the end of the file");
    if (status)
      *status = -PSICONV_E_PARSE;
    return 0;
  }
  if (status)
    *status = 0;
  return *ptr0 + (*ptr1 << 8) + (*ptr2 << 16) + (*ptr3 << 24);
}

psiconv_s32 psiconv_read_sint(const psiconv_config config,const psiconv_buffer buf,int lev,psiconv_u32 off,
                              int *length,int *status)
{
  int localstatus;
  psiconv_u32 temp;

  temp=psiconv_read_u32(config,buf,lev,off,&localstatus);
  if (status) 
    *status = localstatus;
  if (length)
    *length = localstatus?0:4;

  return localstatus?0:(temp & 0x7fffffff)*(temp&0x80000000?-1:1);
}

psiconv_u32 psiconv_read_S(const psiconv_config config,const psiconv_buffer buf, int lev, psiconv_u32 off,
                           int *length,int *status)
{
  psiconv_u8 temp;
  psiconv_u32 res;
  int len,localstatus;

  psiconv_progress(config,lev+1,off,"Going to read a S length indicator");
  temp = psiconv_read_u8(config,buf,lev+2,off,&localstatus);
  if (localstatus)
    goto ERROR;
  if ((temp & 0x03) == 0x02) {
    res = psiconv_read_u8(config,buf,lev+2,off,&localstatus) >> 2;
    if (localstatus)
      goto ERROR;
    len = 1;
    psiconv_debug(config,lev+2,off,"Indicator (1 byte): %02x",res);
  } else if ((temp & 0x07) == 0x05) {
    res = psiconv_read_u16(config,buf,lev+2,off,&localstatus) >> 3;
    if (localstatus)
      goto ERROR;
    len = 2;
    psiconv_debug(config,lev+2,off,"Indicator (2 bytes): %04x",res);
  } else {
    psiconv_error(config,lev+2,off,"S indicator: unknown encoding!");
    psiconv_debug(config,lev+2,off,"Raw data first byte: %02x",temp);
    goto ERROR;
  }

  if (length)
    *length = len;
  if (status)
    *status = 0;

  psiconv_progress(config,lev+1,off+len-1,
                   "End of S length indicator (total length: %08x)", len);

  return res;

ERROR:
  psiconv_error(config,lev+1,off,"Reading of S indicator failed");
  if (status)
    *status = localstatus;
  if (length)
     *length = 0;
  return 0;
}

psiconv_u32 psiconv_read_X(const psiconv_config config,const psiconv_buffer buf, int lev, psiconv_u32 off,
                           int *length, int *status)
{
  psiconv_u8 temp;
  psiconv_u32 res;
  int len,localstatus;

  psiconv_progress(config,lev+1,off,"Going to read a X length indicator");
  temp = psiconv_read_u8(config,buf,lev+2,off,&localstatus);
  if (localstatus)
    goto ERROR;
  if ((temp & 0x01) == 0x00) {
    res = psiconv_read_u8(config,buf,lev+2,off,&localstatus) >> 1;
    if (localstatus)
      goto ERROR;
    len = 1;
    psiconv_debug(config,lev+2,off,"Indicator (1 byte): %02x",res);
  } else if ((temp & 0x03) == 0x01) {
    res = psiconv_read_u16(config,buf,lev+2,off,&localstatus) >> 2;
    if (localstatus)
      goto ERROR;
    len = 2;
    psiconv_debug(config,lev+2,off,"Indicator (2 bytes): %04x",res);
  } else if ((temp & 0x07) == 0x03) {
    res = psiconv_read_u32(config,buf,lev+2,off,&localstatus) >> 3;
    if (localstatus)
      goto ERROR;
    len = 4;
    psiconv_debug(config,lev+2,off,"Indicator (4 bytes): %08x",res);
  } else {
    psiconv_error(config,lev+2,off,"X indicator: unknown encoding!");
    psiconv_debug(config,lev+2,off,"Raw data first byte: %02x",temp);
    goto ERROR;
  }

  if (length)
    *length = len;
  if (status)
    *status = 0;

  psiconv_progress(config,lev+1,off+len-1,
                   "End of X length indicator (total length: %08x)", len);

  return res;

ERROR:
  psiconv_error(config,lev+1,off,"Reading of X indicator failed");
  if (status)
    *status = localstatus;
  if (length)
     *length = 0;
  return 0;
}

psiconv_length_t psiconv_read_length(const psiconv_config config,const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length, int *status)
{
  psiconv_length_t res;
  int localstatus;

  res = (2.54/1440.0) * ((psiconv_s32) psiconv_read_u32(config,buf,lev,off,
                                                        &localstatus));
  if (localstatus) {
    psiconv_error(config,lev+1,off,"Reading of length failed");
    if (length)
      *length = 0;
    if (status)
       *status = localstatus;
     return 0;
  }
  psiconv_debug(config,lev+1,off,"Length: %f",res);
  if (length)
    *length = 4;
  if (status)
     *status = 0;
  return res;
}

psiconv_size_t psiconv_read_size(const psiconv_config config,const psiconv_buffer buf, int lev,
                                 psiconv_u32 off, int *length, int *status)
{
  psiconv_size_t res;
  int localstatus;
  res = ((psiconv_s32) psiconv_read_u32(config,buf,lev,off,&localstatus)) / 20.0;
  if (localstatus) {
    psiconv_error(config,lev+1,off,"Reading of size failed");
    if (length)
      *length = 0;
    if (status)
       *status = localstatus;
     return 0;
  }
  psiconv_debug(config,lev+1,off,"Size: %f",res);
  if (status)
     *status = 0;
  if (length)
    *length = 4;
  return res;
}

int psiconv_parse_bool(const psiconv_config config,const psiconv_buffer buf, int lev, psiconv_u32 off, 
                       int *length, psiconv_bool_t *result)
{
  psiconv_u8 temp;
  int localstatus;
  temp = psiconv_read_u8(config,buf,lev,off,&localstatus);
  if (localstatus) {
    psiconv_error(config,lev+1,off,"Reading of bool failed");
    if (length)
      *length = 0;
     return localstatus;
  }
  if (length)
    *length = 1;
  if (temp == 0) {
    *result = psiconv_bool_false;
    return 0;
  } else if (temp == 1) {
    *result = psiconv_bool_true;
    return 0;
  }
  psiconv_warn(config,lev+1,off,"Unknown value for boolean");
  psiconv_debug(config,lev+1,off,"Boolean value: %02x",temp);
  *result = psiconv_bool_true;
  return 0;
}

psiconv_string_t psiconv_read_string(const psiconv_config config,
                                     const psiconv_buffer buf,int lev,
                                     psiconv_u32 off,int *length, int *status)
{
  return psiconv_read_string_aux(config,buf,lev,off,length,status,-1);
}

psiconv_string_t psiconv_read_short_string(const psiconv_config config,
                                     const psiconv_buffer buf,int lev,
                                     psiconv_u32 off,int *length, int *status)
{
  return psiconv_read_string_aux(config,buf,lev,off,length,status,-2);
}

psiconv_string_t psiconv_read_charlist(const psiconv_config config,
                                       const psiconv_buffer buf, int lev,
				       psiconv_u32 off, int nrofchars,
				       int *status)
{
  int length;
  if (nrofchars <= 0) {
    psiconv_error(config,lev,off,
	          "psiconv_read_charlist called with non-positive nrofchars");
    if (status)
      *status = -PSICONV_E_OTHER;
    return NULL;
  }
  return psiconv_read_string_aux(config,buf,lev,off,&length,status,nrofchars);
}


psiconv_string_t psiconv_read_string_aux(const psiconv_config config,
                                     const psiconv_buffer buf,int lev,
                                     psiconv_u32 off,int *length, int *status,
				     int kind)
{
  int bytecount,i,leng,len,localstatus;
  psiconv_string_t result;
  char *res_copy;
  psiconv_list string;
  psiconv_ucs2 nextchar;
  psiconv_ucs2 *nextcharptr;

  psiconv_progress(config,lev+1,off,"Going to read a string");

  if (kind == -1) 
    bytecount = psiconv_read_S(config,buf,lev+2,off,&leng,&localstatus);
  else if (kind == -2) {
    bytecount = psiconv_read_u8(config,buf,lev+2,off,&localstatus);
    leng = 1;
  } else {
    bytecount = kind;
    leng = 0;
    localstatus = 0;
  }
  if (localstatus)
    goto ERROR1;
  psiconv_debug(config,lev+2,off,"Length: %i",bytecount);
  len = leng;

  if (!(string = psiconv_list_new(sizeof(*result))))
    goto ERROR1;

  /* Read the string into a temporary list */
  i = 0;
  while (i < bytecount) {
    nextchar = psiconv_unicode_read_char(config,buf,lev,off+i+len,
	                                  &leng,&localstatus);
    if (localstatus)
      goto ERROR2;
    if ((localstatus = psiconv_list_add(string,&nextchar)))
      goto ERROR2;
    i += leng;
  }
  if (i > bytecount) {
    psiconv_error(config,lev,off+i+len,"Malformed string");
    localstatus = PSICONV_E_PARSE;
    goto ERROR2;
  }
  len += bytecount;

  /* Copy the list to the actual string */
  if (!(result = malloc(sizeof(*result) * (psiconv_list_length(string) + 1))))
    goto ERROR2;
  for (i = 0; i < psiconv_list_length(string); i++) {
    if (!(nextcharptr = psiconv_list_get(string,i))) {
      psiconv_error(config,lev,off+i+len,"Data structure corruption");
      goto ERROR3;
    }
    result[i] = *nextcharptr;
  }
  result[i] = 0;

  res_copy = psiconv_make_printable(config,result);
  if (!res_copy)
    goto ERROR3;
  psiconv_debug(config,lev+2,off,"Contents: `%s'",res_copy);
  free(res_copy);

  psiconv_list_free(string);

  if (length)
    *length = len;

  if (status)
    *status = 0;

  psiconv_progress(config,lev+1,off+len-1,"End of string (total length: %08x)",len);

  return result;

ERROR3:
  free(result);
ERROR2:
  psiconv_list_free(string);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of string failed");
  if (status)
    *status = localstatus;
  if (length)
    *length = 0;
  return NULL;
}

psiconv_float_t psiconv_read_float(const psiconv_config config,const psiconv_buffer buf, int lev,
                                   psiconv_u32 off, int *length, int *status)
{
  psiconv_float_t result,bitvalue;
  int res,bit;
  psiconv_u32 temp=0;

  psiconv_progress(config,lev+1,off,"Going to read a float");

  bitvalue = 0.5;
  result = 1.0;
  for (bit = 0x33; bit > 0; bit--) {
    if ((bit == 0x33) || ((bit & 0x07) == 0x07))
      temp = psiconv_read_u8(config,buf,lev+2,off+ (bit >> 3),&res);
    if (res)
      goto ERROR;
    if (temp & (0x01 << (bit & 0x07)))
      result += bitvalue;
    bitvalue /= 2.0;
  }
  temp = psiconv_read_u16(config,buf,lev+2,off+6,&res);
  if (res)
    goto ERROR;
  if (temp & 0x8000)
    result = -result;
  temp = (temp & 0x7ff0) >> 4;
  result *= pow2(((int) temp)-0x3ff);
  psiconv_debug(config,lev+1,off,"Float value: %f",result);
  if (length)
    *length = 8;
  if (*status)
    *status = res;
  return result;
ERROR:
  psiconv_error(config,lev+1,off,"Reading of float failed");
  if (length)
    *length = 0;
  if (*status)
    *status = res;
  return 0.0;
}

