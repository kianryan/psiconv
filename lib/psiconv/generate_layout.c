/*
    generate_layout.c - Part of psiconv, a PSION 5 file formats converter
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

#include "generate_routines.h"
#include "error.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif


int psiconv_write_color(const psiconv_config config, psiconv_buffer buf, 
                        int lev, const psiconv_color value)
{
  int res;

  psiconv_progress(config,lev,0,"Writing color");

  if (!value) {
    psiconv_error(config,lev,0,"Null color");
    res = -PSICONV_E_GENERATE;
    goto ERROR;
  }
  if ((res = psiconv_write_u8(config,buf,lev+1,value->red))) 
    goto ERROR;
  if ((res = psiconv_write_u8(config,buf,lev+1,value->green))) 
    goto ERROR;
  if ((res = psiconv_write_u8(config,buf,lev+1,value->blue)))
    goto ERROR;
  psiconv_progress(config,lev,0,"End of color");
  return 0;

ERROR:
  psiconv_error(config,lev,0,"Writing of color failed");
  return res;
}

int psiconv_write_font(const psiconv_config config, psiconv_buffer buf, 
                       int lev, const psiconv_font value)
{
  int res,len;

  psiconv_progress(config,lev,0,"Writing font");
  if (!value) {
    psiconv_error(config,0,psiconv_buffer_length(buf),"Null font");
    res = -PSICONV_E_GENERATE;
    goto ERROR;
  }
  len = psiconv_unicode_strlen(value->name);
  if ((res = psiconv_write_u8(config,buf,lev+1,len+1)))
    goto ERROR;
  if ((res = psiconv_write_charlist(config,buf,lev+1,value->name)))
    goto ERROR;
  if ((res = psiconv_write_u8(config,buf,lev+1,value->screenfont)))
    goto ERROR;
  psiconv_progress(config,lev,0,"End of font");
  return 0;

ERROR:
  psiconv_error(config,lev,0,"Writing of font failed");
  return res;
}

int psiconv_write_border(const psiconv_config config, psiconv_buffer buf,int lev,  const psiconv_border value)
{
  int res;

  psiconv_progress(config,lev,0,"Writing border");

  if (!value) {
    psiconv_error(config,lev,0,"Null border");
    res = -PSICONV_E_GENERATE;
    goto ERROR;
  }
  if (value->kind > psiconv_border_dotdotdashed) 
    psiconv_warn(config,lev,0,
                 "Unknown border kind (%d); assuming none",value->kind);
  if ((res =psiconv_write_u8(config,buf,lev+1,
	                         value->kind == psiconv_border_none?0:
                                 value->kind == psiconv_border_solid?1:
                                 value->kind == psiconv_border_double?2:
                                 value->kind == psiconv_border_dotted?3:
                                 value->kind == psiconv_border_dashed?4:
                                 value->kind == psiconv_border_dotdashed?5:
                                 value->kind == psiconv_border_dotdotdashed?6:
                                                0)))
    goto ERROR;
  if ((res = psiconv_write_size(config,buf,lev+1,
	                        (value->kind == psiconv_border_solid) ||
                                (value->kind == psiconv_border_double) ?
                                 value->thickness:1.0/20.0)))
    goto ERROR;
  if ((res = psiconv_write_color(config,buf,lev+1,value->color)))
    goto ERROR;
  /* Unknown byte */
  if ((res = psiconv_write_u8(config,buf,lev+1,1)))
    goto ERROR;
  psiconv_progress(config,lev,0,"End of border");
  return 0;

ERROR:
  psiconv_error(config,lev,0,"Writing of border failed");
  return res;
}

