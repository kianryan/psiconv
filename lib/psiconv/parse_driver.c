/*
    parse_driver.c - Part of psiconv, a PSION 5 file formats converter
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

#include "parse.h"
#include "parse_routines.h"
#include "unicode.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

/* Compare whether application id names match.
   Sought must be lower case; the comparison is case insensitive */
static psiconv_bool_t applid_matches(psiconv_string_t found, 
                                     const char *sought)
{
  int i;
  if (psiconv_unicode_strlen(found) != strlen(sought))
    return psiconv_bool_false;
  for (i = 0; i < strlen(sought); i++) 
    if ((found[i] != sought[i]) &&
	((sought[i] < 'a') || (sought[i] > 'z') || 
	 (found[i] != sought[i] + 'A' - 'a')))
      return psiconv_bool_false;
  return psiconv_bool_true;
}

psiconv_file_type_t psiconv_file_type(const psiconv_config config,
                                      psiconv_buffer buf,int *length,
                                      psiconv_header_section *result)
{
  psiconv_header_section header;
  psiconv_file_type_t res;
  int leng;

  if ((psiconv_parse_header_section(config,buf,0,0,&leng,&header)))
    return psiconv_unknown_file;
  res = header->file;
  if (result)
    *result = header;
  else
    psiconv_free_header_section(header);
  if (length)
    *length = leng;
  return res;
}

