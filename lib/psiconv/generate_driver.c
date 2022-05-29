/*
    generate_driver.c - Part of psiconv, a PSION 5 file formats converter
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

#include "error.h"
#include "generate_routines.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static psiconv_ucs2 unicode_paint[10] = { 'P','a','i','n','t','.','a','p','p',0 };
static psiconv_ucs2 unicode_texted[11] ={ 'T','e','x','t','E','d','.','a','p','p',0 };
static psiconv_ucs2 unicode_word[9] =   { 'W','o','r','d','.','a','p','p',0 };
                                                                                

int psiconv_write(const psiconv_config config, psiconv_buffer *buf,
                  const psiconv_file value)
{
  int res;
  int lev = 0;

  if (!value) {
    psiconv_error(config,0,0,"Can't parse to an empty buffer!");
    return -PSICONV_E_OTHER;
  }
  if (!(*buf = psiconv_buffer_new())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    return -PSICONV_E_NOMEM;
  }

  if (value->type == psiconv_word_file) {
    if ((res = psiconv_write_header_section(config,*buf,lev+1,PSICONV_ID_PSION5,
                                            PSICONV_ID_DATA_FILE,
                                            PSICONV_ID_WORD)))
      goto ERROR;
    if ((res =psiconv_write_word_file(config,*buf,lev+1,(psiconv_word_f) (value->file))))
      goto ERROR;
  } else if (value->type == psiconv_texted_file) {
    if ((res = psiconv_write_header_section(config,*buf,lev+1,PSICONV_ID_PSION5,
                                            PSICONV_ID_DATA_FILE,
                                            PSICONV_ID_TEXTED)))
      goto ERROR;
    if ((res =psiconv_write_texted_file(config,*buf,lev+1,
                                           (psiconv_texted_f) (value->file))))
      goto ERROR;
  } else if (value->type == psiconv_sketch_file) {
    if ((res = psiconv_write_header_section(config,*buf,lev+1,PSICONV_ID_PSION5,
                                            PSICONV_ID_DATA_FILE,
                                            PSICONV_ID_SKETCH)))
      goto ERROR;
    if ((res =psiconv_write_sketch_file(config,*buf,lev+1,
                                           (psiconv_sketch_f) (value->file))))
      goto ERROR;
  } else if (value->type == psiconv_mbm_file) {
    if ((res = psiconv_write_header_section(config,*buf,lev+1,PSICONV_ID_PSION5,
                                            PSICONV_ID_MBM_FILE,
                                            0x00000000)))
      goto ERROR;
    if ((res =psiconv_write_mbm_file(config,*buf,lev+1,
                                           (psiconv_mbm_f) (value->file))))
      goto ERROR;
  } else if (value->type == psiconv_clipart_file) {
    /* No complete header section, so we do it all in the below function */
    if ((res =psiconv_write_clipart_file(config,*buf,lev+1,
                                           (psiconv_clipart_f) (value->file))))
      goto ERROR;
  } else {
    psiconv_error(config,0,0,"Unknown or unsupported file type");
    res = -PSICONV_E_GENERATE;
    goto ERROR;
  }
  if ((res = psiconv_buffer_resolve(*buf))) {
    psiconv_error(config,lev+1,0,"Internal error resolving buffer references");
    goto ERROR;
  }
  return -PSICONV_E_OK;

ERROR:
  psiconv_buffer_free(*buf);
  return res;
}
  