int psiconv_write_bullet(const psiconv_config config, psiconv_buffer buf,int lev,  const psiconv_bullet value)
{
  int res;
  psiconv_buffer extra_buf;

  psiconv_progress(config,lev,0,"Writing bullet");

  if (!value) {
    psiconv_error(config,0,psiconv_buffer_length(buf),"Null bullet");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(extra_buf = psiconv_buffer_new())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }
  if ((res = psiconv_write_size(config,extra_buf,lev+1,value->font_size)))
    goto ERROR2;
  if ((res = psiconv_unicode_write_char(config,extra_buf,lev+1,
	                                value->character)))
    goto ERROR2;
  if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->indent)))
    goto ERROR2;
  if ((res = psiconv_write_color(config,extra_buf,lev+1,value->color)))
    goto ERROR2;
  if ((res = psiconv_write_font(config,extra_buf,lev+1,value->font)))
    goto ERROR2;

  if ((res = psiconv_write_u8(config,buf,lev+1,psiconv_buffer_length(extra_buf))))
    goto ERROR2;
  if ((res = psiconv_buffer_concat(buf,extra_buf))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  
ERROR2:
  psiconv_buffer_free(extra_buf);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of bullet failed");
  else
    psiconv_progress(config,lev,0,"End of bullet");
  return res;
}

int psiconv_write_tab(const psiconv_config config, psiconv_buffer buf,int lev, psiconv_tab value)
{
  int res;

  psiconv_progress(config,lev,0,"Writing tab");

  if (!value) {
    psiconv_error(config,lev,0,"Null tab");
    res = -PSICONV_E_GENERATE;
    goto ERROR;
  }
  if ((res = psiconv_write_length(config,buf,lev+1,value->location)))
    goto ERROR;
  if ((value->kind != psiconv_tab_left) && 
      (value->kind != psiconv_tab_right) &&
      (value->kind != psiconv_tab_centre)) 
    psiconv_warn(config,lev,0,
                 "Unknown tab kind (%d); assuming left",value->kind);
  if ((res = psiconv_write_u8(config,buf,lev+1, 
	                       value->kind == psiconv_tab_right?2:
                               value->kind == psiconv_tab_centre?3:1)))
    goto ERROR;
  psiconv_progress(config,lev,0,"End of tab");
  return 0;
ERROR:
  psiconv_error(config,lev,0,"Writing of tab failed");
  return res;
}

