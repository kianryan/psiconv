/*
    generate_common.c - Part of psiconv, a PSION 5 file formats converter
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

static int psiconv_write_layout_section(const psiconv_config config,
                           psiconv_buffer buf, int lev,
                           const psiconv_text_and_layout value,
                           const psiconv_word_styles_section styles,
                           int with_styles);

/* Maybe use a psiconv_header_section variable instead? */
int psiconv_write_header_section(const psiconv_config config,
                                 psiconv_buffer buf,int lev,psiconv_u32 uid1,
                                 psiconv_u32 uid2, psiconv_u32 uid3)
{
  int res;
  psiconv_progress(config,lev,0,"Writing header section");
  if ((res = psiconv_write_u32(config,buf,lev+1,uid1)))
    goto ERROR;
  if ((res = psiconv_write_u32(config,buf,lev+1,uid2)))
    goto ERROR;
  if ((res = psiconv_write_u32(config,buf,lev+1,uid3)))
    goto ERROR;
  if ((res =  psiconv_write_u32(config,buf,lev+1,
	                        psiconv_checkuid(uid1,uid2,uid3))))
    goto ERROR;
  psiconv_progress(config,lev,0,"End of header section");
  return 0;

ERROR:
  psiconv_error(config,lev,0,"Writing of header section failed");
  return res;
}

int psiconv_write_section_table_section(const psiconv_config config,
                                    psiconv_buffer buf,int lev, 
                                    const psiconv_section_table_section value)
{
  int res,i;
  psiconv_section_table_entry entry;

  psiconv_progress(config,lev,0,"Writing section table section");
  if (!value) {
    psiconv_error(config,lev,0,"Null section table section");
    res = -PSICONV_E_GENERATE;
    goto ERROR;
  }

  if ((res = psiconv_write_u8(config,buf,lev+1,2 * psiconv_list_length(value))))
    goto ERROR;
  for (i = 0; i < psiconv_list_length(value); i++) {
    if (!(entry = psiconv_list_get(value,i))) {
      psiconv_error(config,lev+1,0,"Data structure corruption");
      res = -PSICONV_E_NOMEM;
      goto ERROR;
    }
    if ((res = psiconv_write_u32(config,buf,lev+1,entry->id)))
      goto ERROR;
    if ((res = psiconv_write_offset(config,buf,lev+1,entry->offset)))
      goto ERROR;
  }
  psiconv_progress(config,lev,0,"End of section table section");
  return -PSICONV_E_OK;

ERROR:
  psiconv_error(config,lev,0,"Writing of section table section failed");
  return res;
}

int psiconv_write_application_id_section(const psiconv_config config,
                                        psiconv_buffer buf,int lev,psiconv_u32 id,
                                        const psiconv_string_t text)
{
  int res;
  psiconv_progress(config,lev,0,"Writing application id section");
  if ((res = psiconv_write_u32(config,buf,lev+1,id)))
    goto ERROR;
  if ((res = psiconv_write_string(config,buf,lev+1,text)))
    goto ERROR;
  psiconv_progress(config,lev,0,"End of application id section");
  return 0;
ERROR:
  psiconv_error(config,lev,0,"Writing of application id section failed");
  return res;
}

int psiconv_write_text_section(const psiconv_config config,
                           psiconv_buffer buf,int lev,
                           const psiconv_text_and_layout value)
{
  int res;
  psiconv_buffer extra_buf = NULL;
  int i,j;
  psiconv_paragraph paragraph;

  psiconv_progress(config,lev,0,"Writing text section");
  if (!value) {
    psiconv_error(config,lev+1,0,"Null text section");
    res = -PSICONV_E_GENERATE;
    goto ERROR;
  }

  if (psiconv_list_length(value)) {
    if (!(extra_buf = psiconv_buffer_new())) {
      psiconv_error(config,lev+1,0,"Out of memory error");
      res = -PSICONV_E_NOMEM;
      goto ERROR;
    }
    for (i = 0; i < psiconv_list_length(value); i++) {
      if (!(paragraph = psiconv_list_get(value,i))) {
        psiconv_error(config,lev+1,0,"Data structure corruption");
        res = -PSICONV_E_NOMEM;
        goto ERROR;
      }
      for (j = 0; j < psiconv_unicode_strlen(paragraph->text); j++) 
	if ((res = psiconv_unicode_write_char(config,extra_buf,lev+1,
                                              paragraph->text[j])))
          goto ERROR;
      psiconv_unicode_write_char(config,extra_buf,lev+1,0x06);
    }
    if ((res = psiconv_write_X(config,buf,lev+1,psiconv_buffer_length(extra_buf))))
      goto ERROR;
    res = psiconv_buffer_concat(buf,extra_buf);
  } else 
    /* Hack: empty text sections are just not allowed */
    if ((res = psiconv_write_u16(config,buf,lev+1,0x0602)))
      goto ERROR;

  psiconv_progress(config,lev,0,"End of text section");
  return 0;

ERROR:  
  if (extra_buf)
    psiconv_buffer_free(extra_buf);
  psiconv_error(config,lev,0,"Writing of text section failed");
  return res;
}

