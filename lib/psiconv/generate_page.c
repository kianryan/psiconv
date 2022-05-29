/*
    generate_page.c - Part of psiconv, a PSION 5 file formats converter
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

#include <stdlib.h>
#include <string.h>

#include "generate_routines.h"
#include "error.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif



int psiconv_write_page_header(const psiconv_config config,
                              psiconv_buffer buf, int lev,
                              const psiconv_page_header value,
                              psiconv_buffer *extra_buf)
{
  int res;
  psiconv_paragraph_layout basepara;
  psiconv_character_layout basechar;

  psiconv_progress(config,lev,0,"Writing page header");

  if (!value) {
    psiconv_error(config,lev,0,"Null page header");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(basepara=psiconv_basic_paragraph_layout())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }
  if (!(basechar=psiconv_basic_character_layout())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR2;
  }

  /* Unknown byte */
  if ((res = psiconv_write_u8(config,buf,lev+1,0x01)))
    goto ERROR3;
  if ((res = psiconv_write_bool(config,buf,lev+1,value->on_first_page)))
    goto ERROR3;
  /* Three unknown bytes */
  if ((res = psiconv_write_u8(config,buf,lev+1,0x00)))
    goto ERROR3;
  if ((res = psiconv_write_u8(config,buf,lev+1,0x00)))
    goto ERROR3;
  if ((res = psiconv_write_u8(config,buf,lev+1,0x00)))
    goto ERROR3;
  if ((res = psiconv_write_paragraph_layout_list(config,buf,lev+1,
                                    value->base_paragraph_layout,basepara)))
    goto ERROR3;
  if ((res = psiconv_write_character_layout_list(config,buf,lev+1,
                                    value->base_character_layout,basechar)))
    goto ERROR3;
  res =  psiconv_write_texted_section(config,buf,lev+1,value->text,
                                      value->base_character_layout,
                                      value->base_paragraph_layout,extra_buf);
ERROR3:
  psiconv_free_character_layout(basechar);
ERROR2:
  psiconv_free_paragraph_layout(basepara);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of page header failed");
  else
    psiconv_progress(config,lev,0,"End of page header");
  return res;
}

int psiconv_write_page_layout_section(const psiconv_config config,
                                      psiconv_buffer buf, int lev,
                                      const psiconv_page_layout_section value)
{
  int res;
  psiconv_buffer header_buf,footer_buf;

  psiconv_progress(config,lev,0,"Writing page layout section");

  if (!value) {
    psiconv_error(config,lev,0,"Null page section");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if ((res = psiconv_write_u32(config,buf,lev+1,value->first_page_nr)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->header_dist)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->footer_dist)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->left_margin)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->right_margin)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->top_margin)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->bottom_margin)))
    goto ERROR1;
  if ((res = psiconv_write_page_header(config,buf,lev+1,value->header,&header_buf)))
    goto ERROR1;
  if ((res = psiconv_write_page_header(config,buf,lev+1,value->footer,&footer_buf)))
    goto ERROR2;
  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_PAGE_DIMENSIONS2)))
    goto ERROR3;
  if ((res = psiconv_write_length(config,buf,lev+1,value->page_width)))
    goto ERROR3;
  if ((res =  psiconv_write_length(config,buf,lev+1,value->page_height)))
    goto ERROR3;
  if ((res = psiconv_write_bool(config,buf,lev+1,value->landscape)))
    goto ERROR3;
  if ((res = psiconv_buffer_concat(buf,header_buf))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_buffer_concat(buf,footer_buf))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }


ERROR3:
  psiconv_buffer_free(footer_buf);
ERROR2:
  psiconv_buffer_free(header_buf);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of page layout section failed");
  else
    psiconv_progress(config,lev,0,"End of page layout section");
  return res;
}
