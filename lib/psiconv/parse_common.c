/*
    parse_common.c - Part of psiconv, a PSION 5 file formats converter
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


static int psiconv_parse_layout_section(const psiconv_config config,
                                 const psiconv_buffer buf,
                                 int lev,psiconv_u32 off,
                                 int *length,
                                 psiconv_text_and_layout result,
                                 psiconv_word_styles_section styles,
                                 int with_styles);
static psiconv_file_type_t psiconv_determine_embedded_object_type
                                       (const psiconv_config config,
					const psiconv_buffer buf,int lev,
                                        int *status);

int psiconv_parse_header_section(const psiconv_config config,
                                 const psiconv_buffer buf,int lev,
                                 psiconv_u32 off, int *length, 
                                 psiconv_header_section *result)
{
  int res=0;
  int len=0;
  psiconv_u32 temp;

  psiconv_progress(config,lev+1,off+len,"Going to read the header section");
  if (!((*result) = malloc(sizeof(**result))))
    goto ERROR1;
  
  psiconv_progress(config,lev+2,off+len,"Going to read UID1 to UID3");
  (*result)->uid1 = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"UID1: %08x",(*result)->uid1);
  if ((*result)->uid1 == PSICONV_ID_CLIPART) {
    /* That's all folks... */
    (*result)->file = psiconv_clipart_file;
    (*result)->uid2 = 0;
    (*result)->uid3 = 0;
    (*result)->checksum = 0;
    len += 4;
    psiconv_debug(config,lev+2,off+len,"File is a Clipart file");
    goto DONE;
  }
  if ((*result)->uid1 != PSICONV_ID_PSION5) {
    psiconv_error(config,lev+2,off+len,
	                       "UID1 has unknown value. This is probably "
                               "not a (parsable) Psion 5 file");
    res = -PSICONV_E_PARSE;
    goto ERROR2;
  }
  len += 4;
  (*result)->uid2 = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"UID2: %08x",(*result)->uid2);
  len += 4;
  (*result)->uid3 = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"UID3: %08x",(*result)->uid3);
  len += 4;

  (*result)->file = psiconv_unknown_file;
  if ((*result)->uid1 == PSICONV_ID_PSION5) {
    if ((*result)->uid2 == PSICONV_ID_DATA_FILE) {
      if ((*result)->uid3 == PSICONV_ID_WORD) {
        (*result)->file = psiconv_word_file;
        psiconv_debug(config,lev+2,off+len,"File is a Word file");
      } else if ((*result)->uid3 == PSICONV_ID_TEXTED) {
        (*result)->file = psiconv_texted_file;
        psiconv_debug(config,lev+2,off+len,"File is a TextEd file");
      } else if ((*result)->uid3 == PSICONV_ID_SKETCH) {
        (*result)->file = psiconv_sketch_file;
        psiconv_debug(config,lev+2,off+len,"File is a Sketch file");
      } else if ((*result)->uid3 == PSICONV_ID_SHEET) {
        (*result)->file = psiconv_sheet_file;
        psiconv_debug(config,lev+2,off+len,"File is a Sheet file");
      }
    } else if ((*result)->uid2 == PSICONV_ID_MBM_FILE) {
      (*result)->file = psiconv_mbm_file;
      if ((*result)->uid3 != 0x00)
        psiconv_warn(config,lev+2,off+len,"UID3 set in MBM file?!?");
      psiconv_debug(config,lev+2,off+len,"File is a MBM file");
    }  
  }
  if ((*result)->file == psiconv_unknown_file) {
    psiconv_warn(config,lev+2,off+len,"Unknown file type");
    (*result)->file = psiconv_unknown_file;
  }

  psiconv_progress(config,lev+2,off+len,"Checking UID4");
  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp == psiconv_checkuid((*result)->uid1,(*result)->uid2,
                               (*result)->uid3)) 
    psiconv_debug(config,lev+2,off+len,"Checksum %08x is correct",temp);
  else {
    psiconv_error(config,lev+2,off+len,"Checksum failed, file corrupted!");
    psiconv_debug(config,lev+2,off+len,"Expected checksum %08x, found %08x",
                  psiconv_checkuid((*result)->uid1,(*result)->uid2,
                                   (*result)->uid3),temp);
    res = -PSICONV_E_PARSE;
    goto ERROR2;
  }
  len += 4;