int psiconv_parse(const psiconv_config config,const psiconv_buffer buf,
                  psiconv_file *result)
{
  int res=0;
  int lev=0;
  int off=0;
  int leng;

  if (!((*result) = malloc(sizeof(**result))))
    goto ERROR1;

  (*result)->type = psiconv_file_type(config,buf,&leng,NULL);
  if ((*result)->type == psiconv_unknown_file) {
    psiconv_warn(config,lev+1,off,"Unknown file type: can't parse!");
    (*result)->file = NULL;
  } else if ((*result)->type == psiconv_word_file)
    res = psiconv_parse_word_file(config,buf,lev+2,leng,
                                  (psiconv_word_f *)(&((*result)->file)));
  else if ((*result)->type == psiconv_texted_file)
    res = psiconv_parse_texted_file(config,buf,lev+2,leng,
                                  (psiconv_texted_f *)(&((*result)->file)));
  else if ((*result)->type == psiconv_mbm_file)
    res = psiconv_parse_mbm_file(config,buf,lev+2,leng,
                                  (psiconv_mbm_f *)(&((*result)->file)));
  else if ((*result)->type == psiconv_sketch_file)
    res = psiconv_parse_sketch_file(config,buf,lev+2,leng,
                                  (psiconv_sketch_f *)(&((*result)->file)));
  else if ((*result)->type == psiconv_clipart_file)
    res = psiconv_parse_clipart_file(config,buf,lev+2,leng,
                                  (psiconv_clipart_f *)(&((*result)->file)));
  else if ((*result)->type == psiconv_sheet_file)
    res = psiconv_parse_sheet_file(config,buf,lev+2,leng,
                                  (psiconv_sheet_f *)(&((*result)->file)));
  else {
    psiconv_warn(config,lev+1,off,"Can't parse this file yet!");
    (*result)->file = NULL;
  }
  if (res)
    goto ERROR2;
  return 0;

ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Psion File failed");
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_clipart_file(const psiconv_config config,
                               const psiconv_buffer buf,int lev, 
                               psiconv_u32 off, psiconv_clipart_f *result)
{
  int res=0;
  int i;
  psiconv_jumptable_section table;
  psiconv_clipart_section clipart;
  psiconv_u32 *entry;

  psiconv_progress(config,lev+1,off,"Going to read a clipart file");
  if (!((*result) = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off,"Going to read the MBM jumptable");
  if ((res =  psiconv_parse_jumptable_section(config,buf,lev+2,off, NULL,&table)))
    goto ERROR2;

  psiconv_progress(config,lev+2,off,"Going to read the clipart sections");
  if (!((*result)->sections = psiconv_list_new(sizeof(*clipart))))
    goto ERROR3;
  for (i = 0; i < psiconv_list_length(table); i ++) {
    if (!(entry = psiconv_list_get(table,i)))
      goto ERROR4;
    psiconv_progress(config,lev+3,off,"Going to read clipart section %i",i);
    if ((res = psiconv_parse_clipart_section(config,buf,lev+3,*entry,NULL,&clipart)))
      goto ERROR4;
    if ((res = psiconv_list_add((*result)->sections,clipart)))
      goto ERROR5;
    free(clipart);
  }

  psiconv_free_jumptable_section(table);
  psiconv_progress(config,lev+1,off,"End of clipart file");
  return res;
ERROR5:
  psiconv_free_clipart_section(clipart);
ERROR4:
  for (i = 0; i < psiconv_list_length((*result)->sections); i++) {
    if (!(clipart = psiconv_list_get((*result)->sections,i))) {
      psiconv_error(config,lev+1,off,"Data structure corruption");
      goto ERROR3;
    }
    psiconv_free_clipart_section(clipart);
  }
  psiconv_list_free((*result)->sections);
ERROR3:
  psiconv_free_jumptable_section(table);
ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Clipart File failed");
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_mbm_file(const psiconv_config config,
                           const psiconv_buffer buf,int lev, psiconv_u32 off,
                           psiconv_mbm_f *result)
{
  int res=0;
  int i;
  psiconv_jumptable_section table;
  psiconv_paint_data_section paint;
  psiconv_u32 *entry;
  psiconv_u32 sto;

  psiconv_progress(config,lev+1,off,"Going to read a mbm file");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off,"Going to read the offset of the MBM jumptable");
  sto = psiconv_read_u32(config,buf,lev+2,off,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off,"Offset: %08x",sto);

  psiconv_progress(config,lev+2,off,"Going to read the MBM jumptable");
  if ((res =  psiconv_parse_jumptable_section(config,buf,lev+2,sto, NULL,&table)))
    goto ERROR2;

  psiconv_progress(config,lev+2,off,"Going to read the picture sections");
  if (!((*result)->sections = psiconv_list_new(sizeof(*paint))))
    goto ERROR3;
  for (i = 0; i < psiconv_list_length(table); i ++) {
    if (!(entry = psiconv_list_get(table,i)))
      goto ERROR4;
    psiconv_progress(config,lev+3,off,"Going to read picture section %i",i);
    if ((res = psiconv_parse_paint_data_section(config,buf,lev+3,*entry,NULL,
         0,&paint)))
      goto ERROR4;
    if ((res = psiconv_list_add((*result)->sections,paint)))
      goto ERROR5;
    free(paint);
  }

  psiconv_free_jumptable_section(table);
  psiconv_progress(config,lev+1,off,"End of mbm file");
  return 0;
ERROR5:
  psiconv_free_paint_data_section(paint);
ERROR4:
  for (i = 0; i < psiconv_list_length((*result)->sections); i++) {
    if (!(paint = psiconv_list_get((*result)->sections,i))) {
      psiconv_error(config,lev+1,off,"Data structure corruption");
      goto ERROR3;
    }
    psiconv_free_paint_data_section(paint);
  }
  psiconv_list_free((*result)->sections);
ERROR3:
  psiconv_free_jumptable_section(table);
ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of MBM File failed");
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_sketch_file(const psiconv_config config,
                              const psiconv_buffer buf,int lev,
                              psiconv_u32 off,
                              psiconv_sketch_f *result)
{
  psiconv_section_table_section table;
  psiconv_application_id_section appl_id;
  psiconv_u32 applid_sec = 0;
  psiconv_u32 sketch_sec = 0;
  psiconv_u32 sto;
  psiconv_section_table_entry entry;
  int i;
  int res=0;
  char *temp_str;

  psiconv_progress(config,lev+1,off,"Going to read a sketch file");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off,
                   "Going to read the offset of the section table section");
  sto = psiconv_read_u32(config,buf,lev+2,off,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off,"Offset: %08x",sto);

  psiconv_progress(config,lev+2,sto, "Going to read the section table section");
  if ((res = psiconv_parse_section_table_section(config,buf,lev+2,sto, NULL,&table)))
    goto ERROR2;

  for (i = 0; i < psiconv_list_length(table); i ++) {
    psiconv_progress(config,lev+2,sto, "Going to read entry %d",i);
    if (!(entry = psiconv_list_get(table,i)))
      goto ERROR3;
    if (entry->id == PSICONV_ID_APPL_ID_SECTION) {
      applid_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Application ID section at %08x",applid_sec);
    } else if (entry->id == PSICONV_ID_SKETCH_SECTION) {
      sketch_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Sketch section at %08x",sketch_sec);
    } else {
      psiconv_warn(config,lev+3,sto,
                   "Found unknown section in the Section Table (ignoring)");
      psiconv_debug(config,lev+3,sto,
                    "Section ID %08x, offset %08x",entry->id,entry->offset);
    }
  }

  psiconv_progress(config,lev+2,sto, "Looking for the Application ID section");
  if (! applid_sec) {
    psiconv_error(config,lev+2,sto,
                 "Application ID section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR3;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Application ID section at offset %08x",applid_sec);
    if ((res = psiconv_parse_application_id_section(config,buf,lev+2,applid_sec,NULL,
                                                &appl_id)))
      goto ERROR3;
  }
  if ((appl_id->id != PSICONV_ID_SKETCH) ||
       !applid_matches(appl_id->name,"paint.app")) {
    psiconv_warn(config,lev+2,applid_sec,
                 "Application ID section contains unexpected data");
    psiconv_debug(config,lev+2,applid_sec,"ID: %08x expected, %08x found",
                  PSICONV_ID_SKETCH,appl_id->id);
    if (!(temp_str = psiconv_make_printable(config,appl_id->name)))
      goto ERROR4;
    psiconv_debug(config,lev+2,applid_sec,"Name: `%s' expected, `%s' found",
                            "Paint.app",temp_str);
    free(temp_str);
    res = -PSICONV_E_PARSE;
    goto ERROR4;
  }

  psiconv_progress(config,lev+2,sto, "Looking for the Sketch section");
  if (! sketch_sec) {
   psiconv_warn(config,lev+2,sto,
                "Sketch section not found in the section table");
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Sketch section at offset %08x",applid_sec);
    if ((res = psiconv_parse_sketch_section(config,buf,lev+2,sketch_sec,NULL,
                                        &(*result)->sketch_sec)))
      goto ERROR4;
  }

  psiconv_free_application_id_section(appl_id);
  psiconv_free_section_table_section(table);

  psiconv_progress(config,lev+1,off,"End of sketch file");
  return res;

ERROR4:
  psiconv_free_application_id_section(appl_id);
ERROR3:
  free(table);
ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sketch File failed");
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}


int psiconv_parse_texted_file(const psiconv_config config,
                              const psiconv_buffer buf,int lev, 
                              psiconv_u32 off,
                              psiconv_texted_f *result)
{
  int res=0;
  psiconv_section_table_section table;
  psiconv_application_id_section appl_id;
  char *temp_str;
  psiconv_character_layout base_char;
  psiconv_paragraph_layout base_para;
  psiconv_u32 page_sec = 0;
  psiconv_u32 texted_sec = 0;
  psiconv_u32 applid_sec = 0;
  psiconv_u32 sto;
  psiconv_section_table_entry entry;
  int i;

  psiconv_progress(config,lev+1,off,"Going to read a texted file");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off,
                   "Going to read the offset of the section table section");
  sto = psiconv_read_u32(config,buf,lev+2,off,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off,"Offset: %08x",sto);

  psiconv_progress(config,lev+2,sto, "Going to read the section table section");
  if ((res = psiconv_parse_section_table_section(config,buf,lev+2,sto, NULL,&table)))
    goto ERROR2;

  for (i = 0; i < psiconv_list_length(table); i ++) {
    psiconv_progress(config,lev+2,sto, "Going to read entry %d",i);
    if (!(entry = psiconv_list_get(table,i)))
      goto ERROR3;
    if (entry->id == PSICONV_ID_APPL_ID_SECTION) {
      applid_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Application ID section at %08x",applid_sec);
    } else if (entry->id == PSICONV_ID_PAGE_LAYOUT_SECTION) {
      page_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Page Layout section at %08x",page_sec);
    } else if (entry->id == PSICONV_ID_TEXTED) {
      texted_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the TextEd section at %08x",texted_sec);
    } else {
      psiconv_warn(config,lev+3,sto,
                   "Found unknown section in the Section Table (ignoring)");
      psiconv_debug(config,lev+3,sto,
                    "Section ID %08x, offset %08x",entry->id,entry->offset);
    }
  }
      
  psiconv_progress(config,lev+2,sto, "Looking for the Application ID section");
  if (! applid_sec) {
   psiconv_error(config,lev+2,sto,
                "Application ID section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR3;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Application ID section at offset %08x",applid_sec);
    if ((res = psiconv_parse_application_id_section(config,buf,lev+2,applid_sec,NULL,
                                                &appl_id)))
      goto ERROR3;
  }

  if ((appl_id->id != PSICONV_ID_TEXTED) ||
       !applid_matches(appl_id->name,"texted.app")) {
    psiconv_warn(config,lev+2,applid_sec,
                 "Application ID section contains unexpected data");
    psiconv_debug(config,lev+2,applid_sec,"ID: %08x expected, %08x found",
                  PSICONV_ID_TEXTED,appl_id->id);
    if (!(temp_str = psiconv_make_printable(config,appl_id->name)))
      goto ERROR4;
    psiconv_debug(config,lev+2,applid_sec,"Name: `%s' expected, `%s' found",
                            "TextEd.app",temp_str);
    free(temp_str);
    res = -PSICONV_E_PARSE;
    goto ERROR4;
  }

  psiconv_progress(config,lev+2,sto,
                   "Looking for the Page layout section");
  if (! page_sec) {
   psiconv_error(config,lev+2,sto,
                "Page layout section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR4;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Page layout section at offset %08x",page_sec);
    if ((res = psiconv_parse_page_layout_section(config,buf,lev+2,page_sec,NULL,
                                             &(*result)->page_sec)))
      goto ERROR4;
  }

  if (!(base_char = psiconv_basic_character_layout()))
    goto ERROR5;
  if (!(base_para = psiconv_basic_paragraph_layout()))
    goto ERROR6;
  
  psiconv_progress(config,lev+2,sto,
                   "Looking for the TextEd section");
  if (! texted_sec) {
   psiconv_error(config,lev+2,sto,
                "TextEd section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR7;
  } else {
    psiconv_debug(config,lev+2,sto, "TextEd section at offset %08x",texted_sec);
    if ((res = psiconv_parse_texted_section(config,buf,lev+2,texted_sec,NULL,
                                        &(*result)->texted_sec,
                                        base_char,base_para)))
      goto ERROR7;
  }
  psiconv_free_character_layout(base_char);
  psiconv_free_paragraph_layout(base_para);
  
  psiconv_free_application_id_section(appl_id);
  psiconv_free_section_table_section(table);

  psiconv_progress(config,lev+1,off,"End of TextEd file");
  return 0;

ERROR7:
  psiconv_free_paragraph_layout(base_para);
ERROR6:
  psiconv_free_character_layout(base_char);
ERROR5:
  psiconv_free_page_layout_section((*result)->page_sec);
ERROR4:
  psiconv_free_application_id_section(appl_id);
ERROR3:
  psiconv_free_section_table_section(table);
ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of TextEd File failed");
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_word_file(const psiconv_config config,
                            const psiconv_buffer buf,int lev, psiconv_u32 off,
                            psiconv_word_f *result)
{
  int res=0;
  psiconv_section_table_section table;
  psiconv_application_id_section appl_id;
  char *temp_str;
  psiconv_u32 pwd_sec = 0;
  psiconv_u32 status_sec = 0;
  psiconv_u32 styles_sec = 0;
  psiconv_u32 page_sec = 0;
  psiconv_u32 text_sec = 0;
  psiconv_u32 layout_sec = 0;
  psiconv_u32 applid_sec = 0;
  psiconv_section_table_entry entry;
  psiconv_u32 sto;
  int i;

  psiconv_progress(config,lev+1,off,"Going to read a word file");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off,
                   "Going to read the offset of the section table section");
  sto = psiconv_read_u32(config,buf,lev+2,off,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off,"Offset: %08x",sto);

  psiconv_progress(config,lev+2,sto,
                   "Going to read the section table section");
  if ((res = psiconv_parse_section_table_section(config,buf,lev+2,sto, NULL,&table)))
    goto ERROR2;

  for (i = 0; i < psiconv_list_length(table); i ++) {
    psiconv_progress(config,lev+2,sto, "Going to read entry %d",i);
    if (!(entry = psiconv_list_get(table,i)))
      goto ERROR3;
    if (entry->id == PSICONV_ID_APPL_ID_SECTION) {
      applid_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Application ID section at %08x",applid_sec);
    } else if (entry->id == PSICONV_ID_PAGE_LAYOUT_SECTION) {
      page_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Page Layout section at %08x",page_sec);
    } else if (entry->id == PSICONV_ID_TEXT_SECTION) {
      text_sec = entry->offset;
      psiconv_debug(config,lev+3,sto, "Found the Text section at %08x",text_sec);
    } else if (entry->id == PSICONV_ID_PASSWORD_SECTION) {
      pwd_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Password section at %08x",pwd_sec);
      psiconv_error(config,lev+3,sto,
                   "Password section found - can't read encrypted data");
      res = -PSICONV_E_PARSE;
      goto ERROR3;
    } else if (entry->id == PSICONV_ID_WORD_STATUS_SECTION) {
      status_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Word Status section at %08x",status_sec);
    } else if (entry->id == PSICONV_ID_WORD_STYLES_SECTION) {
      styles_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Word Styles section at %08x",styles_sec);
    } else if (entry->id == PSICONV_ID_LAYOUT_SECTION) {
      layout_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Layout section at %08x",layout_sec);
    } else {
      psiconv_warn(config,lev+3,sto,
                   "Found unknown section in the Section Table (ignoring)");
      psiconv_debug(config,lev+3,sto,
                    "Section ID %08x, offset %08x",entry->id,entry->offset);
    }
  }
      

  psiconv_progress(config,lev+2,sto,
                   "Looking for the Status section");
  if (!status_sec) {
   psiconv_error(config,lev+2,sto, "Status section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR3;
  } else {
    psiconv_debug(config,lev+2,sto, "Status section at offset %08x",status_sec);
    if ((res = psiconv_parse_word_status_section(config,buf,lev+2,status_sec,NULL,
                                             &((*result)->status_sec))))
      goto ERROR3;
  }

  psiconv_progress(config,lev+2,sto, "Looking for the Application ID section");
  if (! applid_sec) {
   psiconv_error(config,lev+2,sto,
                "Application ID section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR4;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Application ID section at offset %08x",applid_sec);
    if ((res = psiconv_parse_application_id_section(config,buf,lev+2,applid_sec,NULL,
                                                &appl_id)))
      goto ERROR4;
  }
  if ((appl_id->id != PSICONV_ID_WORD) ||
       !applid_matches(appl_id->name,"word.app")) {
    psiconv_warn(config,lev+2,applid_sec,
                 "Application ID section contains unexpected data");
    psiconv_debug(config,lev+2,applid_sec,"ID: %08x expected, %08x found",
                  PSICONV_ID_WORD,appl_id->id);
    if (!(temp_str = psiconv_make_printable(config,appl_id->name)))
      goto ERROR5;
    psiconv_debug(config,lev+2,applid_sec,"Name: `%s' expected, `%s' found",
                            "Word.app",temp_str);
    free(temp_str);
    res = -PSICONV_E_PARSE;
    goto ERROR5;
  }

  psiconv_progress(config,lev+2,sto,
                   "Looking for the Page layout section");
  if (! page_sec) {
   psiconv_error(config,lev+2,sto,
                "Page layout section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR5;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Page layout section at offset %08x",page_sec);
    if ((res = psiconv_parse_page_layout_section(config,buf,lev+2,page_sec,NULL,
                                             &(*result)->page_sec)))
      goto ERROR5;
  }

  psiconv_progress(config,lev+2,sto,
                   "Looking for the Word Style section");
  if (!styles_sec) {
   psiconv_error(config,lev+2,sto,
                "Word styles section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR6;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Word styles section at offset %08x",styles_sec);
    if ((res = psiconv_parse_word_styles_section(config,buf,lev+2,styles_sec,NULL,
                                             &(*result)->styles_sec)))
      goto ERROR6;
  }
  
  psiconv_progress(config,lev+2,sto,
                   "Looking for the Text section");
  if (!text_sec) {
   psiconv_error(config,lev+2,sto, "Text section not found in the section table");
   res = -PSICONV_E_PARSE;
   goto ERROR7;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Text section at offset %08x",text_sec);
    if ((res = psiconv_parse_text_section(config,buf,lev+2,text_sec,NULL,
                                      &(*result)->paragraphs)))
      goto ERROR7;
  }

  psiconv_progress(config,lev+2,sto, "Looking for the Layout section");
  if (!layout_sec) {
    psiconv_debug(config,lev+2,sto, "No layout section today");
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Layout section at offset %08x",layout_sec);
    if ((res = psiconv_parse_styled_layout_section(config,buf,lev+2,layout_sec,NULL,
                                               (*result)->paragraphs,
                                               (*result)->styles_sec)))
      goto ERROR8;
  }

  psiconv_free_application_id_section(appl_id);
  psiconv_free_section_table_section(table);

  psiconv_progress(config,lev+1,off,"End of word file");
  return 0;


ERROR8:
  psiconv_free_text_and_layout((*result)->paragraphs);
ERROR7:
  psiconv_free_word_styles_section((*result)->styles_sec);
ERROR6:
  psiconv_free_page_layout_section((*result)->page_sec);
ERROR5:
  psiconv_free_application_id_section(appl_id);
ERROR4:
  psiconv_free_word_status_section((*result)->status_sec);
ERROR3:
  psiconv_free_section_table_section(table);
ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Word File failed");
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}

int psiconv_parse_sheet_file(const psiconv_config config,
                             const psiconv_buffer buf,int lev, psiconv_u32 off,
                             psiconv_sheet_f *result)
{
  int res=0;
  psiconv_section_table_section table;
  psiconv_application_id_section appl_id;
  char *temp_str;
  psiconv_u32 pwd_sec = 0;
  psiconv_u32 status_sec = 0;
  psiconv_u32 page_sec = 0;
  psiconv_u32 applid_sec = 0;
  psiconv_u32 workbook_sec = 0;
  psiconv_section_table_entry entry;
  psiconv_u32 sto;
  int i;

  psiconv_progress(config,lev+1,off,"Going to read a sheet file");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off,
                   "Going to read the offset of the section table section");
  sto = psiconv_read_u32(config,buf,lev+2,off,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off,"Offset: %08x",sto);

  psiconv_progress(config,lev+2,sto,
                   "Going to read the section table section");
  if ((res = psiconv_parse_section_table_section(config,buf,lev+2,sto, NULL,&table)))
    goto ERROR2;

  for (i = 0; i < psiconv_list_length(table); i ++) {
    psiconv_progress(config,lev+2,sto, "Going to read entry %d",i);
    if (!(entry = psiconv_list_get(table,i)))
      goto ERROR3;
    if (entry->id == PSICONV_ID_APPL_ID_SECTION) {
      applid_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Application ID section at %08x",applid_sec);
    } else if (entry->id == PSICONV_ID_PAGE_LAYOUT_SECTION) {
      page_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Page Layout section at %08x",page_sec);
    } else if (entry->id == PSICONV_ID_PASSWORD_SECTION) {
      pwd_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Password section at %08x",pwd_sec);
      psiconv_error(config,lev+3,sto,
                   "Password section found - can't read encrypted data");
      res = -PSICONV_E_PARSE;
      goto ERROR3;
    } else if (entry->id == PSICONV_ID_SHEET_WORKBOOK_SECTION) {
      workbook_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Sheet Workbook section at %08x",workbook_sec);
    } else if (entry->id == PSICONV_ID_SHEET_STATUS_SECTION) {
      status_sec = entry->offset;
      psiconv_debug(config,lev+3,sto,
                    "Found the Sheet Status section at %08x",status_sec);
    } else {
      psiconv_warn(config,lev+3,sto,
                   "Found unknown section in the Section Table (ignoring)");
      psiconv_debug(config,lev+3,sto,
                    "Section ID %08x, offset %08x",entry->id,entry->offset);
    }
  }
      

  psiconv_progress(config,lev+2,sto,
                   "Looking for the Status section");
  if (!status_sec) {
   psiconv_error(config,lev+2,sto, "Status section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR3;
  } else {
    psiconv_debug(config,lev+2,sto, "Status section at offset %08x",status_sec);
    if ((res = psiconv_parse_sheet_status_section(config,buf,lev+2,status_sec,NULL,
                                                  &((*result)->status_sec))))
      goto ERROR3;
  }

  psiconv_progress(config,lev+2,sto, "Looking for the Application ID section");
  if (! applid_sec) {
   psiconv_error(config,lev+2,sto,
                "Application ID section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR4;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Application ID section at offset %08x",applid_sec);
    if ((res = psiconv_parse_application_id_section(config,buf,lev+2,applid_sec,NULL,
                                                &appl_id)))
      goto ERROR4;
  }
  if ((appl_id->id != PSICONV_ID_SHEET) ||
       !applid_matches(appl_id->name,"sheet.app")) {
    psiconv_warn(config,lev+2,applid_sec,
                 "Application ID section contains unexpected data");
    psiconv_debug(config,lev+2,applid_sec,"ID: %08x expected, %08x found",
                  PSICONV_ID_SHEET,appl_id->id);
    if (!(temp_str = psiconv_make_printable(config,appl_id->name)))
      goto ERROR5;
    psiconv_debug(config,lev+2,applid_sec,"Name: `%s' expected, `%s' found",
                            "Sheet.app",temp_str);
    free(temp_str);
    res = -PSICONV_E_PARSE;
    goto ERROR5;
  }

  psiconv_progress(config,lev+2,sto,
                   "Looking for the Page layout section");
  if (! page_sec) {
   psiconv_error(config,lev+2,sto,
                "Page layout section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR5;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Page layout section at offset %08x",page_sec);
    if ((res = psiconv_parse_page_layout_section(config,buf,lev+2,page_sec,NULL,
                                             &(*result)->page_sec)))
      goto ERROR5;
  }

  psiconv_progress(config,lev+2,sto,
                   "Looking for the Sheet Workbook section");
  if (! workbook_sec) {
   psiconv_error(config,lev+2,sto,
                "Sheet workbook section not found in the section table");
    res = -PSICONV_E_PARSE;
    goto ERROR6;
  } else {
    psiconv_debug(config,lev+2,sto,
                  "Sheet workbook section at offset %08x",page_sec);
    if ((res = psiconv_parse_sheet_workbook_section(config,buf,lev+2,workbook_sec,NULL,
                                             &(*result)->workbook_sec)))
      goto ERROR6;
  }

  psiconv_free_application_id_section(appl_id);
  psiconv_free_section_table_section(table);

  psiconv_progress(config,lev+1,off,"End of Sheet file");
  return 0;

ERROR6:
  psiconv_free_page_layout_section((*result)->page_sec);
ERROR5:
  psiconv_free_application_id_section(appl_id);
ERROR4:
  psiconv_free_sheet_status_section((*result)->status_sec);
ERROR3:
  psiconv_free_section_table_section(table);
ERROR2:
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet File failed");
  if (res == 0)
    return -PSICONV_E_NOMEM;
  else
    return res;
}