int psiconv_write_layout_section(const psiconv_config config,
                           psiconv_buffer buf,int lev,
                           const psiconv_text_and_layout value,
                           const psiconv_word_styles_section styles,
                           int with_styles)
{
  typedef struct psiconv_paragraph_type_list_s
  {
    psiconv_character_layout character;
    psiconv_paragraph_layout paragraph;
    psiconv_u8 style;
    psiconv_u8 nr;
  } *psiconv_paragraph_type_list;
  psiconv_u32 obj_id;
  psiconv_list paragraph_type_list; /* Of psiconv_paragraph_type_list_s */
  psiconv_paragraph_type_list paragraph_type;
  struct psiconv_paragraph_type_list_s new_type;
  psiconv_buffer buf_types,buf_elements,buf_inlines,buf_objects;
  psiconv_paragraph paragraph;
  psiconv_in_line_layout in_line = NULL;
  psiconv_word_style style;
  psiconv_character_layout para_charlayout;
  int i,j,para_type,nr_of_inlines=0,res,ptl_length,pel_length,thislen,paralen;

  psiconv_progress(config,lev,0,"Writing layout section");
  if (!value) {
    psiconv_error(config,lev,0,"Null text section");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(paragraph_type_list = psiconv_list_new(sizeof(new_type)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  if (!(buf_types = psiconv_buffer_new())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR2;
  }

  if (!(buf_elements = psiconv_buffer_new())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR3;
  }

  if (!(buf_inlines = psiconv_buffer_new())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR4;
  }

  if (!(buf_objects = psiconv_buffer_new())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR5;
  }

  for (i = 0; i < psiconv_list_length(value); i++) {
    if (!(paragraph = psiconv_list_get(value,i))) {
      psiconv_error(config,lev+1,0,"Data structure corruption");
      res = -PSICONV_E_NOMEM;
      goto ERROR6;
    }
    if ((res = psiconv_write_u32(config,buf_elements,lev+1,
	                         psiconv_unicode_strlen(paragraph->text)+1)))
      goto ERROR6;

    /* We need it for the next if-statement */
    if (psiconv_list_length(paragraph->in_lines) == 1) 
      if (!(in_line = psiconv_list_get(paragraph->in_lines,0))) {
        psiconv_error(config,lev+1,0,"Data structure corruption");
	res = -PSICONV_E_NOMEM;
	goto ERROR6;
      }

    if ((psiconv_list_length(paragraph->in_lines) > 1) ||
	((psiconv_list_length(paragraph->in_lines) == 1) &&
	 (in_line->object != NULL))) {
      /* Inline layouts, or an object, so we generate a paragraph element
         and inline elements */
      if ((res = psiconv_write_u8(config,buf_elements,lev+1,0x00)))
        goto ERROR6;
      if (!(style = psiconv_get_style(styles,paragraph->base_style))) {
        psiconv_error(config,lev+1,0,"Unknown style");
        res = -PSICONV_E_GENERATE;
        goto ERROR6;
      }
      if ((res = psiconv_write_paragraph_layout_list(config,buf_elements,lev+1,
                                                     paragraph->base_paragraph,
                                                     style->paragraph)))
        goto ERROR6;
      if (with_styles)
        if ((res = psiconv_write_u8(config,buf_elements,lev+1,paragraph->base_style)))
          goto ERROR6;
      if ((res = psiconv_write_u32(config,buf_elements,lev+1,
                                   psiconv_list_length(paragraph->in_lines))))
         goto ERROR6;

      /* Generate the inlines. NB: Against what are all settings relative?!? */
      paralen = 0;
      for (j = 0; j < psiconv_list_length(paragraph->in_lines); j++) {
        nr_of_inlines ++;
        if (!(in_line = psiconv_list_get(paragraph->in_lines,j))) {
          psiconv_error(config,lev,0,"Data structure corruption");
          res = -PSICONV_E_NOMEM;
          goto ERROR6;
        }
        if ((res = psiconv_write_u8(config,buf_inlines,lev+1,in_line->object?0x01:0x00)))
          goto ERROR6;
        thislen = in_line->length;
        paralen += thislen;
        /* If this is the last in_line, we need to make sure that the
           complete length of all inlines equals the text length */
        if (j == psiconv_list_length(paragraph->in_lines)-1) {
          if (paralen > psiconv_unicode_strlen(paragraph->text)+1) {
            psiconv_error(config,lev+1,0,"Inline formatting data length and line length are inconsistent");
            res = -PSICONV_E_GENERATE;
            goto ERROR6;
          }
          thislen += psiconv_unicode_strlen(paragraph->text)+1-paralen;
        }
        if ((res = psiconv_write_u32(config,buf_inlines,lev+1,thislen)))
          goto ERROR6;
        if ((res = psiconv_write_character_layout_list(config,buf_inlines,lev+1,
                                                     in_line->layout,
                                                     style->character)))
          goto ERROR6;
	if (in_line->object) {
	  if ((res = psiconv_write_u32(config,buf_inlines,lev+1,PSICONV_ID_OBJECT)))
	    goto ERROR6;
	  obj_id = psiconv_buffer_unique_id();
	  if ((res = psiconv_buffer_add_reference(buf_inlines,obj_id))) {
            psiconv_error(config,lev+1,0,"Out of memory error");
	    goto ERROR6;
	  }
	  if ((res = psiconv_buffer_add_target(buf_objects,obj_id))) {
            psiconv_error(config,lev+1,0,"Out of memory error"); 
	    goto ERROR6;
	  }
	  if ((res = psiconv_write_embedded_object_section(config,buf_objects,lev+1,
		                                   in_line->object)))
	    goto ERROR6;
	  if ((res = psiconv_write_length(config,buf_inlines,lev+1,in_line->object_width)))
	    goto ERROR6;
	  if ((res = psiconv_write_length(config,buf_inlines,lev+1,in_line->object_height)))
	    goto ERROR6;
	}
      } 
    } else {
      /* No inline layouts (or only 1), so we generate a paragraph type list */
      para_type = 0;
      /* Set para_charlayout to the correct character-level layout */
      if (psiconv_list_length(paragraph->in_lines) == 0)
        para_charlayout = paragraph->base_character;
      else {
        if (!(in_line = psiconv_list_get(paragraph->in_lines,0))) {
          psiconv_error(config,lev,0,"Data structure corruption");
          res = -PSICONV_E_NOMEM;
          goto ERROR6;
        } 
        para_charlayout = in_line->layout;
      }
      for (j = 0; j < psiconv_list_length(paragraph_type_list); j++) {
        if (!(paragraph_type = psiconv_list_get(paragraph_type_list,j))) {
          psiconv_error(config,lev,0,"Data structure corruption");
          res = -PSICONV_E_NOMEM;
          goto ERROR6;
        }
        if ((paragraph->base_style == paragraph_type->style) &&
            !psiconv_compare_character_layout(para_charlayout,
                                              paragraph_type->character) &&
            !psiconv_compare_paragraph_layout(paragraph->base_paragraph,
                                              paragraph_type->paragraph)) {
          para_type = paragraph_type->nr;
          break;
        }
      }
      if (!para_type) {
        /* We need to add a new entry */
        para_type = new_type.nr = j+1;
        /* No need to copy them, we won't change them anyway */
        new_type.paragraph = paragraph->base_paragraph;
        new_type.character = para_charlayout;
        new_type.style = paragraph->base_style;
        paragraph_type = &new_type;
        if ((res = psiconv_list_add(paragraph_type_list,paragraph_type))) {
          psiconv_error(config,lev+1,0,"Out of memory error"); 
          goto ERROR6;
	}
        if ((res = psiconv_write_u32(config,buf_types,lev+1,paragraph_type->nr))) 
          goto ERROR6;
        if (!(style = psiconv_get_style(styles,paragraph_type->style))) {
          psiconv_error(config,lev,0,"Unknown style");
          res = -PSICONV_E_GENERATE;
          goto ERROR6;
        }
        if ((res = psiconv_write_paragraph_layout_list(config,buf_types,lev+1,
                                paragraph_type->paragraph,style->paragraph)))
          goto ERROR6;
        if (with_styles)
          if ((res = psiconv_write_u8(config,buf_types,lev+1,paragraph_type->style)))
            goto ERROR6;
        if ((res = psiconv_write_character_layout_list(config,buf_types,lev+1,
                                paragraph_type->character,style->character)))
          goto ERROR6;
      }
      if ((res = psiconv_write_u8(config,buf_elements,lev+1,para_type)))
        goto ERROR6;
    }
  }

  /* HACK: special case: no paragraphs at all. We need to improvize. */
  if (!psiconv_list_length(value)) {
    if ((res = psiconv_write_u32(config,buf_types,lev+1,1)))
      goto ERROR6;
    if ((res = psiconv_write_u32(config,buf_types,lev+1,0)))
      goto ERROR6;
    if (with_styles)
      if ((res = psiconv_write_u8(config,buf_types,lev+1,0)))
        goto ERROR6;
    if ((res = psiconv_write_u32(config,buf_types,lev+1,0)))
      goto ERROR6;

    if ((res = psiconv_write_u32(config,buf_elements,lev+1,1)))
      goto ERROR6;
    if ((res = psiconv_write_u8(config,buf_elements,lev+1,1)))
      goto ERROR6;
    pel_length = 1;
    ptl_length = 1;
  } else  {
    pel_length = psiconv_list_length(value);
    ptl_length = psiconv_list_length(paragraph_type_list);
  }

  /* Now append everything */
  if ((res = psiconv_write_u16(config,buf,lev+1,with_styles?0x0001:0x0000)))
    goto ERROR6;
  if ((res = psiconv_write_u8(config,buf,lev+1, ptl_length)))
    goto ERROR6;
  if ((res = psiconv_buffer_concat(buf,buf_types))) {
    psiconv_error(config,lev+1,0,"Out of memory error"); 
    goto ERROR6;
  }
  if ((res = psiconv_write_u32(config,buf,lev+1,pel_length)))
    goto ERROR6;
  if ((res = psiconv_buffer_concat(buf,buf_elements))) {
    psiconv_error(config,lev+1,0,"Out of memory error"); 
    goto ERROR6;
  }
  if ((res = psiconv_write_u32(config,buf,lev+1,nr_of_inlines)))
    goto ERROR6;
  if ((res = psiconv_buffer_concat(buf,buf_inlines))) {
    psiconv_error(config,lev+1,0,"Out of memory error"); 
    goto ERROR6;
  }
  if ((res = psiconv_buffer_concat(buf,buf_objects))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR6;
  }

ERROR6:
  psiconv_buffer_free(buf_objects);
ERROR5:
  psiconv_buffer_free(buf_inlines);
ERROR4:
  psiconv_buffer_free(buf_elements);
ERROR3:
  psiconv_buffer_free(buf_types);
ERROR2:
  psiconv_list_free(paragraph_type_list);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of layout section failed");
  else
    psiconv_progress(config,lev,0,"End of layout section");
  return res;
}

int psiconv_write_styled_layout_section(const psiconv_config config,
                                    psiconv_buffer buf,int lev,
                                    psiconv_text_and_layout result,
                                    psiconv_word_styles_section styles)
{
  int res;

  psiconv_progress(config,lev,0,"Writing styled layout section");
  res = psiconv_write_layout_section(config,buf,lev+1,result,styles,1);
  if (res)
    psiconv_error(config,lev,0,"Writing of styles layout section failed");
  else
    psiconv_progress(config,lev,0,"End of styled layout section");
  return res;
}

int psiconv_write_styleless_layout_section(const psiconv_config config,
                                       psiconv_buffer buf,int lev,
                                       const psiconv_text_and_layout value,
                                       const psiconv_character_layout base_char,
                                       const psiconv_paragraph_layout base_para)
{
  int res = 0;
  psiconv_word_styles_section styles_section;

  psiconv_progress(config,lev,0,"Writing styleless layout section");
  if (!(styles_section = malloc(sizeof(*styles_section)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR1;
  }
  if (!(styles_section->normal = malloc(sizeof(*styles_section->normal)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  if (!(styles_section->normal->character =
                            psiconv_clone_character_layout(base_char))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if (!(styles_section->normal->paragraph =
                            psiconv_clone_paragraph_layout(base_para))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR4;
  }
  styles_section->normal->hotkey = 0;
  if (!(styles_section->normal->name = psiconv_unicode_empty_string())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR5;
  }
  if (!(styles_section->styles = psiconv_list_new(sizeof(
                                        struct psiconv_word_style_s)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR6;
  }

  res = psiconv_write_layout_section(config,buf,lev+1,value,styles_section,0);
  psiconv_free_word_styles_section(styles_section);
  psiconv_progress(config,lev,0,"End of styleless layout section");
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
  psiconv_error(config,lev,0,"Writing of styleless layout section failed");
  if (!res)
    return -PSICONV_E_NOMEM;
  else
    return res;
}


int psiconv_write_embedded_object_section(const psiconv_config config,
                                  psiconv_buffer buf,int lev,
                                  const psiconv_embedded_object_section value)
{
  int res;
  psiconv_u32 display_id,icon_id,table_id;
  psiconv_buffer extra_buf;

  psiconv_progress(config,lev,0,"Writing embedded object section");
  if (!value) {
    psiconv_error(config,lev,0,"Null Object");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(extra_buf = psiconv_buffer_new())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  display_id = psiconv_buffer_unique_id();
  icon_id = psiconv_buffer_unique_id();
  table_id = psiconv_buffer_unique_id();
  if ((res = psiconv_write_u8(config,buf,lev+1,0x06)))
    goto ERROR2;
  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_OBJECT_DISPLAY_SECTION)))
    goto ERROR2;
  if ((res = psiconv_buffer_add_reference(buf,display_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_OBJECT_ICON_SECTION)))
    goto ERROR2;
  if ((res = psiconv_buffer_add_reference(buf,icon_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_OBJECT_SECTION_TABLE_SECTION)))
    goto ERROR2;
  if ((res = psiconv_buffer_add_reference(buf,table_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }

  if ((res = psiconv_buffer_add_target(buf,display_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  if ((res = psiconv_write_object_display_section(config,buf,lev+1,value->display)))
    goto ERROR2;
  if ((res = psiconv_buffer_add_target(buf,icon_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  if ((res = psiconv_write_object_icon_section(config,buf,lev+1,value->icon)))
    goto ERROR2;
  if ((res = psiconv_buffer_add_target(buf,table_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  switch(value->object->type) {
    case psiconv_word_file:
      if ((res = psiconv_write_word_file(config,extra_buf,lev+1,
                                      (psiconv_word_f) value->object->file)))
	goto ERROR2;
      break;
    case psiconv_sketch_file:
      if ((res = psiconv_write_sketch_file(config,extra_buf,lev+1,
                                      (psiconv_sketch_f) value->object->file)))
	goto ERROR2;
      break;
/*
    case psiconv_sheet_file:
      if ((res = psiconv_write_sheet_file(config,extra_buf,lev+1,
                                      (psiconv_sheet_f) value->object->file)))
	goto ERROR2;
      break;
*/
    default:
      psiconv_error(config,lev,0,"Unknown or unsupported object type");
      res = -PSICONV_E_GENERATE;
      goto ERROR2;
  }
  
  if ((res = psiconv_buffer_resolve(extra_buf))) {
    psiconv_error(config,lev+1,0,"Internal error resolving buffer references");
    goto ERROR2;
  }
  if ((res = psiconv_buffer_concat(buf,extra_buf))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  psiconv_buffer_free(extra_buf);

  psiconv_progress(config,lev,0,"End of embedded object section");
  return 0;

ERROR2:
  psiconv_buffer_free(extra_buf);
ERROR1:
  psiconv_error(config,lev,0,"Writing of embedded object section failed");
  return res;
}


int psiconv_write_object_display_section(const psiconv_config config,
                                  psiconv_buffer buf,int lev,
                                  const psiconv_object_display_section value)
{
  int res;

  psiconv_progress(config,lev,0,"Writing object display section");

  if (!value) {
    psiconv_error(config,lev,0,"Null Object Display Section");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if ((res = psiconv_write_u8(config,buf,lev+1,value->show_icon?0x00:0x01)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->width)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->height)))
    goto ERROR1;
  if ((res = psiconv_write_u32(config,buf,lev+1,0x00000000)))
    goto ERROR1;

  psiconv_progress(config,lev,0,"End of object display section");

  return 0;

ERROR1:
  psiconv_error(config,lev,0,"Writing of object display section failed");
  return res;
}

int psiconv_write_object_icon_section(const psiconv_config config,
                                  psiconv_buffer buf,int lev,
                                  const psiconv_object_icon_section value)
{
  int res;

  psiconv_progress(config,lev,0,"Writing object icon section");

  if (!value) {
    psiconv_error(config,lev,0,"Null Object Icon Section");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if ((res = psiconv_write_string(config,buf,lev+1,value->icon_name)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->icon_width)))
    goto ERROR1;
  if ((res = psiconv_write_length(config,buf,lev+1,value->icon_height)))
    goto ERROR1;

  psiconv_progress(config,lev,0,"End of object icon section");
  return 0;

ERROR1:
  psiconv_error(config,lev,0,"Writing of object icon section failed");
  return res;
}