DONE:
  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,
                   "End of Header Section (total length: %08x)",len);
 
  return res;

ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Header Section failed");
  if (length)
    *length = 0;
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_section_table_section(const psiconv_config config,
                                        const psiconv_buffer buf, int lev,
                                        psiconv_u32 off, int *length,
                                        psiconv_section_table_section *result)
{
  int res=0;
  int len=0;
  psiconv_section_table_entry entry;

  int i;
  psiconv_u8 nr;

  psiconv_progress(config,lev+1,off+len,"Going to read the section table section");
  if (!(*result = psiconv_list_new(sizeof(*entry))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read the section table length");
  nr = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Length: %08x",nr);
  if (nr & 0x01) {
    psiconv_warn(config,lev+2,off+len,
                 "Section table length odd - ignoring last entry");
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read the section table entries");
  entry = malloc(sizeof(*entry));
  for (i = 0; i < nr / 2; i++) {
    entry->id = psiconv_read_u32(config,buf,lev+2,off + len,&res);
    if (res)
      goto ERROR3;
    psiconv_debug(config,lev+2,off + len,"Entry %d: ID = %08x",i,entry->id);
    len += 0x04;
    entry->offset = psiconv_read_u32(config,buf,lev+2,off + len,&res);
    if (res)
      goto ERROR3;
    psiconv_debug(config,lev+2,off +len,"Entry %d: Offset = %08x",i,entry->offset);
    len += 0x04;
    if ((res=psiconv_list_add(*result,entry)))
      goto ERROR3;
  }

  free(entry);

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of section table section "
                   "(total length: %08x)", len);

  return 0;
ERROR3:
  free(entry);
ERROR2:
  psiconv_list_free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Section Table Section failed");
  if (length)
    *length = 0;
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_application_id_section(const psiconv_config config,
                                         const psiconv_buffer buf, int lev, 
                                         psiconv_u32 off, int *length,
                                         psiconv_application_id_section *result)
{
  int res=0;
  int len=0;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read the application id section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read the type identifier");
  (*result)->id = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Identifier: %08x",(*result)->id);
  len += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read the application id string");
  (*result)->name = psiconv_read_string(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  len += leng;

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of application id section "
                   "(total length: %08x", len);

  return res;
ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Application ID Section failed");
  if (length)
    *length = 0;
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_text_section(const psiconv_config config,
                               const psiconv_buffer buf,int lev,psiconv_u32 off,
                               int *length,psiconv_text_and_layout *result)
{

  int res = 0;
  int len=0;

  psiconv_u32 text_len;
  psiconv_paragraph para;
  psiconv_ucs2 temp;
  psiconv_list line;

  int nr;
  int i,leng;
  char *str_copy;
 
  psiconv_progress(config,lev+1,off,"Going to parse the text section");

  if(!(*result = psiconv_list_new(sizeof(*para))))
    goto ERROR1;
  if (!(para = malloc(sizeof(*para))))
    goto ERROR2;

  psiconv_progress(config,lev+2,off,"Reading the text length");
  text_len = psiconv_read_X(config,buf,lev+2,off,&leng,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off,"Length: %08x",text_len);
  len += leng;

  if (!(line = psiconv_list_new(sizeof(psiconv_ucs2))))
    goto ERROR3;

  i = 0;
  nr = 0;
  while (i < text_len) {
    temp = psiconv_unicode_read_char(config,buf,lev+2,off+len+i,&leng,&res);
    if (res)
      goto ERROR4;
    if (i + leng > text_len) {
      psiconv_error(config,lev+2,off+len+i,"Malformed text section");
      res = PSICONV_E_PARSE;
      goto ERROR4;
    }
    if ((temp == 0x06) || (i + leng == text_len)) {
      if (!(para->text = psiconv_unicode_from_list(line))) 
	goto ERROR4;

      if (!(str_copy = psiconv_make_printable(config,para->text)))
        goto ERROR5;
      psiconv_debug(config,lev+2,off+i+len,"Line %d: %d characters",nr,
                    strlen(str_copy) +1);
      psiconv_debug(config,lev+2,off+i+len,"Line %d: `%s'",nr,str_copy);
      free(str_copy);
      i += leng;

      if (!(para->in_lines = psiconv_list_new(sizeof(
				struct psiconv_in_line_layout_s))))
	goto ERROR5;
      if (!(para->replacements = psiconv_list_new(sizeof(
				struct psiconv_replacement_s)))) 
	goto ERROR6;
      if (!(para->base_character = psiconv_basic_character_layout()))
	 goto ERROR7;
      if (!(para->base_paragraph = psiconv_basic_paragraph_layout()))
	 goto ERROR8;
      para->base_style = 0;

      if ((res = psiconv_list_add(*result,para)))
        goto ERROR9;
      psiconv_progress(config,lev+2,off+len+i,"Starting a new line");
      psiconv_list_empty(line);
      nr ++;
    } else {
      if ((res = psiconv_list_add(line,&temp)))
	goto ERROR4;
      i += leng;
    }
  }

  psiconv_list_free(line);
  free(para);

  len += text_len;

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,
                   "End of text section (total length: %08x", len);

  return res;

ERROR9:
  psiconv_free_paragraph_layout(para->base_paragraph);
ERROR8:
  psiconv_free_character_layout(para->base_character);
ERROR7:
  psiconv_list_free(para->replacements);
ERROR6:
  psiconv_list_free(para->in_lines);
ERROR5:
  free(para->text);
ERROR4:
  psiconv_list_free(line);
ERROR3:
  free(para);
ERROR2:
  psiconv_free_text_and_layout(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Text Section failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

/* First do a parse_text_section, or you will get into trouble here */
int psiconv_parse_layout_section(const psiconv_config config,
                                 const psiconv_buffer buf,
                                 int lev,psiconv_u32 off,
                                 int *length,
                                 psiconv_text_and_layout result,
                                 psiconv_word_styles_section styles,
                                 int with_styles)
{
  int res = 0;
  int len = 0;
  psiconv_u32 temp;
  int parse_styles,nr,i,j,total,leng,line_length;

  typedef struct anon_style_s
  {
    int nr;
    psiconv_s16 base_style;
    psiconv_character_layout character;
    psiconv_paragraph_layout paragraph;
  } *anon_style;

  typedef psiconv_list anon_style_list; /* of struct anon_style */

  anon_style_list anon_styles;
  struct anon_style_s anon;
  anon_style anon_ptr=NULL;

  psiconv_character_layout temp_char;
  psiconv_paragraph_layout temp_para;
  psiconv_word_style temp_style;
  psiconv_paragraph para;
  struct psiconv_in_line_layout_s in_line;

  int *inline_count;


  psiconv_progress(config,lev+1,off,"Going to read the layout section");

  psiconv_progress(config,lev+2,off,"Going to read the section type");
  temp = psiconv_read_u16(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  psiconv_debug(config,lev+2,off+len,"Type: %02x",temp);
  parse_styles = with_styles;
  if ((temp == 0x0001) && !with_styles) {
    psiconv_warn(config,lev+2,off+len,"Styleless layout section expected, "
                 "but styled section found!");
    parse_styles = 1;
  } else if ((temp == 0x0000) && (with_styles)) {
    psiconv_warn(config,lev+2,off+len,"Styled layout section expected, "
                 "but styleless section found!");
    parse_styles = 0;
  } else if ((temp != 0x0000) && (temp != 0x0001)) {
    psiconv_warn(config,lev+2,off+len,
                 "Layout section type indicator has unknown value!");
  }
  len += 0x02;

  psiconv_progress(config,lev+2,off+len,"Going to read paragraph type list");
  if (!(anon_styles = psiconv_list_new(sizeof(anon))))
    goto ERROR1;
  psiconv_progress(config,lev+3,off+len,"Going to read paragraph type list length");
  nr = psiconv_read_u8(config,buf,lev+3,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+3,off+len,"Length: %02x",nr);
  len ++;

  psiconv_progress(config,lev+3,off+len,
                   "Going to read the paragraph type list elements");
  for (i = 0; i < nr; i ++) {
    psiconv_progress(config,lev+3,off+len,"Element %d",i);
    anon.nr = psiconv_read_u32(config,buf,lev+4,off+len,&res);
    if (res) 
      goto ERROR3;
    psiconv_debug(config,lev+4,off+len,"Number: %08x",anon.nr);
    len += 0x04;
  
    psiconv_progress(config,lev+4,off,"Going to determine the base style");
    if (parse_styles) {
      temp = psiconv_read_u32(config,buf,lev+4, off+len,&res);
      if (res)
        goto ERROR3;
      anon.base_style = psiconv_read_u8(config,buf,lev+3, off+len+4+temp,&res);
      if (res)
        goto ERROR3;
      psiconv_debug(config,lev+4,off+len+temp,
                    "Style indicator: %02x",anon.base_style);
    } else
      anon.base_style = 0;
    if (!(temp_style = psiconv_get_style(styles,anon.base_style))) {
      psiconv_warn(config,lev+4,off,"Unknown Style referenced");
      if (!(temp_style = psiconv_get_style(styles,anon.base_style))) {
        psiconv_warn(config,lev+4,off,"Base style unknown");
        goto ERROR3;
      }
    }
    if (!(anon.paragraph = psiconv_clone_paragraph_layout
                                              (temp_style->paragraph)))
      goto ERROR3;
    if (!(anon.character = psiconv_clone_character_layout
                                              (temp_style->character)))
      goto ERROR3_1;

    psiconv_progress(config,lev+4,off+len,"Going to read the paragraph layout");
    if ((res = psiconv_parse_paragraph_layout_list(config,buf,lev+4,off+len,&leng,
                                               anon.paragraph)))
       goto ERROR3_2;
    len += leng;
    if (parse_styles)
      len ++;

    psiconv_progress(config,lev+4,off+len,"Going to read the character layout");
    if ((res = psiconv_parse_character_layout_list(config,buf,lev+4,off+len,&leng,
                                               anon.character)))
      goto ERROR3_2;
    len += leng;
    if ((res = psiconv_list_add(anon_styles,&anon)))
      goto ERROR3_2;
  }

  psiconv_progress(config,lev+2,off+len,"Going to parse the paragraph element list");
  psiconv_progress(config,lev+3,off+len,"Going to read the number of paragraphs");
  nr = psiconv_read_u32(config,buf,lev+3,off+len,&res);
  if (res)
    goto ERROR3;
  if (nr != psiconv_list_length(result)) {
    psiconv_warn(config,lev+3,off+len,
         "Number of text paragraphs and paragraph elements does not match");
    psiconv_debug(config,lev+3,off+len,
          "%d text paragraphs, %d paragraph elements",
          psiconv_list_length(result),nr);
  }
  psiconv_debug(config,lev+3,off+len,"Number of paragraphs: %d",nr);
  len += 4;
  if (!(inline_count = malloc(nr * sizeof(*inline_count))))
    goto ERROR3;

  psiconv_progress(config,lev+3,off+len,"Going to read the paragraph elements");
  for (i = 0; i < nr; i ++) {
    psiconv_progress(config,lev+3,off+len,"Element %d",i);
    if (i >= psiconv_list_length(result)) {
      psiconv_debug(config,lev+4,off+len,"Going to allocate a new element");
      if (!(para = malloc(sizeof(*para))))
        goto ERROR4;
      if (!(para->in_lines = psiconv_list_new(sizeof(
                              struct psiconv_in_line_layout_s))))
        goto ERROR4_1;
      para->base_style = 0;
      if (!(para->base_character = psiconv_basic_character_layout()))
        goto ERROR4_2;
      if (!(para->base_paragraph = psiconv_basic_paragraph_layout()))
        goto ERROR4_3;
      if ((res = psiconv_list_add(result,para)))
        goto ERROR4_4;
      free(para);
    }
    if (!(para = psiconv_list_get(result,i)))
      goto ERROR4;

    psiconv_progress(config,lev+4,off+len,"Going to read the paragraph length");
    temp = psiconv_read_u32(config,buf,lev+4,off+len,&res);
    if (res)
      goto ERROR4;
    if (temp != psiconv_unicode_strlen(para->text)+1) {
      psiconv_warn(config,lev+4,off+len,
                   "Disagreement of the length of paragraph in layout section");
      psiconv_debug(config,lev+4,off+len,
                    "Paragraph length: layout section says %d, counted %d",
                    temp,psiconv_unicode_strlen(para->text)+1);
    } else
      psiconv_debug(config,lev+4,off+len,"Paragraph length: %d",temp);
    len += 4;

    psiconv_progress(config,lev+4,off+len,"Going to read the paragraph type");
    temp = psiconv_read_u8(config,buf,lev+4,off+len,&res);
    if (res)
       goto ERROR4;
    if (temp != 0x00) {
      psiconv_debug(config,lev+4,off+len,"Type: %02x",temp);
      for (j = 0; j < psiconv_list_length(anon_styles); j++) {
        if (!(anon_ptr = psiconv_list_get(anon_styles,j))) {
          psiconv_error(config,lev+4,off+len,"Data structure corruption");
          goto ERROR4;
        }
        if (temp == anon_ptr->nr)
          break;
      }
      if (j == psiconv_list_length(anon_styles)) {
        psiconv_warn(config,lev+4,off+len,"Layout section paragraph type unknown");
        psiconv_debug(config,lev+4,off+len,"Unknown type - using base styles instead");
        para->base_style = 0;
        if (!(temp_style = psiconv_get_style(styles,0))) {
          psiconv_error(config,lev+4,off,"Base style unknown");
          goto ERROR4;
        }
        if (!(temp_para = psiconv_clone_paragraph_layout
                                               (temp_style->paragraph)))
          goto ERROR4;
        psiconv_free_paragraph_layout(para->base_paragraph);
        para->base_paragraph = temp_para;

        if (!(temp_char = psiconv_clone_character_layout
                                               (temp_style->character)))
          goto ERROR4;
        psiconv_free_character_layout(para->base_character);
        para->base_character = temp_char;
      } else {
        para->base_style = anon_ptr->base_style;
        if (!(temp_para = psiconv_clone_paragraph_layout (anon_ptr->paragraph)))
          goto ERROR4;
        psiconv_free_paragraph_layout(para->base_paragraph);
        para->base_paragraph = temp_para;

        if (!(temp_char = psiconv_clone_character_layout (anon_ptr->character)))
          goto ERROR4;
        psiconv_free_character_layout(para->base_character);
        para->base_character = temp_char;
      }
      inline_count[i] = 0;
      len += 0x01;
    } else {
      psiconv_debug(config,lev+4,off+len,"Type: %02x (not based on a paragraph type)"
                    ,temp);
      len += 0x01;
      if (parse_styles) {
        temp = psiconv_read_u32(config,buf,lev+4,off+len,&res);
        if (res)
          goto ERROR4;
        psiconv_progress(config,lev+4,off+len+temp+4,
                         "Going to read the paragraph element base style");
        temp = psiconv_read_u8(config,buf,lev+4, off+len+temp+4,&res);
        if (res)
          goto ERROR4;
        psiconv_debug(config,lev+4,off+len+temp+4, "Style: %02x",temp);
      } else
        temp = 0x00;

      if (!(temp_style = psiconv_get_style (styles,temp))) {
        psiconv_warn(config,lev+4,off,"Unknown Style referenced");
        if (!(temp_style = psiconv_get_style(styles,0))) {
          psiconv_error(config,lev+4,off,"Base style unknown");
          goto ERROR4;
        }
      }

      if (!(temp_para = psiconv_clone_paragraph_layout(temp_style->paragraph)))
        goto ERROR4;
      psiconv_free_paragraph_layout(para->base_paragraph);
      para->base_paragraph = temp_para;

      if (!(temp_char = psiconv_clone_character_layout(temp_style->character)))
        goto ERROR4;
      psiconv_free_character_layout(para->base_character);
      para->base_character = temp_char;

      para->base_style = temp;
      psiconv_progress(config,lev+4,off+len,"Going to read paragraph layout");
      if ((res = psiconv_parse_paragraph_layout_list(config,buf,lev+4,off+len,&leng,
                                                para->base_paragraph)))
        goto ERROR4;
      len += leng;
      if (parse_styles)
        len += 1;
      psiconv_progress(config,lev+4,off+len,"Going to read number of in-line "
                       "layout elements");
      inline_count[i] = psiconv_read_u32(config,buf,lev+4,off+len,&res);
      if (res)
        goto ERROR4;
      psiconv_debug(config,lev+4,off+len,"Nr: %08x",inline_count[i]);
      len += 4;
    }
  }
        
  psiconv_progress(config,lev+2,off+len,"Going to read the text layout inline list");

  psiconv_progress(config,lev+3,off+len,"Going to read the number of elements");
  nr = psiconv_read_u32(config,buf,lev+3,off+len,&res);
  if (res)
    goto ERROR4;
  psiconv_debug(config,lev+3,off+len,"Elements: %08x",nr);
  len += 0x04;

  psiconv_progress(config,lev+3,off+len,
                   "Going to read the text layout inline elements");
  total = 0;
  for (i = 0; i < psiconv_list_length(result); i++) {
    if (!(para = psiconv_list_get(result,i))) {
      psiconv_error(config,lev+3,off+len,"Data structure corruption");
      goto ERROR4;
    }
    line_length = -1;
    for (j = 0; j < inline_count[i]; j++) {
      psiconv_progress(config,lev+3,off+len,"Element %d: Paragraph %d, element %d",
                        total,i,j);
      if (total >= nr) {
        psiconv_warn(config,lev+3,off+len,
                     "Layout section inlines: not enough element");
        psiconv_debug(config,lev+3,off+len,"Can't read element!");
      } else {
        total ++;
	in_line.object = NULL;
	in_line.layout = NULL;
        if (!(in_line.layout = psiconv_clone_character_layout
                 (para->base_character)))
          goto ERROR4;
        psiconv_progress(config,lev+4,off+len,"Going to read the element type");
        temp = psiconv_read_u8(config,buf,lev+4,len+off,&res);
        if (res)
          goto ERROR5;
        len += 1;
        psiconv_debug(config,lev+4,off+len,"Type: %02x",temp);
        psiconv_progress(config,lev+4,off+len,
                      "Going to read the number of characters it applies to");
        in_line.length = psiconv_read_u32(config,buf,lev+4,len+off,&res);
        if (res)
          goto ERROR5;
        psiconv_debug(config,lev+4,off+len,"Length: %02x",in_line.length);
        len += 4;
        psiconv_progress(config,lev+4,off+len,"Going to read the character layout");
        if ((res = psiconv_parse_character_layout_list(config,buf,lev+4,off+len,&leng,
                                                   in_line.layout)))
          goto ERROR5;
        len += leng;

        if (temp == 0x01) {
	  psiconv_debug(config,lev+4,off+len,"Found an embedded object");
	  psiconv_progress(config,lev+4,off+len,"Going to read the object marker "
	                                 "(0x%08x expected)",PSICONV_ID_OBJECT);
	  temp = psiconv_read_u32(config,buf,lev+4,off+len,&res);
	  if (res)
	    goto ERROR5;
	  if (temp != PSICONV_ID_OBJECT) {
	    psiconv_warn(config,lev+4,off+len,"Unknown id marks embedded object");
	    psiconv_debug(config,lev+4,off+len,"Marker: read %08x, expected %08x",
		          temp,PSICONV_ID_OBJECT);
	  }
	  len += 4;
	  psiconv_progress(config,lev+4,off+len,
	                   "Going to read the Embedded Object Section offset");
	  temp = psiconv_read_u32(config,buf,lev+4,off+len,&res);
	  if (res)
	    goto ERROR5;
	  psiconv_debug(config,lev+4,off+len, "Offset: %08x",temp);
	  len += 4;
	  psiconv_progress(config,lev+4,off+len,
	                   "Going to parse the Embedded Object Section");
	  if ((res = psiconv_parse_embedded_object_section(config,buf,lev+4,temp,
	                                              NULL,&(in_line.object))))
            goto ERROR5;
	  psiconv_progress(config,lev+4,off+len,
	                   "Going to read the object width");
	  in_line.object_width = psiconv_read_length(config,buf,lev+4,off+len,
	                                             &leng,&res);
	  if (res)
	    goto ERROR5;
	  psiconv_debug(config,lev+4,off+len,"Object width: %f cm",
	                in_line.object_width);
	  len += leng;
	  psiconv_progress(config,lev+4,off+len,
	                   "Going to read the object height");
	  in_line.object_height = psiconv_read_length(config,buf,lev+4,off+len,&leng,
	                                              &res);
	  if (res)
	    goto ERROR5;
	  psiconv_debug(config,lev+4,off+len,"Object height: %f cm",
	                in_line.object_height);
	  len += leng;
        } else if (temp != 0x00) {
          psiconv_warn(config,lev+4,off+len,"Layout section unknown inline type");
        }
        if (line_length + in_line.length > psiconv_unicode_strlen(para->text)) {
          psiconv_warn(config,lev+4,off+len,
                       "Layout section inlines: line length mismatch");
          res = -1;
          in_line.length = psiconv_unicode_strlen(para->text) - line_length;
        }
        line_length += in_line.length;
        if ((res = psiconv_list_add(para->in_lines,&in_line)))
          goto ERROR5;
      }
    }
  }    
  
  if (total != nr) {
    psiconv_warn(config,lev+4,off+len,
                 "Layout section too many inlines, skipping remaining");
  }

  free(inline_count);

  for (i = 0 ; i < psiconv_list_length(anon_styles); i ++) {
    if (!(anon_ptr = psiconv_list_get(anon_styles,i))) {
      psiconv_error(config,lev+4,off+len,"Data structure corruption");
      goto ERROR2;
    }
    psiconv_free_character_layout(anon_ptr->character);
    psiconv_free_paragraph_layout(anon_ptr->paragraph);
  }
  psiconv_list_free(anon_styles);

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of layout section (total length: %08x)",
                   len);

  return 0;

ERROR4_4:
  psiconv_free_paragraph_layout(para->base_paragraph);
ERROR4_3:
  psiconv_free_character_layout(para->base_character);
ERROR4_2:
  psiconv_list_free(para->in_lines);
ERROR4_1:
  free(para);
  goto ERROR4;

ERROR3_2:
  psiconv_free_character_layout(anon.character);
ERROR3_1:
  psiconv_free_paragraph_layout(anon.paragraph);
  goto ERROR3;

ERROR5:
  if (in_line.layout)
    psiconv_free_character_layout(in_line.layout);
  if (in_line.object)
    psiconv_free_embedded_object_section(in_line.object);
ERROR4:
  free(inline_count);
ERROR3:
  for (i = 0; i < psiconv_list_length(anon_styles); i++) {
    if (!(anon_ptr = psiconv_list_get(anon_styles,i))) {
      psiconv_error(config,lev+1,off,"Data structure corruption");
      break;
    }
    psiconv_free_paragraph_layout(anon_ptr->paragraph);
    psiconv_free_character_layout(anon_ptr->character);
  }

ERROR2:
  psiconv_list_free(anon_styles);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Layout Section failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}
      
int psiconv_parse_styled_layout_section(const psiconv_config config,
                                        const psiconv_buffer buf,
                                        int lev,psiconv_u32 off,
                                        int *length,
                                        psiconv_text_and_layout result,
                                        psiconv_word_styles_section styles)
{
  return psiconv_parse_layout_section(config,buf,lev,off,length,result,styles,1);
}

int psiconv_parse_styleless_layout_section(const psiconv_config config,
                                     const psiconv_buffer buf,
                                     int lev,psiconv_u32 off,
                                     int *length,
                                     psiconv_text_and_layout result,
                                     const psiconv_character_layout base_char,
                                     const psiconv_paragraph_layout base_para)
{
  int res = 0;
  psiconv_word_styles_section styles_section;

  if (!(styles_section = malloc(sizeof(*styles_section))))
    goto ERROR1;
  if (!(styles_section->normal = malloc(sizeof(*styles_section->normal))))
    goto ERROR2;
  if (!(styles_section->normal->character = 
                            psiconv_clone_character_layout(base_char)))
    goto ERROR3;
  if (!(styles_section->normal->paragraph = 
                            psiconv_clone_paragraph_layout(base_para)))
    goto ERROR4;
  styles_section->normal->hotkey = 0;

  if (!(styles_section->normal->name = psiconv_unicode_empty_string()))
    goto ERROR5;
  if (!(styles_section->styles = psiconv_list_new(sizeof(
                                        struct psiconv_word_style_s))))
    goto ERROR6;
  
  res = psiconv_parse_layout_section(config,buf,lev,off,length,result,
                                     styles_section,0);

  psiconv_free_word_styles_section(styles_section);
  return res;

ERROR6:
  free(styles_section->normal->name);
ERROR5:
  psiconv_free_paragraph_layout(styles_section->normal->paragraph);
ERROR4:
  psiconv_free_character_layout(styles_section->normal->character);
ERROR3:
  free(styles_section->normal);
ERROR2:
  free(styles_section);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Styleless Layout Section failed");
  if (length)
    *length = 0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_embedded_object_section(const psiconv_config config,
                                  const psiconv_buffer buf, int lev,
                                  psiconv_u32 off, int *length,
                                  psiconv_embedded_object_section *result)
{
  int res=0;
  int len=0;
  int leng,i;
  psiconv_section_table_section table;
  psiconv_section_table_entry entry;
  psiconv_u32 icon_sec=0,display_sec=0,table_sec=0;
  psiconv_buffer subbuf;

  psiconv_progress(config,lev+1,off+len,"Going to read an Embedded Object");
  if (!(*result = malloc(sizeof(**result))))
      goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read the Embedded Object Section");
  psiconv_parse_section_table_section(config,buf,lev+2,off+len,&leng,&table);
  len += leng;

  for (i = 0; i < psiconv_list_length(table); i++) {
    psiconv_progress(config,lev+2,off+len,"Going to read entry %d",i);
    if (!(entry = psiconv_list_get(table,i)))
      goto ERROR2;
    if (entry->id == PSICONV_ID_OBJECT_DISPLAY_SECTION) {
      display_sec = entry->offset;
      psiconv_debug(config,lev+3,off+len,"Found the Object Display Section at %08x",
	            display_sec);
    } else if (entry->id == PSICONV_ID_OBJECT_ICON_SECTION) {
      icon_sec = entry->offset;
      psiconv_debug(config,lev+3,off+len,"Found the Object Icon Section at %08x",
	            icon_sec);
    } else if (entry->id == PSICONV_ID_OBJECT_SECTION_TABLE_SECTION) {
      table_sec = entry->offset;
      psiconv_debug(config,lev+3,off+len,"Found the Object Section Table Section at %08x",
	            table_sec);
    } else {
      psiconv_warn(config,lev+3,off+len,
	           "Found unknown section in the Object Display Section "
		   "(ignoring)");
      psiconv_debug(config,lev+3,off+len,"Section ID %08x, offset %08x", 
	            entry->id, entry->offset);
    }
  }

  psiconv_progress(config,lev+2,off+len,"Looking for the Object Display Section");
  if (!icon_sec) {
    psiconv_warn(config,lev+2,off+len,"Object Display Section not found");
    (*result)->display = NULL;
  } else {
    psiconv_debug(config,lev+2,off+len,"Object Display Section at offset %08x",
	          display_sec);
    if ((res = psiconv_parse_object_display_section(config,buf,lev+2,display_sec,NULL,
	                                            &(*result)->display))) 
      goto ERROR2;
  }

  psiconv_progress(config,lev+2,off+len,"Looking for the Object Icon Section");
  if (!icon_sec) {
    psiconv_warn(config,lev+2,off+len,"Object Icon Section not found");
    (*result)->icon = NULL;
  } else {
    psiconv_debug(config,lev+2,off+len,"Object Icon Section at offset %08x",icon_sec);
    if ((res = psiconv_parse_object_icon_section(config,buf,lev+2,icon_sec,NULL,
	                                         &(*result)->icon))) 
      goto ERROR3;
  }

  psiconv_progress(config,lev+2,off+len,
                   "Looking for the Section Table Offset Section");
  if (!table_sec) {
    psiconv_warn(config,lev+2,off+len,
	         "Embedded Section Table Offset Section not found");
    (*result)->object = NULL;
  } else {
    psiconv_progress(config,lev+2,off+len,
	             "Extracting object: add %08x to all following offsets",
                     table_sec);
    /* We can't determine the length of the object, so we just take it all */
    if ((res = psiconv_buffer_subbuffer(&subbuf,buf,table_sec,
	                         psiconv_buffer_length(buf)-table_sec))) 
      goto ERROR4;
    
    if (!((*result)->object = malloc(sizeof(*(*result)->object))))
       goto ERROR5;

    /* We need to find the file type, but we don't have a normal header */
    /* So we try to find the Application ID Section and hope for the best */
    psiconv_progress(config,lev+3,0,"Trying to determine the file type");
    (*result)->object->type = psiconv_determine_embedded_object_type
                                                       (config,subbuf,lev+3,&res);
    switch ((*result)->object->type) {
      case psiconv_word_file:
        if ((res = psiconv_parse_word_file(config,subbuf,lev+3,0,
                           ((psiconv_word_f *) &(*result)->object->file))))
	  goto ERROR6;
        break;
      case psiconv_texted_file:
        if ((res = psiconv_parse_texted_file(config,subbuf,lev+3,0,
                           ((psiconv_texted_f *) &(*result)->object->file))))
	  goto ERROR6;
        break;
      case psiconv_sheet_file:
        if ((res = psiconv_parse_sheet_file(config,subbuf,lev+3,0,
                           ((psiconv_sheet_f *) &(*result)->object->file))))
	  goto ERROR6;
        break;
      case psiconv_sketch_file:
        if ((res = psiconv_parse_sketch_file(config,subbuf,lev+3,0,
                           ((psiconv_sketch_f *) &(*result)->object->file))))
	  goto ERROR6;
        break;
      default:
        psiconv_warn(config,lev+3,0,"Can't parse embedded object (still continuing)");
	(*result)->object->file = NULL;
    }
  }

  psiconv_buffer_free(subbuf);
  psiconv_free_section_table_section(table);

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of Embedded Object Section "
                   "(total length: %08x)",len);

  return res;

 
ERROR6:
  free((*result)->object);
ERROR5:
  psiconv_buffer_free(subbuf);
ERROR4:
  psiconv_free_object_icon_section((*result)->icon);
ERROR3:
  psiconv_free_object_display_section((*result)->display);
ERROR2:
  psiconv_free_section_table_section(table);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading Embedded Object failed");

  if (length)
    *length = 0;

  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

psiconv_file_type_t psiconv_determine_embedded_object_type
                                       (const psiconv_config config,
					const psiconv_buffer buf,int lev,
                                        int *status)
{
  psiconv_u32 off;
  psiconv_section_table_section table;
  int res,i;
  psiconv_file_type_t file_type = psiconv_unknown_file;
  psiconv_section_table_entry entry;
  psiconv_application_id_section applid;
  
  psiconv_progress(config,lev+1,0,"Going to determine embedded object file type");
  psiconv_progress(config,lev+2,0,"Going to read the Section Table Offset Section");
  off = psiconv_read_u32(config,buf,lev,0,&res);
  if (res)
    goto ERROR1;
  psiconv_debug(config,lev+2,0,"Offset: %08x",off);

  psiconv_progress(config,lev+2,off,"Going to read the Section Table Section");
  if ((res = psiconv_parse_section_table_section(config,buf,lev+2,off,NULL,&table)))
    goto ERROR1;

  psiconv_progress(config,lev+2,off,"Going to search the Section Table Section "
                             "for the Application ID Section");
  for (i=0; i < psiconv_list_length(table); i++) {
    psiconv_progress(config,lev+3,off,"Going to read entry %d",i);
    if (!(entry = psiconv_list_get(table,i)))
      goto ERROR2;
    if (entry->id == PSICONV_ID_APPL_ID_SECTION) {
      psiconv_progress(config,lev+3,off,
	               "Found the Application ID Section at offset %08x",
                       entry->offset);
      off = entry->offset;
      break;
    }
  }
  if (i == psiconv_list_length(table)) {
    psiconv_error(config,lev+2,off,"No Application ID Section found");
    res = PSICONV_E_PARSE;
    goto ERROR2;
  }

  psiconv_progress(config,lev+2,off,"Going to read the Application ID Section");
  if ((res = psiconv_parse_application_id_section(config,buf,lev+2,off,NULL,&applid)))
    goto ERROR2;

  switch (applid->id) {
    case PSICONV_ID_WORD: file_type = psiconv_word_file; 
			  psiconv_debug(config,lev+2,off,"Found a Word file");
			  break;
    case PSICONV_ID_TEXTED: file_type = psiconv_texted_file;
                          psiconv_debug(config,lev+2,off,"Found a TextEd file");
			  break;
    case PSICONV_ID_SKETCH: file_type = psiconv_sketch_file;
			  psiconv_debug(config,lev+2,off,"Found a Sketch file");
			  break;
    case PSICONV_ID_SHEET: file_type = psiconv_sheet_file;
			  psiconv_debug(config,lev+2,off,"Found a Sheet file");
			  break;
    default: psiconv_warn(config,lev+2,off,"Found an unknown file type");
	     psiconv_debug(config,lev+2,off,"Found ID %08x",applid->id);
  }

ERROR2:
  psiconv_free_application_id_section(applid);
ERROR1:
  psiconv_free_section_table_section(table);
  if (status)
    *status = res;
  return file_type;

}

int psiconv_parse_object_display_section(const psiconv_config config,
                                        const psiconv_buffer buf,int lev,
                                        psiconv_u32 off, int *length,
                                        psiconv_object_display_section *result)
{
  int res = 0;
  int len = 0;
  int leng;
  psiconv_u32 temp;
  
  psiconv_progress(config,lev+1,off,"Going to read the Object Display Section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read the display as icon flag "
                   "(expecting 0x00 or 0x01)");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp == 0x00) {
    (*result)->show_icon = psiconv_bool_true;
    psiconv_debug(config,lev+2,off+len,"Displayed as icon");
  } else if (temp == 0x01) {
    (*result)->show_icon = psiconv_bool_false;
    psiconv_debug(config,lev+2,off+len,"Displayed as full document");
  } else {
    psiconv_warn(config,lev+2,off+len,"Unknown Object Display Section Icon Flag");
    psiconv_debug(config,lev+2,off+len,"Icon flag found: %02x",temp);
    /* Improvise */
    (*result)->show_icon = (temp & 0x01?psiconv_bool_false:psiconv_bool_true);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read the display width");
  (*result)->width = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Display width: %f cm",(*result)->width);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the display height");
  (*result)->height = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Display length: %f cm",(*result)->height);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read unknown long (%08x expected)",
                   0);
  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (temp != 0) {
    psiconv_warn(config,lev+2,off+len,"Unknown Object Display Section final long");
    psiconv_debug(config,lev+2,off+len,"Long read: %08x",temp);
  }
  len += 4;

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of Object Display Section "
                   "(total length: %08x",len);
  return res;

ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off+len,"Reading of Object Display Section failed");
  if (length)
    *length=0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_object_icon_section(const psiconv_config config,
                                      const psiconv_buffer buf,int lev,
                                      psiconv_u32 off, int *length,
                                      psiconv_object_icon_section *result)
{
  int res = 0;
  int len = 0;
  int leng;
  
  psiconv_progress(config,lev+1,off,"Going to read the Object Icon Section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read the icon name");
  (*result)->icon_name = psiconv_read_string(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the icon width");
  (*result)->icon_width = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Icon width: %f cm",(*result)->icon_width);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the icon height");
  (*result)->icon_height = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Icon length: %f cm",(*result)->icon_height);
  len += leng;

  if (length)
    *length = len;

  psiconv_progress(config,lev+1,off+len-1,"End of Object Icon Section"
                   "(total length: %08x",len);
  return res;

ERROR3:
  free((*result)->icon_name);
ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off+len,"Reading of Object Icon Section failed");
  if (length)
    *length=0;
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}
