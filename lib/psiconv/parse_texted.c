/*
    parse_texted.c - Part of psiconv, a PSION 5 file formats converter
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

#ifdef DMALLOC
#include <dmalloc.h>
#endif

int psiconv_parse_texted_section(const psiconv_config config,
                                 const psiconv_buffer buf,int lev,
                                 psiconv_u32 off, int *length,
                                 psiconv_texted_section *result,
                                 psiconv_character_layout base_char,
                                 psiconv_paragraph_layout base_para)
{
  int res = 0;
  int len = 0;
  psiconv_u32 layout_sec = 0;
  psiconv_u32 unknown_sec = 0;
  psiconv_u32 replacement_sec = 0;
  psiconv_u32 temp;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read a texted section");
  if (!((*result) = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read section id");
  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != PSICONV_ID_TEXTED_BODY) {
    psiconv_error(config,lev+2,off+len,
                 "Page header section body id not found");
    psiconv_debug(config,lev+2,off+len,
                  "Page body id: read %08x, expected %08x",temp,
                  PSICONV_ID_TEXTED);
    res = -PSICONV_E_PARSE;
    goto ERROR2;
  }
  len += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read the section jumptable");
  while (temp = psiconv_read_u32(config,buf,lev+3,off+len,&res), 
         !res && temp != PSICONV_ID_TEXTED_TEXT) {
    len += 4;
    if (temp == PSICONV_ID_TEXTED_LAYOUT) {
      layout_sec = psiconv_read_u32(config,buf,lev+3,off+len,&res);
      if (res)
        goto ERROR2;
      psiconv_debug(config,lev+3,off+len,"Found Layout section at %08x",layout_sec);
    } else if (temp == PSICONV_ID_TEXTED_REPLACEMENT) {
      replacement_sec = psiconv_read_u32(config,buf,lev+3,off+len,&res);
      if (res)
        goto ERROR2;
      psiconv_debug(config,lev+3,off+len,"Found Replacement section at %08x",
                    replacement_sec);
    } else if (temp == PSICONV_ID_TEXTED_UNKNOWN) {
      unknown_sec= psiconv_read_u32(config,buf,lev+3,off+len,&res);
      if (res)
        goto ERROR2;
      if (unknown_sec) {
        psiconv_warn(config,lev+3,off+len,
             "Unknown section in TextEd jumptable has real offset (ignoring)");
      }
      psiconv_debug(config,lev+3,off+len,"Found Unknown section at %08x",
                    unknown_sec);
    } else {
      psiconv_warn(config,lev+3,off+len,
                   "Unknown section in TextEd jumptable (ignoring)");
      psiconv_debug(config,lev+3,off+len,"Section ID %08x at offset %08x",temp,
                    psiconv_read_u32(config,buf,lev+3,off+len,NULL));
    }
    len += 4;
  }
  if (res)
    goto ERROR2;
  
  len += 4;
  psiconv_progress(config,lev+2,off+len,"Going to read the text");
  if ((res = psiconv_parse_text_section(config,buf,lev+2,off+len,&leng,
                                        &(*result)->paragraphs)))
    goto ERROR2;
  len += leng;
  
  if (layout_sec) {
    psiconv_progress(config,lev+2,off+len,"Going to read the layout");
    if ((res = psiconv_parse_styleless_layout_section(config,buf,lev+2,layout_sec,NULL,
                                           (*result)->paragraphs,
                                           base_char,base_para)))
      goto ERROR3;
  }

#if 0
  if (replacement_sec) {
    psiconv_progress(config,lev+2,off+len,"Going to read the replacements");
    /* WHATEVER */
   }
#endif
    
  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of TextEd section "
                   "(total length: %08x", len);

  return 0;

ERROR3:
  psiconv_free_text_and_layout((*result)->paragraphs);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of TextEd Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}


