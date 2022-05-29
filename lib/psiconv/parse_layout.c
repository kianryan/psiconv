/*
    parse_layout.c - Part of psiconv, a PSION 5 file formats converter
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

#ifdef DMALLOC
#include <dmalloc.h>
#endif


int psiconv_parse_color(const psiconv_config config,
                        const psiconv_buffer buf, int lev, psiconv_u32 off, 
                        int *length, psiconv_color *result)
{
  int res = 0;
  int len = 0;

  psiconv_progress(config,lev+1,off,"Going to parse color");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  (*result)->red = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  (*result)->green = psiconv_read_u8(config,buf,lev+2,off+len+1,&res);
  if (res)
    goto ERROR2;
  (*result)->blue = psiconv_read_u8(config,buf,lev+2,off+len+2,&res);
  if (res)
    goto ERROR2;
  len += 3;

  psiconv_debug(config,lev+2,off,"Color: red %02x, green %02x, blue %02x",
                (*result)->red, (*result)->green, (*result)->blue);
  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of color (total length: %08x)",len);
  return 0;

ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Color failed");
  if (length)
    *length = 0;
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}



int psiconv_parse_font(const psiconv_config config,
                       const psiconv_buffer buf, int lev, psiconv_u32 off, 
                       int *length, psiconv_font *result)
{
  int res = 0;
  char *str_copy;
  int len=0;
  int fontlen;

  psiconv_progress(config,lev+1,off,"Going to parse font");
  if (!(*result = malloc(sizeof(**result)))) 
    goto ERROR1;

  fontlen = psiconv_read_u8(config,buf,lev+2,off,&res);
  if (res)
    goto ERROR2;
  len = 1;

  (*result)->name = psiconv_read_charlist(config,buf,lev+2,off+len, fontlen-1,&res);
  if (res)
    goto ERROR2;
  len += fontlen - 1;

  (*result)->screenfont = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;

  if (!(str_copy = psiconv_make_printable(config,(*result)->name)))
    goto ERROR3;

  psiconv_debug(config,lev+2,off+len,
                "Found font `%s', displayed with screen font %02x",
                    str_copy,(*result)->screenfont);
  free(str_copy);
  len ++;

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off + len - 1,
                   "End of font (total length: %08x)",len);
  return 0;

ERROR3:
  free ((*result)->name);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Font failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_border(const psiconv_config config,
                         const psiconv_buffer buf,int lev,psiconv_u32 off,
                         int *length, psiconv_border *result)
{
  int res = 0;
  int len = 0;
  psiconv_u32 temp;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to parse border data");
  if (!(*result = malloc(sizeof(**result)))) {
    goto ERROR1;
  }

  psiconv_progress(config,lev+2,off+len,"Going to read border kind");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp == 0x00)
    (*result)->kind = psiconv_border_none;
  else if (temp == 0x01)
    (*result)->kind = psiconv_border_solid;
  else if (temp == 0x02)
    (*result)->kind = psiconv_border_double;
  else if (temp == 0x03)
    (*result)->kind = psiconv_border_dotted;
  else if (temp == 0x04)
    (*result)->kind = psiconv_border_dashed;
  else if (temp == 0x05)
    (*result)->kind = psiconv_border_dotdashed;
  else if (temp == 0x06)
    (*result)->kind = psiconv_border_dotdotdashed;
  else {
    psiconv_warn(config,lev+2,off,"Unknown border kind (defaults to `none')");
    (*result)->kind = psiconv_border_none;
  }
  psiconv_debug(config,lev+2,off+len,"Kind: %02x",temp);
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read border thickness");
  (*result)->thickness = psiconv_read_size(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
#if 0
  /* This seems no longer necessary to test? */
  if (((*result)->kind != psiconv_border_solid) &&
      ((*result)->kind != psiconv_border_double) &&
      ((*result)->thickness != 0.0) &&
      (fabs((*result)->thickness - 1/20) >= 1/1000)) {
    psiconv_warn(config,lev+2,off,
                 "Border thickness specified for unlikely border type");
  }