int psiconv_write_paragraph_layout_list(const psiconv_config config,
                                        psiconv_buffer buf,int lev,  
                                        psiconv_paragraph_layout value,
                                        psiconv_paragraph_layout base)
{
  int res,i;
  psiconv_buffer extra_buf;
  psiconv_tab tab;
  
  psiconv_progress(config,lev,0,"Writing paragraph layout list");

  if (!value) {
    psiconv_error(config,lev,0,"Null paragraph layout list");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }
  if (!(extra_buf = psiconv_buffer_new())) {
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }
  
  if (!base || psiconv_compare_color(base->back_color,value->back_color)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x01)))
      goto ERROR2;
    if ((res = psiconv_write_color(config,extra_buf,lev+1,value->back_color)))
      goto ERROR2;
  }

  if (!base || (value->indent_left != base->indent_left)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x02)))
      goto ERROR2;
    if ((res = psiconv_write_length(config,extra_buf,lev+1,value->indent_left)))
      goto ERROR2;
  }

  if (!base || (value->indent_right != base->indent_right)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x03)))
      goto ERROR2;
    if ((res = psiconv_write_length(config,extra_buf,lev+1,value->indent_right)))
      goto ERROR2;
  }

  if (!base || (value->indent_first != base->indent_first)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x04)))
      goto ERROR2;
    if ((res = psiconv_write_length(config,extra_buf,lev+1,value->indent_first)))
      goto ERROR2;
  }

  if (!base || (value->justify_hor != base->justify_hor)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x05)))
      goto ERROR2;
    if ((value->justify_hor < psiconv_justify_left) ||
        (value->justify_hor > psiconv_justify_full))
      psiconv_warn(config,lev,0,
                   "Unknown horizontal justify (%d); assuming left",
                   value->justify_hor);
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,
               value->justify_hor == psiconv_justify_centre?1:
               value->justify_hor == psiconv_justify_right?2:
               value->justify_hor == psiconv_justify_full?3:0)))
      goto ERROR2;
  }

  if (!base || (value->justify_ver != base->justify_ver)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x06)))
      goto ERROR2;
    if ((value->justify_ver < psiconv_justify_top) ||
        (value->justify_ver > psiconv_justify_bottom))
      psiconv_warn(config,0,psiconv_buffer_length(buf),
                   "Unknown vertical justify (%d); assuming top",
                    value->justify_ver);
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,
               value->justify_ver == psiconv_justify_middle?1:
               value->justify_ver == psiconv_justify_bottom?2:0)))
      goto ERROR2;
  }

  if (!base || (value->linespacing != base->linespacing)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x07)))
      goto ERROR2;
    if ((res = psiconv_write_size(config,extra_buf,lev+1,value->linespacing)))
      goto ERROR2;
  }

  if (!base || (value->linespacing_exact != base->linespacing_exact)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x08)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->linespacing_exact)))
      goto ERROR2;
  }

  if (!base || (value->space_above != base->space_above)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x09)))
      goto ERROR2;
    if ((res = psiconv_write_size(config,extra_buf,lev+1,value->space_above)))
      goto ERROR2;
  }

  if (!base || (value->space_below != base->space_below)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x0a)))
      goto ERROR2;
    if ((res = psiconv_write_size(config,extra_buf,lev+1,value->space_below)))
      goto ERROR2;
  }

  if (!base || (value->keep_together != base->keep_together)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x0b)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->keep_together)))
      goto ERROR2;
  }

  if (!base || (value->keep_with_next != base->keep_with_next)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x0c)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->keep_with_next)))
      goto ERROR2;
  }

  if (!base || (value->on_next_page != base->on_next_page)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x0d)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->on_next_page)))
      goto ERROR2;
  }

  if (!base || (value->no_widow_protection != base->no_widow_protection)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x0e)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->no_widow_protection)))
      goto ERROR2;
  }

  if (!base || (value->wrap_to_fit_cell != base->wrap_to_fit_cell)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x0f)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->wrap_to_fit_cell)))
      goto ERROR2;
  }

  if (!base || (value->border_distance != base->border_distance)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x10)))
      goto ERROR2;
    if ((res = psiconv_write_length(config,extra_buf,lev+1,value->border_distance)))
      goto ERROR2;
  }

  if (!base || psiconv_compare_border(value->top_border,base->top_border)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x11)))
      goto ERROR2;
    if ((res = psiconv_write_border(config,extra_buf,lev+1,value->top_border))) 
      goto ERROR2;
  }

  if (!base || psiconv_compare_border(value->bottom_border,
                                       base->bottom_border)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x12)))
      goto ERROR2;
    if ((res = psiconv_write_border(config,extra_buf,lev+1,value->bottom_border)))
      goto ERROR2;
  }

  if (!base || psiconv_compare_border(value->left_border,
                                       base->left_border)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x13)))
      goto ERROR2;
    if ((res = psiconv_write_border(config,extra_buf,lev+1,value->left_border)))
      goto ERROR2;
  }

  if (!base || psiconv_compare_border(value->right_border,
                                       base->right_border)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x14))) 
      goto ERROR2;
    if ((res = psiconv_write_border(config,extra_buf,lev+1,value->right_border)))
      goto ERROR2;
  }

  if (!base || psiconv_compare_bullet(value->bullet,
                                       base->bullet)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x15))) 
      goto ERROR2;
    if ((res = psiconv_write_bullet(config,extra_buf,lev+1,value->bullet)))
      goto ERROR2;
  }

  if (!value->tabs || !value->tabs->extras) {
    psiconv_error(config,0,psiconv_buffer_length(buf),"Null tabs");
    res = -PSICONV_E_GENERATE;
    goto ERROR2;
  } 

  /* It is not entirely clear how tabs are inherited. For now, I assume
     if there is any difference at all, we will have to generate both
     the normal tab-interval, and all specific tabs */
  if (!base || psiconv_compare_all_tabs(value->tabs,base->tabs)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x16))) 
      goto ERROR2;
    if ((res = psiconv_write_length(config,extra_buf,lev+1,value->tabs->normal)))
      goto ERROR2;
    for (i = 0; i < psiconv_list_length(value->tabs->extras); i++) {
      if (!(tab = psiconv_list_get(value->tabs->extras,i))) {
        psiconv_error(config,lev+1,0,"Data structure corruption");
        res = -PSICONV_E_NOMEM;
        goto ERROR2;
      }
      if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x17)))
        goto ERROR2;
      if ((res = psiconv_write_tab(config,extra_buf,lev+1,tab)))
        goto ERROR2;
    }
  }

  if ((res = psiconv_write_u32(config,buf,lev+1,psiconv_buffer_length(extra_buf))))
    goto ERROR2;

  if ((res = psiconv_buffer_concat(buf,extra_buf))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }

ERROR2:
  psiconv_buffer_free(extra_buf);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of paragraph layout list failed");
  else
    psiconv_progress(config,lev,0,"End of paragraph layout list");
  return res;
}

int psiconv_write_character_layout_list(const psiconv_config config,
                                        psiconv_buffer buf,int lev,  
                                        psiconv_character_layout value,
                                        psiconv_character_layout base)
{
  int res;
  psiconv_buffer extra_buf;

  psiconv_progress(config,lev,0,"Writing character layout list");

  if (!value) {
    psiconv_error(config,lev,0,"Null character layout list");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }
  if (!(extra_buf = psiconv_buffer_new())) {
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  if (!base || psiconv_compare_color(base->color,value->color)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x19)))
      goto ERROR2;
    if ((res = psiconv_write_color(config,extra_buf,lev+1,value->color)))
      goto ERROR2;
  }

  if (!base || psiconv_compare_color(base->back_color,value->back_color)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x1a)))
      goto ERROR2;
    if ((res = psiconv_write_color(config,extra_buf,lev+1,value->back_color)))
      goto ERROR2;
  }

  if (!base || (value->font_size != base->font_size)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x1c)))
      goto ERROR2;
    if ((res = psiconv_write_size(config,extra_buf,lev+1,value->font_size)))
      goto ERROR2;
  }

  if (!base || (value->italic != base->italic)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x1d)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->italic)))
      goto ERROR2;
  }

  if (!base || (value->bold != base->bold)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x1e)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->bold)))
      goto ERROR2;
  }

  if (!base || (value->super_sub != base->super_sub)) {
    if ((value->super_sub != psiconv_superscript) &&
        (value->super_sub != psiconv_subscript) &&
        (value->super_sub != psiconv_normalscript))
      psiconv_warn(config,lev,0,"Unknown supersubscript (%d); assuming normal",
                   value->super_sub);
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x1f)))
      goto ERROR2;
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,
                                value->super_sub == psiconv_superscript?1:
                                value->super_sub == psiconv_subscript?2:0)))
      goto ERROR2;
  }

  if (!base || (value->underline != base->underline)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x20)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->underline)))
      goto ERROR2;
  }

  if (!base || (value->strikethrough != base->strikethrough)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x21)))
      goto ERROR2;
    if ((res = psiconv_write_bool(config,extra_buf,lev+1,value->strikethrough)))
      goto ERROR2;
  }

  if (!base || psiconv_compare_font(base->font,value->font)) {
    if ((res = psiconv_write_u8(config,extra_buf,lev+1,0x22)))
      goto ERROR2;
    if ((res = psiconv_write_font(config,extra_buf,lev+1,value->font)))
      goto ERROR2;
  }

  if ((res = psiconv_write_u32(config,buf,lev+1,psiconv_buffer_length(extra_buf))))
    goto ERROR2;

  res = psiconv_buffer_concat(buf,extra_buf);

ERROR2:
  psiconv_buffer_free(extra_buf);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of character layout list failed");
  else
    psiconv_progress(config,lev,0,"End of character layout list");
  return res;
}
