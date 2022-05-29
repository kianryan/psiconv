/*
    parse_word.c - Part of psiconv, a PSION 5 file formats converter
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

int psiconv_parse_word_status_section(const psiconv_config config,
                                      const psiconv_buffer buf, int lev,
                                      psiconv_u32 off, int *length, 
                                      psiconv_word_status_section *result)
{
  int res=0;
  int len=0;
  psiconv_u32 temp;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read the word status section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Word status section initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the first byte of display flags");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;

  (*result)->show_tabs = temp&0x01 ? psiconv_bool_true : psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show tabs: %02x",(*result)->show_tabs);
  (*result)->show_spaces = temp&0x02 ? psiconv_bool_true : psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show spaces: %02x",(*result)->show_spaces);
  (*result)->show_paragraph_ends = temp &0x04 ? psiconv_bool_true :
                                                psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show paragraph ends: %02x",
                (*result)->show_paragraph_ends);
  (*result)->show_line_breaks = temp & 0x08 ? psiconv_bool_true :
                                              psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show line breaks: %02x",
                (*result)->show_line_breaks);
  (*result)->show_hard_minus = temp & 0x20 ? psiconv_bool_true :
                                             psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show hard minus: %02x",
                (*result)->show_hard_minus);
  (*result)->show_hard_space = temp & 0x40 ? psiconv_bool_true :
                                             psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show hard space: %02x",
                (*result)->show_hard_space);
  if (temp & 0x90) {
    psiconv_warn(config,lev+2,off+len,"Word status section first byte of display "
                               "flags contains unknown flags (ignored)");
    psiconv_debug(config,lev+2,off+len,"Unknown flags: %02x",temp & 0x90);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read second byte of display flags");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;

  (*result)->show_full_pictures = temp & 0x01 ? psiconv_bool_true :
                                                psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show full pictures: %02x",
                (*result)->show_full_pictures);
  (*result)->show_full_graphs = temp & 0x02 ? psiconv_bool_true :
                                              psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show full graphs: %02x",
                               (*result)->show_full_graphs);
  if (temp & 0xfc) {
    psiconv_warn(config,lev+2,off+len,"Word status section second byte of display "
                               "flags contains unknown flags (ignored)");
    psiconv_debug(config,lev+2,off+len,"Unknown flags: %02x",temp & 0xfc);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read top toolbar setting");
  if ((res = psiconv_parse_bool(config,buf,lev+2,off+len,&leng,
             &(*result)->show_top_toolbar)))
    goto ERROR2;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read side toolbar setting");
  if ((res = psiconv_parse_bool(config,buf,lev+2,off+len,&leng,
                                &(*result)->show_side_toolbar)))
    goto ERROR2;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read operational flags");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  (*result)->fit_lines_to_screen = temp & 0x08 ? psiconv_bool_true :
                                                 psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Fit lines to screen: %02x",
                (*result)->fit_lines_to_screen);
  if (temp & 0xf7) {
    psiconv_warn(config,lev+2,off+len,"Word status section operational flags "
                               "contains unknown flags (ignored)");
    psiconv_debug(config,lev+2,off+len,"Unknown flags: %02x",temp & 0xfc);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read cursor position");
  (*result)->cursor_position = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Cursor position: %08x",
                (*result)->cursor_position);
  len += 0x04;

  psiconv_progress(config,lev+2,off+len,"Going to read display size");
  (*result)->display_size = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Display size: %08x",
                (*result)->display_size);
  len += 0x04;

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of word status section (total length: %08x)", len);
  return 0;

ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Word Status Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_word_styles_section(const psiconv_config config,
                                      const psiconv_buffer buf, int lev,
                                      psiconv_u32 off, int *length,
                                      psiconv_word_styles_section *result)
{
  int res=0;
  int len=0;
  int leng,i,nr,j;
  psiconv_word_style style;
  psiconv_u32 temp;

  psiconv_progress(config,lev+1,off,"Going to read the word styles section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read style normal");
  if (!(style = malloc(sizeof(*style))))
    goto ERROR2;
  style->name = NULL;
  if (!(style->paragraph = psiconv_basic_paragraph_layout()))
    goto ERROR2_1;
  psiconv_progress(config,lev+3,off+len,"Going to read the paragraph codes");
  if ((res = psiconv_parse_paragraph_layout_list(config,buf,lev+3,off+len,&leng,
                                             style->paragraph)))
    goto ERROR2_2;
  len += leng;
  psiconv_progress(config,lev+3,off+len,"Going to read the character codes");
  if (!(style->character = psiconv_basic_character_layout()))
    goto ERROR2_2;
  if ((res = psiconv_parse_character_layout_list(config,buf,lev+3,off+len,&leng,
                                             style->character)))
    goto ERROR2_3;
  len += leng;
  /* Ugly: I really don't know whether this is right for UTF8 */
  psiconv_progress(config,lev+3,off+len,"Going to read the hotkey");
  style->hotkey = psiconv_unicode_read_char(config,buf,lev+3,off+len,NULL,&res);
  psiconv_debug(config,lev+3,off+len,"Normal Hotkey value %08x",style->hotkey);
  if (res)
    goto ERROR2_3;
  len += 0x04;
  (*result)->normal = style;

  psiconv_progress(config,lev+2,off+len,"Going to read hotkeys list");
  if (!((*result)->styles = psiconv_list_new(sizeof(*style))))
    goto ERROR3;
  if (!(style = malloc(sizeof(*style)))) {
    goto ERROR3_1;
  }
    
  psiconv_progress(config,lev+3,off+len,"Going to read the number of entries");
  nr = psiconv_read_u8(config,buf,lev+3,off+len,&res);
  if (res)
    goto ERROR3_2;
  len ++;
  psiconv_debug(config,lev+3,off+len,"Nummer of hotkeys: %02x",nr);
  for (i = 0; i < nr; i ++) {
    /* Ugly: I really don't know whether this is right for UTF8 */
    style->hotkey = psiconv_unicode_read_char(config,buf,lev+3,off+len,
	                                      NULL,&res);
    psiconv_debug(config,lev+3,off+len,"Hotkey %d value %08x",i,style->hotkey);
    len += 0x04;
    if ((res = psiconv_list_add((*result)->styles,style)))
      goto ERROR3_2;
  }
  free(style);

  psiconv_progress(config,lev+2,off+len,"Going to read all other styles");
  psiconv_progress(config,lev+2,off+len,"Going to read the number of styles");
  nr = psiconv_read_u8(config,buf,lev+3,off+len,&res);
  if (res)
    goto ERROR4;
  if (nr != psiconv_list_length((*result)->styles)) {
    psiconv_warn(config,lev+3,off+len,"Number of styles and hotkeys do not match");
    psiconv_debug(config,lev+3,off+len,"%d hotkeys, %d styles",
          psiconv_list_length((*result)->styles), nr);
  }
  len ++;

  for (i = 0; i < nr; i++) {
    psiconv_progress(config,lev+2,off+len,"Next style: %d",i);
    if (i >= psiconv_list_length((*result)->styles)) {
      if (!(style = malloc(sizeof(*style))))
        goto ERROR5;
      style->hotkey = 0;
      if (psiconv_list_add((*result)->styles,style)) {
        free(style);
        goto ERROR5;
      }
      psiconv_debug(config,lev+3,off+len,"New entry added in list");
      free(style);
    }
    if (!(style = psiconv_list_get((*result)->styles,i))) 
      goto ERROR5;
    psiconv_progress(config,lev+3,off+len,"Going to read the style name");
    style->name = psiconv_read_string(config,buf,lev+3,off+len,&leng,&res);
    if (res)
      goto ERROR5;
    len += leng;
    psiconv_progress(config,lev+3,off+len,
                     "Going to read whether this style is built-in");
    temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR6;

    if (temp == PSICONV_ID_STYLE_BUILT_IN) {
      style->built_in = psiconv_bool_true;
      psiconv_debug(config,lev+3,off+len,"Built-in style");
    } else if (temp == PSICONV_ID_STYLE_REMOVABLE) {
      style->built_in = psiconv_bool_false;
      psiconv_debug(config,lev+3,off+len,"Removable style");
    } else {
      psiconv_warn(config,lev+3,off+len,
               "Word styles section unknown style id (treated as built-in)");
      psiconv_debug(config,lev+3,off+len,"Unknown id: %08x",temp);
      style->built_in = psiconv_bool_false;
    }
    len += 4;
    psiconv_progress(config,lev+3,off+len,"Going to read outline level");
    style->outline_level = psiconv_read_u32(config,buf,lev+3,off+len,&res);
    if (res)
      goto ERROR6;
    psiconv_debug(config,lev+3,off+len,"Outline Level: %08x", style->outline_level);
    len += 4;
    psiconv_progress(config,lev+3,off+len,"Going to read the character codes");
    if (!(style->character = psiconv_clone_character_layout((*result)->normal->character)))
      goto ERROR6;
    if ((res = psiconv_parse_character_layout_list(config,buf,lev+3,off+len,&leng,
                                               style->character)))
      goto ERROR7;
    len += leng;
    psiconv_progress(config,lev+3,off+len,"Going to read the paragraph codes");
    if (!(style->paragraph = psiconv_clone_paragraph_layout((*result)->normal->paragraph)))
      goto ERROR7;
    if ((res = psiconv_parse_paragraph_layout_list(config,buf,lev+3,off+len,&leng,
                                               style->paragraph)))
      goto ERROR8;
    len += leng;
  }

  psiconv_progress(config,lev+2,off+len,"Reading trailing bytes");
  for (i = 0; i < psiconv_list_length((*result)->styles); i++) {
    temp = psiconv_read_u8(config,buf,lev+3,off+len,&res);
    if (res)
      goto ERROR4;
    if (temp != 0xff) {
      psiconv_warn(config,lev+3,off+len,"Unknown trailing style byte"); 
      psiconv_debug(config,lev+3,off+len,"Trailing byte: %02x expected, read %02x",
                    0xff,temp); 
    } else
      psiconv_debug(config,lev+3,off+len,"Read trailing byte 0xff");
    len++;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of word styles section (total length: %08x)", len);

  return res;


ERROR3_2:
  free(style);
ERROR3_1:
  psiconv_list_free((*result)->styles);
goto ERROR3;

ERROR2_3:
  psiconv_free_character_layout(style->character);
ERROR2_2:
  psiconv_free_paragraph_layout(style->paragraph);
ERROR2_1:
  free (style);
goto ERROR2;

ERROR8:
  psiconv_free_paragraph_layout(style->paragraph);
ERROR7:
  psiconv_free_character_layout(style->character);
ERROR6:
  free(style->name);
ERROR5:
  for (j = 0; j < i ;j++) {
    if (!(style = psiconv_list_get((*result)->styles,j))) {
      psiconv_error(config,lev+1,off,"Data structure corruption");
      goto ERROR4;
    }
    psiconv_free_character_layout(style->character);
    psiconv_free_paragraph_layout(style->paragraph);
    free(style->name);
  }
ERROR4:
  psiconv_list_free((*result)->styles);
ERROR3:
  psiconv_free_word_style((*result)->normal);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Word Status Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}



