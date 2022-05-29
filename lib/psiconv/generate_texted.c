/*
    generate_texted.c - Part of psiconv, a PSION 5 file formats converter
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

#include "generate_routines.h"
#include "error.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif



int psiconv_write_texted_section(const psiconv_config config,
                                 psiconv_buffer buf, int lev,
                                 const psiconv_texted_section value,
                                 const psiconv_character_layout base_char,
                                 const psiconv_paragraph_layout base_para,
                                 psiconv_buffer *extra_buf)
{
  int res,with_layout_section=0;
  psiconv_u32 layout_id;

  psiconv_progress(config,lev,0,"Writing texted section");

  if (!value) {
    psiconv_error(config,lev,0,"Null TextEd section");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(*extra_buf = psiconv_buffer_new())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  layout_id = psiconv_buffer_unique_id();
  if ((res = psiconv_buffer_add_target(*extra_buf,layout_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }

  if (psiconv_list_length(value->paragraphs)) {
    with_layout_section = 1;
    if ((res = psiconv_write_styleless_layout_section(config,*extra_buf,lev+1,
                       value->paragraphs,
                       base_char,base_para)))
      goto ERROR2;
  } 

  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_TEXTED_BODY)))
    goto ERROR2;
  
  /* Partly dummy TextEd Jumptable */
  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_TEXTED_REPLACEMENT)))
    goto ERROR2;
  if ((res = psiconv_write_u32(config,buf,lev+1,0)))
    goto ERROR2;
  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_TEXTED_UNKNOWN)))
    goto ERROR2;
  if ((res = psiconv_write_u32(config,buf,lev+1,0)))
    goto ERROR2;
  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_TEXTED_LAYOUT)))
    goto ERROR2;
  if (with_layout_section) {
    if ((res = psiconv_write_offset(config,buf,lev+1,layout_id)))
      goto ERROR2;
  } else {
    if ((res = psiconv_write_u32(config,buf,lev+1,0)))
      goto ERROR2;
  }

  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_TEXTED_TEXT)))
    goto ERROR2;

  if ((res = psiconv_write_text_section(config,buf,lev+1,value->paragraphs)))
    goto ERROR2;
  psiconv_progress(config,lev,0,"End of texted section");
  return 0;

ERROR2:
  psiconv_buffer_free(*extra_buf);
ERROR1:
  psiconv_error(config,lev,0,"Writing of texted section failed");
  return res;
}