#endif
  psiconv_debug(config,lev+2,off+len,"Thickness: %f",(*result)->thickness);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the border color");
  if ((psiconv_parse_color(config,buf,lev+2,off+len,&leng,&(*result)->color)))
    goto ERROR2;
  len += leng;
 
  psiconv_progress(config,lev+2,off+len,"Going to read the final unknown byte "
                                 "(0x00 or 0x01 expected)");
  temp = psiconv_read_u8(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR3;
  if ((temp != 0x01) && (temp != 0x00)) {
    psiconv_warn(config,lev+2,off,"Unknown last byte in border specification");
    psiconv_debug(config,lev+2,off+len, "Last byte: read %02x, expected %02x or %02x",
                  temp,0x00,0x01);
  }
  len ++;

  if (length)
    *length = len;
 
  psiconv_progress(config,lev+1,off + len - 1,
           "End of border (total length: %08x)",len);

  return 0;

ERROR3:
  psiconv_free_color((*result)->color);
ERROR2:
  free (result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Border failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_bullet(const psiconv_config config,
                         const psiconv_buffer buf,int lev,psiconv_u32 off,
                         int *length, psiconv_bullet *result)
{
  int res = 0;
  int len = 0;
  int leng;
  int bullet_length;

  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;
  (*result)->on = psiconv_bool_true;

  psiconv_progress(config,lev+1,off,"Going to parse bullet data");
  psiconv_progress(config,lev+2,off+len,"Going to read bullet length");
  bullet_length = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Length: %02x",bullet_length);
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read bullet font size");
  (*result)->font_size = psiconv_read_size(config,buf,lev+2,off+len, &leng,&res);
  if (res)
    goto ERROR2;
  len +=leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read bullet character");
  (*result)->character = psiconv_unicode_read_char(config,buf,lev+2,
						   off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Character: %02x",(*result)->character);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read indent on/off");
  if ((res = psiconv_parse_bool(config,buf,lev+2,off+len,&leng,&(*result)->indent)))
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Indent on: %02x",(*result)->indent);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read bullet color");
  if ((res = psiconv_parse_color(config,buf,lev+2,off+len,&leng,&(*result)->color)))
    goto ERROR2;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read bullet font");
  if ((res = psiconv_parse_font(config,buf,lev+2,off+len,&leng,&(*result)->font)))
    goto ERROR3;
  len += leng;

  if (len != bullet_length + 1) {
    psiconv_warn(config,lev+2,off,"Bullet data structure length mismatch");
    psiconv_debug(config,lev+2,off,"Length: specified %02x, found %02x",
                  bullet_length,len-1);
  }

  psiconv_progress(config,lev+1,off + len - 1,
                   "End of bullet data (total length: %08x)",len);

  if (length)
    *length = len;
  return 0;

ERROR3:
  psiconv_free_color((*result)->color);
ERROR2:
  free (result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Bullet failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_tab(const psiconv_config config,
                      const psiconv_buffer buf, int lev, psiconv_u32 off,
                      int *length, psiconv_tab *result)
{
  int res = 0;
  int len = 0;
  int leng;
  psiconv_u8 temp;

  psiconv_progress(config,lev+1,off,"Going to parse tab");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off,"Going to read tab location");
  (*result)->location = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the tab kind");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp == 1)
    (*result)->kind = psiconv_tab_left;
  else if (temp == 2)
    (*result)->kind = psiconv_tab_centre;
  else if (temp == 3)
    (*result)->kind = psiconv_tab_right;
  else {
    psiconv_warn(config,lev+2,off+len,"Unknown tab kind argument");
    psiconv_debug(config,lev+2,off+len,"Kind found: %02x (defaulted to left tab)",
                  temp);
    (*result)->kind = psiconv_tab_left;
  }
  psiconv_debug(config,lev+2,off+len,"Kind: %02x",temp);
  len ++;
  
  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of tab (total length: %08x)",len);
  return 0;
  
ERROR2:
  free (result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Tab failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_paragraph_layout_list(const psiconv_config config,
                                        const psiconv_buffer buf, int lev,
                                        psiconv_u32 off, int *length,
                                        psiconv_paragraph_layout result)
{
  int res=0;
  int len=0;
  int list_length,leng,nr;
  psiconv_u8 id;
  psiconv_u32 temp;
  psiconv_tab temp_tab;
  psiconv_color temp_color;
  psiconv_border temp_border;
  psiconv_bullet temp_bullet;

  psiconv_progress(config,lev+1,off,"Going to read paragraph layout list");

  psiconv_progress(config,lev+2,off,"Going to read the list length");
  list_length = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR1;
  psiconv_debug(config,lev+2,off,"Length in bytes: %08x",list_length);
  len += 4;

  nr = 0;
  while(len - 4 < list_length) {
    psiconv_progress(config,lev+2,off+len,"Going to read element %d",nr);
    psiconv_progress(config,lev+3,off+len,"Going to read the element id");
    id = psiconv_read_u8(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR1;
    psiconv_debug(config,lev+3,off+len,"Id: %02x",id);
    len ++;
    switch(id) {
      case 0x01:
        psiconv_progress(config,lev+3,off+len,"Going to read background color");
        if ((res = psiconv_parse_color(config,buf,lev+3,off+len,&leng,&temp_color)))
          goto ERROR1;
        psiconv_free_color(result->back_color);
        result->back_color = temp_color;
        len += leng;
        break;
      case 0x02:
        psiconv_progress(config,lev+3,off+len ,"Going to read indent left");
        result->indent_left = psiconv_read_length(config,buf,lev+3,off+len,&leng,&res);
        if (res)
          goto ERROR1;
        len += leng;
        break;
      case 0x03:
        psiconv_progress(config,lev+3,off+len,"Going to read indent right");
        result->indent_right = psiconv_read_length(config,buf,lev+2,off+len,&leng,
                                                   &res);
        if (res)
          goto ERROR1;
        len += leng;
        break;
      case 0x04:
        psiconv_progress(config,lev+3,off+len,"Going to read indent left first line");
        result->indent_first = psiconv_read_length(config,buf,lev+2,off+len, &leng,
                                                   &res);
        if (res)
          goto ERROR1;
        len += leng;
        break;
      case 0x05:
        psiconv_progress(config,lev+3,off+len,"Going to read horizontal justify");
        temp = psiconv_read_u8(config,buf,lev+3,off+len,&res);
        if (res)
          goto ERROR1;
        if (temp == 0x00)
          result->justify_hor = psiconv_justify_left;
        else if (temp == 0x01)
          result->justify_hor = psiconv_justify_centre;
        else if (temp == 0x02)
          result->justify_hor = psiconv_justify_right;
        else if (temp == 0x03)
          result->justify_hor = psiconv_justify_full;
        else {
          psiconv_warn(config,lev+3,off+len, "Unknown horizontal justify argument "
                       "in paragraph layout codes list");
          result->justify_hor = psiconv_justify_left;
        }
        psiconv_debug(config,lev+3,off+len,"Justify: %02x",temp);
        len ++;
        break;
      case 0x06:
        psiconv_progress(config,lev+3,off+len,"Going to read vertical justify");
        temp = psiconv_read_u8(config,buf,lev+3,off+len,&res);
        if (res)
          goto ERROR1;
        if (temp == 0x00)
          result->justify_ver = psiconv_justify_top;
        else if (temp == 0x01)
          result->justify_ver = psiconv_justify_middle;
        else if (temp == 0x02)
          result->justify_ver = psiconv_justify_bottom;
        else {
          psiconv_warn(config,lev+3,off+len, "Unknown vertical justify argument "
                                      "in paragraph layout codes list");
          result->justify_ver = psiconv_justify_bottom;
        }
        psiconv_debug(config,lev+3,off+len,"Justify: %02x",temp);
        len ++;
		break;
      case 0x07:
        psiconv_progress(config,lev+3,off+len,"Going to read linespacing distance");
        result->linespacing = psiconv_read_size(config,buf,lev+3,off+len,&leng,&res);
        if (res)
          goto ERROR1;
        len += leng;
        break;
      case 0x08:
        psiconv_progress(config,lev+3,off+len,"Going to read linespacing exact");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,
                                  &result->linespacing_exact)))
          goto ERROR1;
        len += leng;
        break;
      case 0x09:
        psiconv_progress(config,lev+3,off+len,"Going to read top space");
        result->space_above = psiconv_read_size(config,buf,lev+3,off+len,&leng,&res);
        if (res)
          goto ERROR1;
        len += leng;
        break;
      case 0x0a:
        psiconv_progress(config,lev+3,off+len,"Going to read bottom space");
        result->space_below = psiconv_read_size(config,buf,lev+3,off+len,&leng,&res);
        if (res)
          goto ERROR1;
        len += leng;
        break;
      case 0x0b:
        psiconv_progress(config,lev+3,off+len,"Going to read on one page");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,
                                      &result->keep_together)))
          goto ERROR1;
        len += leng;
        break;
      case 0x0c:
        psiconv_progress(config,lev+3,off+len,"Going to read together with");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,
                                      &result->keep_with_next)))
          goto ERROR1;
        len += leng;
        break;
      case 0x0d:
        psiconv_progress(config,lev+3,off+len,"Going to read on next page");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,
                                      &result->on_next_page)))
          goto ERROR1;
        len += leng;
        break;
      case 0x0e:
        psiconv_progress(config,lev+3,off+len,"Going to read no widow protection");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,
                                      &result->no_widow_protection)))
          goto ERROR1;
        len += leng;
        break;
      case 0x0f:
        psiconv_progress(config,lev+3,off+len,"Going to read wrap to fit cell limits");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,
                                      &result->wrap_to_fit_cell)))
          goto ERROR1;
        len += leng;
        break;
      case 0x10:
        psiconv_progress(config,lev+3,off+len,"Going to read border distance to text");
        result->border_distance = psiconv_read_length(config,buf,lev+3,
                                                         off+len,&leng,&res);
        if (res)
          goto ERROR1;
        len += leng;
        break;
      case 0x11:
        psiconv_progress(config,lev+3,off+len,"Going to read top border");
        if ((res = psiconv_parse_border(config,buf,lev+3,off+len,&leng,&temp_border)))
          goto ERROR1;
        psiconv_free_border(result->top_border);
        result->top_border = temp_border;
        len += leng;
        break;
      case 0x12:
        psiconv_progress(config,lev+3,off+len,"Going to read bottom border");
        if ((res = psiconv_parse_border(config,buf,lev+3,off+len,&leng,&temp_border)))
          goto ERROR1;
        psiconv_free_border(result->bottom_border);
        result->bottom_border = temp_border;
        len += leng;
        break;
      case 0x13:
        psiconv_progress(config,lev+3,off+len,"Going to read left border");
        if ((res = psiconv_parse_border(config,buf,lev+3,off+len,&leng,&temp_border)))
          goto ERROR1;
        psiconv_free_border(result->left_border);
        result->left_border = temp_border;
        len += leng;
        break;
      case 0x14:
        psiconv_progress(config,lev+3,off+len,"Going to read right border");
        if ((res = psiconv_parse_border(config,buf,lev+3,off+len,&leng,&temp_border)))
          goto ERROR1;
        psiconv_free_border(result->right_border);
        result->right_border = temp_border;
        len += leng;
        break;
      case 0x15:
        psiconv_progress(config,lev+3,off+len,"Going to read bullet");
        if ((res = psiconv_parse_bullet(config,buf,lev+3,off+len,&leng,&temp_bullet)))
          goto ERROR1;
        psiconv_free_bullet(result->bullet);
        result->bullet = temp_bullet;
        len += leng;
        break;
      case 0x16:
        psiconv_progress(config,lev+3,off+len,"Going to read standard tabs");
        result->tabs->normal = psiconv_read_length(config,buf,lev+3,off+len,&leng,
                               &res);
        if (res)
          goto ERROR1;
        len += leng;
        break;
      case 0x17:
        psiconv_progress(config,lev+3,off+len,"Going to read extra tab");
        if ((res = psiconv_parse_tab(config,buf,lev+3,off+len,&leng,&temp_tab)))
          goto ERROR1;
        if ((res = psiconv_list_add(result->tabs->extras,temp_tab))) {
          psiconv_free_tab(temp_tab);
          goto ERROR1;
        }
        psiconv_free_tab(temp_tab);
        len += leng;
        break;
      default:
        psiconv_warn(config,lev+3,off+len,
                     "Unknown code in paragraph layout codes list");
        psiconv_debug(config,lev+3,off+len,"Code: %02x",id);
        len ++;
        break;
    }
    nr ++;
  }

  if (len - 4 != list_length) {
    psiconv_error(config,lev+2,off+len,
         "Read past end of paragraph layout codes list. I probably lost track "
         "somewhere!");
    psiconv_debug(config,lev+2,off+len,"Read %d characters instead of %d",
                  len-4,list_length);
    res = PSICONV_E_PARSE;
    goto ERROR1;
  }

  len = list_length + 4;

  psiconv_progress(config,lev+1,off+len,
                   "End of paragraph layout list (total length: %08x)",len);

  if (length)
    *length = len;
  return 0;

