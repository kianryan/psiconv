/*
    generate_simple.c - Part of psiconv, a PSION 5 file formats converter
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


#include "config.h"
#include "compat.h"

#include <string.h>
#include <stdlib.h>

#include "generate_routines.h"
#include "error.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static int psiconv_write_string_aux(const psiconv_config config,
                                    psiconv_buffer buf, int lev,
				    const psiconv_string_t value,int kind);

int psiconv_write_u8(const psiconv_config config,psiconv_buffer buf,
                     int lev,const psiconv_u8 value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing u8");
  psiconv_debug(config,lev+1,0,"Value: %02x",value);
  res = psiconv_buffer_add(buf,value);
  if (res)
    psiconv_error(config,lev,0,"Out of memory error");
  return res;
}

int psiconv_write_u16(const psiconv_config config,psiconv_buffer buf,
                      int lev,const psiconv_u16 value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing u16");
  psiconv_debug(config,lev+1,0,"Value: %04x",value);
  if ((res = psiconv_buffer_add(buf,value & 0xff))) 
    goto ERROR;
  if ((res = psiconv_buffer_add(buf,(value & 0xff00) >> 8)))
    goto ERROR;
ERROR:
  if (res)
    psiconv_error(config,lev,0,"Out of memory error");
  return res;
}

int psiconv_write_u32(const psiconv_config config,psiconv_buffer buf,
                      int lev,const psiconv_u32 value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing u32");
  psiconv_debug(config,lev+1,0,"Value: %08x",value);

  if ((res = psiconv_buffer_add(buf,value & 0xff))) 
    goto ERROR;
  if ((res = psiconv_buffer_add(buf,(value & 0xff00) >> 8))) 
    goto ERROR;
  if ((res = psiconv_buffer_add(buf,(value & 0xff0000) >> 16))) 
    goto ERROR;
  if ((res = psiconv_buffer_add(buf,(value & 0xff000000) >> 24)))
    goto ERROR;
ERROR:
  if (res)
    psiconv_error(config,lev,0,"Out of memory error");
  return res;
}

int psiconv_write_S(const psiconv_config config,psiconv_buffer buf, 
                    int lev,const psiconv_u32 value)
{
  int res;

  psiconv_progress(config,lev,0,"Writing S");
  psiconv_debug(config,lev+1,0,"Value: %08x",value);
  if (value < 0x40) 
    res = psiconv_write_u8(config,buf,lev+2,value * 4 + 2);
  else if (value < 0x2000) 
    res = psiconv_write_u16(config,buf,lev+2,value * 8 + 3);
  else {
    psiconv_error(config,0,psiconv_buffer_length(buf),
                 "Don't know how to write S value larger than 0x2000 "
                 "(trying %x)",value);
    res = -PSICONV_E_GENERATE;
  }
  if (res)
    psiconv_error(config,lev,0,"Writing of S failed");
  else
    psiconv_progress(config,lev,0,"End of S");
  return res;
}

int psiconv_write_X(const psiconv_config config,psiconv_buffer buf,
                    int lev, const psiconv_u32 value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing X");
  psiconv_debug(config,lev+1,0,"Value: %08x",value);
  if (value < 0x80) 
    res = psiconv_write_u8(config,buf,lev+2,value * 2);
  else if (value < 0x4000) 
    res = psiconv_write_u16(config,buf,lev+2,value * 4 + 1);
  else if (value < 0x20000000) 
    res = psiconv_write_u16(config,buf,lev+2,value * 8 + 3);
  else {
    psiconv_error(config,lev,0,
                 "Don't know how to write X value larger than 0x20000000 "
                 "(trying %x)",value);
    res = -PSICONV_E_GENERATE;
  }
  if (res)
    psiconv_error(config,lev,0,"Writing of X failed");
  else
    psiconv_progress(config,lev,0,"End of X");
  return res;
}

int psiconv_write_length(const psiconv_config config,psiconv_buffer buf, 
                         int lev,const psiconv_length_t value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing length");
  psiconv_debug(config,lev+1,0,"Value: %f",value);
  res = psiconv_write_u32(config,buf,lev+2,value * (1440.0/2.54) + 0.5);
  if (res)
    psiconv_error(config,lev,0,"Writing of length failed");
  else
    psiconv_progress(config,lev,0,"End of length");
  return res;
}

int psiconv_write_size(const psiconv_config config,psiconv_buffer buf,
                       int lev, psiconv_size_t value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing size");
  psiconv_debug(config,lev+1,0,"Value: %f",value);
  res = psiconv_write_u32(config,buf,lev+2,value * 20.0 + 0.5);
  if (res)
    psiconv_error(config,lev,0,"Writing of size failed");
  else
    psiconv_progress(config,lev,0,"End of size");
  return res;
}

int psiconv_write_bool(const psiconv_config config,psiconv_buffer buf, 
                       int lev,const psiconv_bool_t value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing bool");
  psiconv_debug(config,lev+1,0,"Value: %s",
                value == psiconv_bool_false?"False":"True");
  if ((value != psiconv_bool_true) && (value != psiconv_bool_false)) 
    psiconv_warn(config,0,psiconv_buffer_length(buf),
                 "Boolean has non-enum value (found %d, used true)",value);
  res = psiconv_write_u8(config,buf,lev+2,value == psiconv_bool_false?0:1);
  if (res)
    psiconv_error(config,lev,0,"Writing of bool failed");
  else
    psiconv_progress(config,lev,0,"End of bool");
  return res;
}

int psiconv_write_string(const psiconv_config config,psiconv_buffer buf,
                         int lev, const psiconv_string_t value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing string");
  res = psiconv_write_string_aux(config,buf,lev+1,value,-1);
  if (res)
    psiconv_error(config,lev,0,"Writing of string failed");
  else
    psiconv_progress(config,lev,0,"End of string");
  return res;
}

int psiconv_write_short_string(const psiconv_config config,psiconv_buffer buf, 
                               int lev,const psiconv_string_t value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing short string");
  res = psiconv_write_string_aux(config,buf,lev+1,value,-2);
  if (res)
    psiconv_error(config,lev,0,"Writing of short string failed");
  else
    psiconv_progress(config,lev,0,"End of short string");
  return res;
}

int psiconv_write_charlist(const psiconv_config config,psiconv_buffer buf, 
                           int lev,const psiconv_string_t value)
{
  int res;
  psiconv_progress(config,lev,0,"Writing charlist");
  res = psiconv_write_string_aux(config,buf,lev+1,value,0);
  if (res)
    psiconv_error(config,lev,0,"Writing of charlist failed");
  else
    psiconv_progress(config,lev,0,"End of charlist");
  return res;
}


int psiconv_write_string_aux(const psiconv_config config,psiconv_buffer buf,
                             int lev, const psiconv_string_t value,int kind)
{
  int res,i,len;
  char *printable;

  len = psiconv_unicode_strlen(value);
  if (!value) {
    psiconv_error(config,lev,0, "NULL string");
    return -PSICONV_E_GENERATE;
  }

  if (!(printable = psiconv_make_printable(config,value))) {
    psiconv_error(config,lev,0,"Out of memory error");
    return -PSICONV_E_NOMEM;
  }
  psiconv_debug(config,lev+1,0,"Value: %s",printable);
  free(printable);

  if (kind == -1) 
    res = psiconv_write_S(config,buf,lev+2,len);
  else if (kind == -2)
    res = psiconv_write_u8(config,buf,lev+2,len);
  else
    res = 0;
  if (res)
    return res;

  for (i = 0; i < len; i++) 
    if ((res = psiconv_unicode_write_char(config,buf,lev+2,value[i]))) 
      return res;
  return -PSICONV_E_OK;
}

int psiconv_write_offset(const psiconv_config config,psiconv_buffer buf, 
                         int lev,psiconv_u32 id)
{
  int res;
  psiconv_progress(config,lev,0,"Writing offset");
  psiconv_debug(config,lev+1,0,"ID: %08x",id);
  res = psiconv_buffer_add_reference(buf,id);
  if (res)
    psiconv_error(config,lev,0,"Out of memory error");
  return res;
}