int psiconv_write_texted_file(const psiconv_config config,
                              psiconv_buffer buf,int lev,psiconv_texted_f value)
{
  psiconv_character_layout base_char;
  psiconv_paragraph_layout base_para;
  int res;
  psiconv_section_table_section section_table;
  psiconv_section_table_entry entry;
  psiconv_u32 section_table_id;
  psiconv_buffer buf_texted;

  psiconv_progress(config,lev,0,"Writing texted file");
  if (!value) {
    psiconv_error(config,lev,0,"Null TextEd file");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(section_table = psiconv_list_new(sizeof(*entry)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  if (!(entry = malloc(sizeof(*entry)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR2;
  }
    
  if (!(base_char = psiconv_basic_character_layout())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR3;
  }
  if (!(base_para = psiconv_basic_paragraph_layout())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR4;
  }

  section_table_id = psiconv_buffer_unique_id();
  if ((res = psiconv_write_offset(config,buf,lev+1,section_table_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR5;
  }

  entry->id = PSICONV_ID_APPL_ID_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR5;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR5;
  }
  if ((res=psiconv_write_application_id_section(config,buf,lev+1,
                                           PSICONV_ID_TEXTED,unicode_texted)))
    goto ERROR5;
  
  entry->id = PSICONV_ID_PAGE_LAYOUT_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR5;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR5;
  }
  if ((res = psiconv_write_page_layout_section(config,buf,lev+1,value->page_sec)))
    goto ERROR5;

  entry->id = PSICONV_ID_TEXTED;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR5;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR5;
  }
  if ((res = psiconv_write_texted_section(config,buf,lev+1,value->texted_sec,
                                           base_char,base_para,&buf_texted)))
    goto ERROR5;

  if ((res = psiconv_buffer_concat(buf,buf_texted))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR6;
  }

  if ((res = psiconv_buffer_add_target(buf,section_table_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR6;
  }

  res = psiconv_write_section_table_section(config,buf,lev+1,section_table);
  
ERROR6:
  psiconv_buffer_free(buf_texted);
ERROR5:
  psiconv_free_paragraph_layout(base_para);
ERROR4:
  psiconv_free_character_layout(base_char);
ERROR3:
  free(entry);
ERROR2:
  psiconv_list_free(section_table);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of texted file failed");
  else
    psiconv_progress(config,lev,0,"End of texted file");
  return res;
}

int psiconv_write_word_file(const psiconv_config config,
                            psiconv_buffer buf,int lev,psiconv_word_f value)
{
  int res;
  psiconv_section_table_section section_table;
  psiconv_section_table_entry entry;
  psiconv_u32 section_table_id;

  psiconv_progress(config,lev,0,"Writing word file");
  if (!value) {
    psiconv_error(config,lev,0,"Null Word file");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(section_table = psiconv_list_new(sizeof(*entry)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  if (!(entry = malloc(sizeof(*entry)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR2;
  }
    
  section_table_id = psiconv_buffer_unique_id();
  if ((res = psiconv_write_offset(config,buf,lev+1,section_table_id))) 
    goto ERROR3;

  entry->id = PSICONV_ID_APPL_ID_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res=psiconv_write_application_id_section(config,buf,lev+1,
                                           PSICONV_ID_WORD,unicode_word)))
    goto ERROR3;
  
  entry->id = PSICONV_ID_WORD_STATUS_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_write_word_status_section(config,buf,lev+1,value->status_sec)))
    goto ERROR3;

  entry->id = PSICONV_ID_PAGE_LAYOUT_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_write_page_layout_section(config,buf,lev+1,value->page_sec)))
    goto ERROR3;

  entry->id = PSICONV_ID_WORD_STYLES_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_write_word_styles_section(config,buf,lev+1,value->styles_sec)))
    goto ERROR3;

  entry->id = PSICONV_ID_TEXT_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_write_text_section(config,buf,lev+1,value->paragraphs)))
    goto ERROR3;

  entry->id = PSICONV_ID_LAYOUT_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_write_styled_layout_section(config,buf,lev+1,value->paragraphs,
                                                 value->styles_sec)))
    goto ERROR3;

  if ((res = psiconv_buffer_add_target(buf,section_table_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }

  res = psiconv_write_section_table_section(config,buf,lev+1,section_table);
  
ERROR3:
  free(entry);
ERROR2:
  psiconv_list_free(section_table);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of word file failed");
  else
    psiconv_progress(config,lev,0,"End of word file");
  return res;
}

int psiconv_write_sketch_file(const psiconv_config config,
                              psiconv_buffer buf,int lev,psiconv_sketch_f value)
{
  int res;
  psiconv_section_table_section section_table;
  psiconv_section_table_entry entry;
  psiconv_u32 section_table_id;

  psiconv_progress(config,lev,0,"Writing sketch file");
  if (!value) {
    psiconv_error(config,lev,0,"Null Sketch file");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(section_table = psiconv_list_new(sizeof(*entry)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  if (!(entry = malloc(sizeof(*entry)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR2;
  }
    
  section_table_id = psiconv_buffer_unique_id();
  if ((res = psiconv_write_offset(config,buf,lev+1,section_table_id)))
    goto ERROR3;

  entry->id = PSICONV_ID_APPL_ID_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res=psiconv_write_application_id_section(config,buf,lev+1,
                                           PSICONV_ID_SKETCH,unicode_paint)))
    goto ERROR3;
  
  entry->id = PSICONV_ID_SKETCH_SECTION;
  entry->offset = psiconv_buffer_unique_id();
  if ((res = psiconv_list_add(section_table,entry))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_buffer_add_target(buf,entry->offset))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  if ((res = psiconv_write_sketch_section(config,buf,lev+1,value->sketch_sec)))
    goto ERROR3;

  if ((res = psiconv_buffer_add_target(buf,section_table_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
  res = psiconv_write_section_table_section(config,buf,lev+1,section_table);
  
ERROR3:
  free(entry);
ERROR2:
  psiconv_list_free(section_table);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of sketch file failed");
  else
    psiconv_progress(config,lev,0,"End of sketch file");
  return res;
}

int psiconv_write_mbm_file(const psiconv_config config,
                           psiconv_buffer buf,int lev,psiconv_mbm_f value)
{
  int res,i;
  psiconv_jumptable_section jumptable;
  psiconv_u32 *entry,id,table_id;
  psiconv_paint_data_section section;

  psiconv_progress(config,lev,0,"Writing mbm file");
  if (!value) {
    psiconv_error(config,lev,0,"Null MBM file");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(jumptable = psiconv_list_new(sizeof(*entry)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  table_id = psiconv_buffer_unique_id();
  if ((res = psiconv_buffer_add_reference(buf,table_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  
  for (i = 0; i < psiconv_list_length(value->sections); i++) {
    if (!(section = psiconv_list_get(value->sections,i))) {
      psiconv_error(config,lev,0,"Data structure corruption");
      res = -PSICONV_E_NOMEM;
      goto ERROR2;
    }
    id = psiconv_buffer_unique_id();
    if ((res = psiconv_list_add(jumptable,&id))) {
      psiconv_error(config,lev+1,0,"Out of memory error");
      goto ERROR2;
     }
    if ((res = psiconv_buffer_add_target(buf,id))) {
      psiconv_error(config,lev+1,0,"Out of memory error");
      goto ERROR2;
    }
    if ((res = psiconv_write_paint_data_section(config,buf,lev+1,section,0)))
      goto ERROR2;
  }

  if ((res = psiconv_buffer_add_target(buf,table_id))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR2;
  }
  if ((res = psiconv_write_jumptable_section(config,buf,lev+1,jumptable)))
    goto ERROR2;
    
  
ERROR2:
  psiconv_list_free(jumptable);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of mbm file failed");
  else
    psiconv_progress(config,lev,0,"End of mbm file");
  return res;
}

/* Note: this file is special, because it does not have a complete header! */
int psiconv_write_clipart_file(const psiconv_config config,
                           psiconv_buffer buf,int lev,psiconv_clipart_f value)
{
  int res,i;
  psiconv_jumptable_section jumptable;
  psiconv_u32 *entry,id;
  psiconv_clipart_section section;
  psiconv_buffer sec_buf;

  psiconv_progress(config,lev,0,"Writing clipart file");
  if (!value) {
    psiconv_error(config,lev,0,"Null Clipart file");
    res = -PSICONV_E_GENERATE;
    goto ERROR1;
  }

  if (!(jumptable = psiconv_list_new(sizeof(*entry)))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR1;
  }

  if (!(sec_buf = psiconv_buffer_new())) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    res = -PSICONV_E_NOMEM;
    goto ERROR2;
  }

  if ((res = psiconv_write_u32(config,buf,lev+1,PSICONV_ID_CLIPART)))
    goto ERROR3;

  for (i = 0; i < psiconv_list_length(value->sections); i++) {
    if (!(section = psiconv_list_get(value->sections,i))) {
      psiconv_error(config,lev,0,"Data structure corruption");
      res = -PSICONV_E_NOMEM;
      goto ERROR3;
    }
    id = psiconv_buffer_unique_id();
    if ((res = psiconv_list_add(jumptable,&id))) {
      psiconv_error(config,lev+1,0,"Out of memory error");
      goto ERROR3;
    }
    if ((res = psiconv_buffer_add_target(sec_buf,id))) {
      psiconv_error(config,lev+1,0,"Out of memory error");
      goto ERROR3;
    }
    if ((res = psiconv_write_clipart_section(config,sec_buf, lev+1,section)))
      goto ERROR3;
  }

  if ((res = psiconv_write_jumptable_section(config,buf,lev+1,jumptable)))
    goto ERROR3;

  if ((res = psiconv_buffer_concat(buf,sec_buf))) {
    psiconv_error(config,lev+1,0,"Out of memory error");
    goto ERROR3;
  }
    
  
ERROR3:
  psiconv_buffer_free(sec_buf);
ERROR2:
  psiconv_list_free(jumptable);
ERROR1:
  if (res)
    psiconv_error(config,lev,0,"Writing of clipart file failed");
  else
    psiconv_progress(config,lev,0,"End of clipart file");
  return res;
}