ERROR1:
  psiconv_error(config,lev+1,off,"Reading of paragraph_layout_list failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_character_layout_list(const psiconv_config config,
                                        const psiconv_buffer buf, int lev,
                                        psiconv_u32 off, int *length, 
                                        psiconv_character_layout result)
{
  int res=0;
  int len=0;
  int list_length,leng,nr;
  psiconv_u8 id;
  psiconv_u32 temp;
  psiconv_color temp_color;
  psiconv_font temp_font;

  psiconv_progress(config,lev+1,off,"Going to read character layout codes");

  psiconv_progress(config,lev+2,off,"Going to read the list length");
  list_length = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR1;
  psiconv_debug(config,lev+2,off,"Length in bytes: %08x",list_length);
  len += 4;

  nr = 0;
  while(len-4 < list_length) {
    psiconv_progress(config,lev+2,off+len,"Going to read element %d",nr);
    psiconv_progress(config,lev+3,off+len,"Going to read the element id");
    id = psiconv_read_u8(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR1;
    psiconv_debug(config,lev+3,off+len,"Id: %02x",id);
    len ++;
    switch(id) {
      case 0x18:
        psiconv_progress(config,lev+3,off+len,"Going to skip an unknown setting");
        len ++;
        break;
      case 0x19:
        psiconv_progress(config,lev+3,off+len,"Going to read text color");
        if ((res = psiconv_parse_color(config,buf,lev+3,off+len, &leng,&temp_color)))
          goto ERROR1;
        psiconv_free_color(result->color);
        result->color = temp_color;
        len += leng;
        break;
      case 0x1a:
        psiconv_progress(config,lev+3,off+len,"Going to read background color (?)");
        if ((res = psiconv_parse_color(config,buf,lev+2,off+len, &leng,&temp_color)))
          goto ERROR1;
        psiconv_free_color(result->back_color);
        result->back_color = temp_color;
        len += leng;
        break;
      case 0x1b:
        psiconv_progress(config,lev+3,off+len,"Going to skip an unknown setting");
        len ++;
        break;
      case 0x1c:
        psiconv_progress(config,lev+3,off+len,"Going to read font size");
        result->font_size = psiconv_read_size(config,buf,lev+3,off+len,&leng,&res);
        if (res)
          goto ERROR1;
        len += leng;
        break;
      case 0x1d:
        psiconv_progress(config,lev+3,off+len,"Going to read italic");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,&result->italic)))
          goto ERROR1;
        len += leng;
        break;
      case 0x1e:
        psiconv_progress(config,lev+3,off+len,"Going to read bold");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,&result->bold)))
          goto ERROR1;
        len += leng;
        break;
      case 0x1f:
        psiconv_progress(config,lev+3,off+len,"Going to read super_sub");
        temp = psiconv_read_u8(config,buf,lev+3,off+len,&res);
        if (res)
          goto ERROR1;
        if (temp == 0x00)
          result->super_sub = psiconv_normalscript;
        else if (temp == 0x01)
          result->super_sub = psiconv_superscript;
        else if (temp == 0x02)
          result->super_sub = psiconv_subscript;
        else {
          psiconv_warn(config,lev+3,off+len,
               "Unknown super_sub argument in character layout codes list");
        }
        psiconv_debug(config,lev+3,off+len,"Super_sub: %02x",temp);
        len ++;
        break;
      case 0x20:
        psiconv_progress(config,lev+3,off+len,"Going to read underline");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,
                                      &result->underline)))
          goto ERROR1;
        len += leng;
        break;
      case 0x21:
        psiconv_progress(config,lev+3,off+len,"Going to read strikethrough");
        if ((res = psiconv_parse_bool(config,buf,lev+3,off+len,&leng,
                                      &result->strikethrough)))
          goto ERROR1;
        len += leng;
        break;
      case 0x22:
        psiconv_progress(config,lev+3,off+len,"Going to read font");
        if ((res = psiconv_parse_font(config,buf,lev+3,off+len, &leng, &temp_font)))
          goto ERROR1;
        psiconv_free_font(result->font);
        result->font = temp_font;
        len += leng;
        break;
      case 0x23:
        psiconv_progress(config,lev+3,off+len,"Going to skip an unknown setting");
        len ++;
        break;
      case 0x24:
        psiconv_progress(config,lev+3,off+len,
                         "Going to read unknown code 0x24 (%02x expected)", 0);
        temp = psiconv_read_u8(config,buf,lev+3,off+len,&res);
        if (res)
          goto ERROR1;
        if (temp != 0) {
          psiconv_warn(config,lev+3,off+len,
                 "Unknown code 0x24 value != 0x0 (0x%02x)", temp);
        }
        len ++;
        break;
      default:
        psiconv_warn(config,lev+3,off+len,"Unknown code in character layout list");
        psiconv_debug(config,lev+3,off+len,"Code: %02x",id);
        len ++;
        break;
    }
    nr ++;
  }

  if (len - 4 != list_length) {
    psiconv_error(config,lev+2,off+len,
         "Read past end of character layout codes list. I probably lost track "
         "somewhere!");
    psiconv_debug(config,lev+2,off+len,"Read %d characters instead of %d",
                  len-4,list_length);
    res = PSICONV_E_PARSE;
    goto ERROR1;
  }

  len = list_length + 4;

  psiconv_progress(config,lev+1,off+len,
                   "End of character layout list (total length: %08x)",len);

  if (length)
    *length = len;
  return res;

ERROR1:
  psiconv_error(config,lev+1,off,"Reading of character_layout_list failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}
