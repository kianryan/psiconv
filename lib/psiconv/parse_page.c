/*
    parse_page.c - Part of psiconv, a PSION 5 file formats converter
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
#include <string.h>

#include "parse_routines.h"
#include "error.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif


int psiconv_parse_page_header(const psiconv_config config,
                              const psiconv_buffer buf,int lev,psiconv_u32 off,
                              int *length,psiconv_page_header *result)
{
  int res = 0;
  int len = 0;
  int i,leng,has_content;
  psiconv_u32 temp;

  psiconv_progress(config,lev+1,off,"Going to read a page header (or footer)");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the has_content flag");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp == 0x00)
    has_content = 0;
  else if (temp == 0x01)
    has_content = 1;
  else {
    psiconv_warn(config,lev+2,off+len,
               "Page header has_content flag unknown value (assumed default)");
    psiconv_debug(config,lev+2,off+len,"Flag: %02x",temp);
    has_content = 1;
  }
  psiconv_debug(config,lev+2,off+len,"Has_content flag: %02x",has_content);
  len += 1;

  psiconv_progress(config,lev+2,off+len,"Going to read displayed-on-first-page flag");
  if ((res = psiconv_parse_bool(config,buf,lev+2,off+len,&leng,
                                &(*result)->on_first_page)))
    goto ERROR2;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read three zero bytes");
  for (i = 0; i < 0x03; i++,len++) {
    temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR2;
    if (temp != 0x00) {
      psiconv_warn(config,lev+2,off+len,
                   "Page Header unknown value in zero bytes section");
      psiconv_debug(config,lev+2,off+len,"Byte %d: read %02x, expected %02x",
                    i,temp,0x00);
    }
  }

  psiconv_progress(config,lev+2,off+len,"Going to read base paragraph layout");
  if (!((*result)->base_paragraph_layout = psiconv_basic_paragraph_layout()))
    goto ERROR2;

  if (has_content) {
    if ((res = psiconv_parse_paragraph_layout_list(config,buf,lev+2,off+len,&leng,
                                             (*result)->base_paragraph_layout)))
      goto ERROR3;
    len += leng;
  }

  psiconv_progress(config,lev+2,off+len,"Going to read base character layout");
  if (!((*result)->base_character_layout = psiconv_basic_character_layout()))
    goto ERROR3;
  if (has_content) {
    if ((res = psiconv_parse_character_layout_list(config,buf,lev+2,off+len,&leng,
                                             (*result)->base_character_layout)))
      goto ERROR4;
  }
  len += leng;

 
  psiconv_progress(config,lev+2,off+len,"Going to read the TextEd section");
  if (has_content) {
    if ((res = psiconv_parse_texted_section(config,buf,lev+2,off+len,&leng,
                                        &(*result)->text,
                                        (*result)->base_character_layout,
                                        (*result)->base_paragraph_layout)))
      goto ERROR4;
    len += leng;
  } else {
    (*result)->text = NULL;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of page header"
                   "(total length: %08x", len);

  return res;

ERROR4:
  psiconv_free_character_layout((*result)->base_character_layout);
ERROR3:
  psiconv_free_paragraph_layout((*result)->base_paragraph_layout);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Page Header failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}
   
int psiconv_parse_page_layout_section(const psiconv_config config,
                                      const psiconv_buffer buf,int lev,
                                      psiconv_u32 off, int *length, 
                                      psiconv_page_layout_section *result)
{
  int res = 0;
  int len = 0;
  int leng;
  psiconv_u32 temp;

  psiconv_progress(config,lev+1,off,"Going to read the page layout section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read first page number");
  (*result)->first_page_nr = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"First page: %d",(*result)->first_page_nr);
  len += 4;
  
  psiconv_progress(config,lev+2,off+len,"Going to read header distance");
  (*result)->header_dist = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Header distance: %6.3f",(*result)->header_dist);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read footer distance");
  (*result)->footer_dist = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Footer distance: %6.3f",(*result)->footer_dist);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the left margin");
  (*result)->left_margin = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Left margin: %6.3f",(*result)->left_margin);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going read the to right margin");
  (*result)->right_margin = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Right margin: %6.3f",(*result)->right_margin);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the top margin");
  (*result)->top_margin = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Top margin: %6.3f",(*result)->top_margin);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the bottom margin");
  (*result)->bottom_margin = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Bottom margin: %6.3f",(*result)->bottom_margin);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the header");
  if ((res = psiconv_parse_page_header(config,buf,lev+2,off+len,&leng,
                                       &(*result)->header)))
    goto ERROR2;
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the footer");
  if ((res = psiconv_parse_page_header(config,buf,lev+2,off+len,&leng,
                                       &(*result)->footer)))
    goto ERROR3;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read page dimensions id");
  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR4;
  if ((temp != PSICONV_ID_PAGE_DIMENSIONS1) && 
       (temp != PSICONV_ID_PAGE_DIMENSIONS2)) {
    psiconv_warn(config,lev+2,off+len,
                 "Page layout section page dimensions marker not found");
    psiconv_debug(config,lev+2,off+len,
                  "Page dimensions marker: read %08x, expected %08x or %08x",
                  temp, PSICONV_ID_PAGE_DIMENSIONS1,
                  PSICONV_ID_PAGE_DIMENSIONS2);
  }
  len += 4;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the page width");
  (*result)->page_width = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR4;
  psiconv_debug(config,lev+2,off+len,"Page width: %6.3f",(*result)->page_width);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the page height");
  (*result)->page_height = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR4;
  psiconv_debug(config,lev+2,off+len,"Page height: %6.3f",(*result)->page_height);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read page portrait/landscape");
  if ((res = psiconv_parse_bool(config,buf,lev+2,off+len,&leng,&(*result)->landscape)))
    goto ERROR4;
  psiconv_debug(config,lev+2,off+len,"Landscape: %d",(*result)->landscape);
  len += leng;


  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,"End of page section (total length: %08x)",
                   len);

  return res;

ERROR4:
  psiconv_free_page_header((*result)->footer);
ERROR3:
  psiconv_free_page_header((*result)->header);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Page Section failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}
