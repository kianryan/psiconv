/*
    parse_sheet.c - Part of psiconv, a PSION 5 file formats converter
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

#include "parse_routines.h"
#include "error.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static psiconv_sheet_cell_layout psiconv_basic_cell_layout(void)
{
  psiconv_sheet_cell_layout result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->character = psiconv_basic_character_layout()))
    goto ERROR2;
  if (!(result->paragraph = psiconv_basic_paragraph_layout()))
    goto ERROR3;
  if (!(result->numberformat = malloc(sizeof(*result->numberformat))))
    goto ERROR4;
  result->numberformat->code = psiconv_numberformat_general;
  result->numberformat->decimal = 2;
  return result;
ERROR4:
  psiconv_free_paragraph_layout(result->paragraph);
ERROR3:
  psiconv_free_character_layout(result->character);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

static psiconv_sheet_cell_layout psiconv_clone_cell_layout
                                    (psiconv_sheet_cell_layout original)
{
  psiconv_sheet_cell_layout result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->character = 
        psiconv_clone_character_layout(original->character)))
    goto ERROR2;
  if (!(result->paragraph = 
        psiconv_clone_paragraph_layout(original->paragraph)))
    goto ERROR3;
  if (!(result->numberformat = malloc(sizeof(*result->numberformat))))
    goto ERROR4;
  result->numberformat->code = original->numberformat->code;
  result->numberformat->decimal = original->numberformat->decimal;
  return result;
ERROR4:
  psiconv_free_paragraph_layout(result->paragraph);
ERROR3:
  psiconv_free_character_layout(result->character);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

static psiconv_sheet_cell_reference_t 
       psiconv_read_var_cellref (const psiconv_config config,
                                 const psiconv_buffer buf, int lev,
                                 psiconv_u32 off, int *length,
                                 int *status)
{
  int len=0; 
  int res;
  psiconv_sheet_cell_reference_t result;
  psiconv_u32 temp;

  psiconv_progress(config,lev+1,off+len,"Going to read a sheet cell reference");
  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x00);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp != 0x00) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet cell reference initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp & 0xffff0000) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet cell row reference to unknown row (reset)");
  }
  result.row.offset = temp;
  result.row.absolute = psiconv_bool_true;
  len += 4;

  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp & 0xffff0000) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet cell column reference to unknown row (reset)");
  }
  result.column.offset = temp;
  result.column.absolute = psiconv_bool_true;
  len += 4;

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet column reference (total length: %08x)", len);
  return result;
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Column Reference failed");
  if (length)
    *length = 0;
  if (status)
    *status = res?res:-PSICONV_E_NOMEM;
  return result;
}

static psiconv_sheet_cell_block_t 
       psiconv_read_var_cellblock (const psiconv_config config,
                                   const psiconv_buffer buf, int lev,
                                   psiconv_u32 off, int *length,
                                   int *status)
{
  int len=0; 
  int res;
  psiconv_sheet_cell_block_t result;
  psiconv_u32 temp;

  psiconv_progress(config,lev+1,off+len,"Going to read a sheet cell block reference");
  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x00);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp != 0x00) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet cell reference initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp & 0xffff0000) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet block initial row reference to unknown row (reset)");
  }
  result.first.row.offset = temp;
  result.first.row.absolute = psiconv_bool_true;
  len += 4;

  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp & 0xffff0000) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet block initial column reference to unknown row (reset)");
  }
  result.first.column.offset = temp;
  result.first.column.absolute = psiconv_bool_true;
  len += 4;

  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp & 0xffff0000) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet block final row reference to unknown row (reset)");
  }
  result.last.row.offset = temp;
  result.last.row.absolute = psiconv_bool_true;
  len += 4;

  temp = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp & 0xffff0000) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet block final column reference to unknown row (reset)");
  }
  result.last.column.offset = temp;
  result.last.column.absolute = psiconv_bool_true;
  len += 4;

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet cell block reference (total length: %08x)", 
                   len);
  return result;
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Cell Block Reference failed");
  if (length)
    *length = 0;
  if (status)
    *status = res?res:-PSICONV_E_NOMEM;
  return result;
}

int psiconv_parse_sheet_numberformat(const psiconv_config config,
                                     const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_sheet_numberformat result)
{
  int res=0;
  int len=0;
  psiconv_u8 temp;

  psiconv_progress(config,lev+1,off,"Going to read a sheet numberformat");

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet numberformat initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len, "Going to read the code byte");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  psiconv_debug(config,lev+2,off+len,"Code: %02x",temp);
  if (temp == 0x00)
    result->code = psiconv_numberformat_general;
  else if (temp == 0x02)
    result->code = psiconv_numberformat_fixeddecimal;
  else if (temp == 0x04)
    result->code = psiconv_numberformat_scientific;
  else if (temp == 0x06)
    result->code = psiconv_numberformat_currency;
  else if (temp == 0x08)
    result->code = psiconv_numberformat_percent;
  else if (temp == 0x0A)
    result->code = psiconv_numberformat_triads;
  else if (temp == 0x0C)
    result->code = psiconv_numberformat_boolean;
  else if (temp == 0x0E)
    result->code = psiconv_numberformat_text;
  else if (temp == 0x10)
    result->code = psiconv_numberformat_date_dmm;
  else if (temp == 0x12)
    result->code = psiconv_numberformat_date_mmd;
  else if (temp == 0x14)
    result->code = psiconv_numberformat_date_ddmmyy;
  else if (temp == 0x16)
    result->code = psiconv_numberformat_date_mmddyy;
  else if (temp == 0x18)
    result->code = psiconv_numberformat_date_yymmdd;
  else if (temp == 0x1A)
    result->code = psiconv_numberformat_date_dmmm;
  else if (temp == 0x1C)
    result->code = psiconv_numberformat_date_dmmmyy;
  else if (temp == 0x1E)
    result->code = psiconv_numberformat_date_ddmmmyy;
  else if (temp == 0x20)
    result->code = psiconv_numberformat_date_mmm;
  else if (temp == 0x22)
    result->code = psiconv_numberformat_date_monthname;
  else if (temp == 0x24)
    result->code = psiconv_numberformat_date_mmmyy;
  else if (temp == 0x26)
    result->code = psiconv_numberformat_date_monthnameyy;
  else if (temp == 0x28)
    result->code = psiconv_numberformat_date_monthnamedyyyy;
  else if (temp == 0x2A)
    result->code = psiconv_numberformat_datetime_ddmmyyyyhhii;
  else if (temp == 0x2C)
    result->code = psiconv_numberformat_datetime_ddmmyyyyHHii;
  else if (temp == 0x2E)
    result->code = psiconv_numberformat_datetime_mmddyyyyhhii;
  else if (temp == 0x30)
    result->code = psiconv_numberformat_datetime_mmddyyyyHHii;
  else if (temp == 0x32)
    result->code = psiconv_numberformat_datetime_yyyymmddhhii;
  else if (temp == 0x34)
    result->code = psiconv_numberformat_datetime_yyyymmddHHii;
  else if (temp == 0x36)
    result->code = psiconv_numberformat_time_hhii;
  else if (temp == 0x38)
    result->code = psiconv_numberformat_time_hhiiss;
  else if (temp == 0x3A)
    result->code = psiconv_numberformat_time_HHii;
  else if (temp == 0x3C)
    result->code = psiconv_numberformat_time_HHiiss;
  else {
    psiconv_warn(config,lev+2,off+len,"Unknown number format (assumed general)");
    result->code = psiconv_numberformat_general;
  }
  len ++;

  psiconv_progress(config,lev+2,off+len, "Going to read the number of decimals");
  result->decimal = psiconv_read_u8(config,buf,lev+2,off+len,&res) >> 1;
  if (res)
    goto ERROR1;
  psiconv_debug(config,lev+2,off+len,"Decimals: %d",result->decimal);
  len ++;
  
  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet number format (total length: %08x)", len);
  return 0;

ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Number Format failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_status_section(const psiconv_config config,
                                       const psiconv_buffer buf, int lev,
                                       psiconv_u32 off, int *length, 
                                       psiconv_sheet_status_section *result)
{
  int res=0;
  int len=0;
  psiconv_u32 temp;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read the sheet status section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet status section initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the cursor row");
  (*result)->cursor_row = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Cursor row: %08x",
                (*result)->cursor_row);
  len += 0x04;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the cursor column");
  (*result)->cursor_column = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Cursor column: %08x",
                (*result)->cursor_column);
  len += 0x04;

  psiconv_progress(config,lev+2,off+len,"Going to read initially display graph");
  if ((res = psiconv_parse_bool(config,buf,lev+2,off+len,&leng,
             &(*result)->show_graph)))
    goto ERROR2;
  len += leng;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the toolbar status byte");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;

  (*result)->show_side_sheet_toolbar = temp&0x01 ? psiconv_bool_true : 
                                                   psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show side sheet toolbar: %02x",
                (*result)->show_side_sheet_toolbar);
  (*result)->show_top_sheet_toolbar = temp&0x02 ? psiconv_bool_true : 
                                                  psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show top sheet toolbar: %02x",
                (*result)->show_top_sheet_toolbar);
  (*result)->show_side_graph_toolbar = temp&0x04 ? psiconv_bool_true : 
                                                   psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show side graph toolbar: %02x",
                (*result)->show_side_graph_toolbar);
  (*result)->show_top_graph_toolbar = temp&0x08 ? psiconv_bool_true : 
                                                  psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Show top graph toolbar: %02x",
                (*result)->show_top_graph_toolbar);
  if (temp & 0xf0) {
    psiconv_warn(config,lev+2,off+len,"Sheet status section toolbar byte "
                               "flags contains unknown flags (ignored)");
    psiconv_debug(config,lev+2,off+len,"Unknown flags: %02x",temp & 0xf0);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the scrollbar status byte");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if ((temp & 0x03) == 0x03) {
    psiconv_warn(config,lev+2,off+len,"Sheet status section scrollbar byte "
                               "flags contains unknown flags (ignored)");
    psiconv_debug(config,lev+2,off+len,"Unknown flag: %02x",temp & 0x03);
  }
  (*result)->show_horizontal_scrollbar = (temp&0x03) == 1? psiconv_triple_off : 
                                         (temp&0x03) == 2? psiconv_triple_auto:
                                                           psiconv_triple_on;
  psiconv_debug(config,lev+2,off+len,"Show horizontal scrollbar: %02x",
                (*result)->show_horizontal_scrollbar);
  if ((temp & 0x0c) == 0x0c) {
    psiconv_warn(config,lev+2,off+len,"Sheet status section scrollbar byte "
                               "flags contains unknown flags (ignored)");
    psiconv_debug(config,lev+2,off+len,"Unknown flag: %02x",temp & 0x0c);
  }
  (*result)->show_vertical_scrollbar = (temp&0x0c) ==0x04? psiconv_triple_off:
                                       (temp&0x0c) ==0x08? psiconv_triple_auto:
                                                           psiconv_triple_on;
  psiconv_debug(config,lev+2,off+len,"Show vertical scrollbar: %02x",
                (*result)->show_vertical_scrollbar);
  if (temp & 0xf0) {
    psiconv_warn(config,lev+2,off+len,"Sheet status section scrollbar byte "
                               "flags contains unknown flags (ignored)");
    psiconv_debug(config,lev+2,off+len,"Unknown flags: %02x",temp & 0xf0);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read an unknown byte (%02x expected)",0x00);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x00) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet status section unknown byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Unknown byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read sheet display size");
  (*result)->sheet_display_size = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Sheet display size: %08x",
                (*result)->sheet_display_size);
  len += 0x04;

  psiconv_progress(config,lev+2,off+len,"Going to read graph display size");
  (*result)->graph_display_size = psiconv_read_u32(config,buf,lev+2,off + len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Graph display size: %08x",
                (*result)->graph_display_size);
  len += 0x04;

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet status section (total length: %08x)", len);
  return 0;

ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Status Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_workbook_section(const psiconv_config config,
                                        const psiconv_buffer buf, int lev,
                                        psiconv_u32 off, int *length,
                                        psiconv_sheet_workbook_section *result)
{
  int res=0,with_name;
  psiconv_u32 temp,formulas_off,worksheets_off,info_off,var_off,name_off=0;
  int len=0;

  psiconv_progress(config,lev+1,off,"Going to read the sheet workbook section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x or %02x expected)",
                   0x02,0x04);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if ((temp != 0x04) && temp !=0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet workbook section initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  with_name = temp ==0x04;
  len ++;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read the offset of the sheet info Section");
  info_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Offset: %04x",info_off);
  len += 4;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read the offset of the Formulas List");
  formulas_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Offset: %04x",formulas_off);
  len += 4;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read the offset of the Worksheet List");
  worksheets_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Offset: %04x",worksheets_off);
  len += 4;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read the offset of the Variable List");
  var_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Offset: %04x",var_off);
  len += 4;

  if (with_name) {
    psiconv_progress(config,lev+2,off+len,
                      "Going to read the offset of the Name Section");
    name_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR2;
    psiconv_debug(config,lev+2,off+len,"Offset: %04x",name_off);
    len += 4;
  }

  
  psiconv_progress(config,lev+2,off+len,"Going to read the info section");
  if ((res = psiconv_parse_sheet_info_section(config,buf,lev+2,info_off,NULL,
                                               &(*result)->info)))
    goto ERROR2;

  psiconv_progress(config,lev+2,off+len,"Going to read the variables list");
  if ((res = psiconv_parse_sheet_variable_list(config,buf,lev+2,var_off,NULL,
                                                   &(*result)->variables)))
    goto ERROR3;

  psiconv_progress(config,lev+2,off+len,"Going to read the formulas list");
  if ((res = psiconv_parse_sheet_formula_list(config,buf,lev+2,formulas_off,NULL,
                                               &(*result)->formulas)))
    goto ERROR4;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the worksheet list");
  if ((res = psiconv_parse_sheet_worksheet_list(config,buf,lev+2,worksheets_off,
                                                NULL,&(*result)->worksheets)))
    goto ERROR5;

  if (with_name) {
    psiconv_progress(config,lev+2,off+len,"Going to read the name section");
    if ((res = psiconv_parse_sheet_name_section(config,buf,lev+2,name_off,NULL,
                                               &(*result)->name)))
      goto ERROR6;
  } else 
    (*result)->name = NULL;
  
  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet workbook section (total length: %08x)", len);
  return 0;

ERROR6:
  psiconv_free_sheet_worksheet_list((*result)->worksheets);
ERROR5:
  psiconv_free_formula_list((*result)->formulas);
ERROR4:
  psiconv_free_sheet_variable_list((*result)->variables);
ERROR3:
  psiconv_free_sheet_info_section((*result)->info);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Workbook Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_name_section(const psiconv_config config,
                                     const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_sheet_name_section *result)
{
  int res=0;
  psiconv_u32 temp;
  int len=0,leng;

  psiconv_progress(config,lev+1,off,"Going to read the sheet name section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet name section initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len, "Going to read the sheet name");
  (*result)->name = psiconv_read_string(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  len += leng;
  
  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet name section (total length: %08x)", len);
  return 0;

ERROR2:  
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Name Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_info_section(const psiconv_config config,
                                     const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_sheet_info_section *result)
{
  int res=0;
  psiconv_u32 temp;
  int len=0,leng;

  psiconv_progress(config,lev+1,off,"Going to read the sheet info section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet info section initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len, "Going to read an unknown Xint");
  temp = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Value: %d\n",temp);
  len += leng;

  psiconv_progress(config,lev+2,off+len, "Going to read the flags byte");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  (*result)->auto_recalc = temp & 0x01 ? psiconv_bool_true:psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,"Auto recalculation: %02x",
                (*result)->auto_recalc);
  if ((temp & 0xfe) != 0x02) {
    psiconv_warn(config,lev+2,off+len,"Sheet Info Section flags byte "
                               "contains unknown flags (ignored)");
    psiconv_debug(config,lev+2,off+len,"Unknown flags: %02x",temp &0xfe);
  }
  
  len ++;

  
  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet info section (total length: %08x)", len);
  return 0;

ERROR2:  
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Name Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_formula_list(const psiconv_config config,
                                     const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_formula_list *result)
{
  int res=0;
  int len=0;
  psiconv_u32 temp;
  psiconv_formula formula;
  psiconv_u32 listlen,i;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read the sheet formula list");
  if (!(*result = psiconv_list_new(sizeof(struct psiconv_formula_s))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet formula list initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the number of formulas");
  listlen = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Number of formulas: %d",listlen);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read all formulas");
  for (i = 0; i < listlen; i++) {
    psiconv_progress(config,lev+3,off+len,"Going to read formula %d",i);
    if ((res = psiconv_parse_formula(config,buf,lev+3,off+len,&leng,&formula)))
      goto ERROR2;
    if ((res = psiconv_list_add(*result,formula)))
      goto ERROR3;
    free(formula);
    len += leng;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet formula list (total length: %08x)", len);
  return 0;

ERROR3:
  psiconv_free_formula(formula);
ERROR2:
  psiconv_list_free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Formula list failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_cell(const psiconv_config config,
                             const psiconv_buffer buf, int lev,
                             psiconv_u32 off, int *length,
                             psiconv_sheet_cell *result,
                             const psiconv_sheet_cell_layout default_layout,
                             const psiconv_sheet_line_list row_default_layouts,
                             const psiconv_sheet_line_list col_default_layouts)
{
  int res=0;
  int len=0;
  psiconv_u32 temp;
  psiconv_bool_t has_layout;
  int leng;
  char *auxstr;

  psiconv_progress(config,lev+1,off,"Going to read a sheet cell structure");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  (*result)->layout = NULL;
  (*result)->type = psiconv_cell_blank;

  psiconv_progress(config,lev+2,off+len,"Going to read the cell position");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  len ++;
  temp += psiconv_read_u8(config,buf,lev+2,off+len,&res) << 8;
  if (res)
    goto ERROR2;
  len ++;
  temp += psiconv_read_u8(config,buf,lev+2,off+len,&res) << 16;
  if (res)
    goto ERROR2;
  len ++;
  (*result)->column = (temp >> 2) & 0xFF;
  (*result)->row = (temp >> 10) & 0x3FFF;
  psiconv_debug(config,lev+2,off+len,"Cell position is col:%02x row:%04x",
                                      (*result)->column,(*result)->row);
  if (temp & 0x03) {
    psiconv_warn(config,lev+2,off+len,"Unknown flags in cell position (ignored)");
    psiconv_debug(config,lev+2,off+len,"Flags: %02x",temp & 0x03);
  }

  psiconv_progress(config,lev+2,off+len,"Going to read the cell type");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  len ++;
  (*result)->type = (temp >> 5) & 0x07;
  (*result)->calculated = (temp & 0x08)?psiconv_bool_true:psiconv_bool_false;
  has_layout = (temp & 0x10)?psiconv_bool_true:psiconv_bool_false;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the cell value");
  if ((*result)->type == psiconv_cell_blank) {
    psiconv_debug(config,lev+2,off+len,"Cell type is blank: no value given.");
  } else if ((*result)->type == psiconv_cell_int) {
    psiconv_progress(config,lev+2,off+len,"Going to read an integer");
    (*result)->data.dat_int = psiconv_read_u32(config,buf,lev+2,off+len,&res);
	if (res)
       goto ERROR2;
	len += 4;
    psiconv_debug(config,lev+2,off+len,"Cell contents: %ld",(*result)->data.dat_int);

  } else if ((*result)->type == psiconv_cell_bool) {
    psiconv_progress(config,lev+2,off+len,"Going to read a boolean");
    if ((res = psiconv_parse_bool(config,buf,lev+2,off+len,&leng,
                                  &(*result)->data.dat_bool)))
       goto ERROR2;
    psiconv_debug(config,lev+2,off+len,"Cell contents: %01x",temp);
	(*result)->data.dat_bool = temp?psiconv_bool_true:psiconv_bool_false;
    len += leng;
  } else if ((*result)->type == psiconv_cell_error) {
    psiconv_progress(config,lev+2,off+len,"Going to read the error code");
    temp = psiconv_read_u16(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR2;
    if (temp == 0)
      (*result)->data.dat_error = psiconv_sheet_error_none;
    else if (temp == 1)
      (*result)->data.dat_error = psiconv_sheet_error_null;
    else if (temp == 2)
      (*result)->data.dat_error = psiconv_sheet_error_divzero;
    else if (temp == 3)
      (*result)->data.dat_error = psiconv_sheet_error_value;
    else if (temp == 4)
      (*result)->data.dat_error = psiconv_sheet_error_reference;
    else if (temp == 5)
      (*result)->data.dat_error = psiconv_sheet_error_name;
    else if (temp == 6)
      (*result)->data.dat_error = psiconv_sheet_error_number;
    else if (temp == 7)
      (*result)->data.dat_error = psiconv_sheet_error_notavail;
    else {
      psiconv_warn(config,lev+2,off+len,"Unknown error code (default assumed)");
      psiconv_debug(config,lev+2,off+len,"Error code: %04x",temp);
      (*result)->data.dat_error = psiconv_sheet_error_none;
    }
    psiconv_debug(config,lev+2,off+len,"Cell contents: %04x",
                  (*result)->data.dat_error);
    len += 2;
  } else if ((*result)->type == psiconv_cell_float) {
    psiconv_progress(config,lev+2,off+len,"Going to read a float");
    (*result)->data.dat_float =
                            psiconv_read_float(config,buf,lev+2,off+len,&leng,&res);
    if (res)
      goto ERROR2;
    psiconv_debug(config,lev+2,off+len,"Cell contents: %f",(*result)->data.dat_float);
    len += leng;
  } else if ((*result)->type == psiconv_cell_string) {
    psiconv_progress(config,lev+2,off+len,"Going to read a string");
    (*result)->data.dat_string =
                   psiconv_read_string(config,buf,lev+2,off+len,&leng,&res);
    if (res)
      goto ERROR2;
    if (!(auxstr = psiconv_make_printable(config,(*result)->data.dat_string)))
      goto ERROR2;
    psiconv_debug(config,lev+2,off+len,"Cell contents: `%s'",auxstr);
    free(auxstr);
    len += leng;
  } else {
    psiconv_error(config,lev+2,off+len,"Unknown Sheet Cell type: %02x",(*result)->type);
    res = PSICONV_E_PARSE;
    goto ERROR2;
  }
  
  if (!((*result)->layout = psiconv_clone_cell_layout(
                     psiconv_get_default_layout(row_default_layouts,
                                                col_default_layouts,
                                                default_layout,
                                                (*result)->row,
                                                (*result)->column))))
    goto ERROR2;
  if (has_layout) {
    if ((res = psiconv_parse_sheet_cell_layout(config,buf,lev+2,off+len,
                                               &leng,(*result)->layout))) 
      goto ERROR2;
    len += leng;
  }

  if ((*result)->calculated) {
  	psiconv_progress(config,lev+2,off+len,"Going to read the cell formula reference");
    temp = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
    if (res)
      goto ERROR2;
    psiconv_debug(config,lev+2,off+len,"Cell formula reference: %d",temp);
    len += leng;
    (*result)->ref_formula = temp;
  }
  
  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet cell structure (total length: %08x)", len);
  return 0;

ERROR2:
  psiconv_free_sheet_cell(*result);
ERROR1:
  psiconv_warn(config,lev+1,off,"Reading of Sheet Cell Structure failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_cell_list(const psiconv_config config,
                              const psiconv_buffer buf, int lev,
                              psiconv_u32 off, int *length,
                              psiconv_sheet_cell_list *result,
                              const psiconv_sheet_cell_layout default_layout,
                              const psiconv_sheet_line_list row_default_layouts,
                              const psiconv_sheet_line_list col_default_layouts)
{
  int res=0;
  int len=0;
  psiconv_u32 temp;
  psiconv_sheet_cell cell;
  psiconv_u32 listlen,i;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read the sheet cell list");
  if (!(*result = psiconv_list_new(sizeof(struct psiconv_sheet_cell_s))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet cell list initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x00);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x00) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet cell list initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the number of defined cells");
  listlen = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Number of defined cells: %d",listlen);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read all cells");
  for (i = 0; i < listlen; i++) {
    psiconv_progress(config,lev+3,off+len,"Going to read cell %d",i);
    if ((res = psiconv_parse_sheet_cell(config,buf,lev+3,off+len,&leng,&cell,
                                        default_layout,row_default_layouts,
                                        col_default_layouts)))
      goto ERROR2;
    if ((res = psiconv_list_add(*result,cell)))
      goto ERROR3;
    free(cell);
    len += leng;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet cell list (total length: %08x)", len);
  return 0;

ERROR3:
  psiconv_free_sheet_cell(cell);
ERROR2:
  psiconv_free_sheet_cell_list(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Cells List failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}


int psiconv_parse_sheet_worksheet_list(const psiconv_config config,
                                       const psiconv_buffer buf, int lev,
                                       psiconv_u32 off, int *length,
                                       psiconv_sheet_worksheet_list *result)
{
  psiconv_sheet_worksheet worksheet;
  int res=0;
  int len=0;
  psiconv_u8 temp;
  psiconv_u32 offset;
  int leng,i,nr;

  psiconv_progress(config,lev+1,off,"Going to read the worksheet list");
  if (!(*result = psiconv_list_new(sizeof(*worksheet))))
    goto ERROR1;
  
  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial bytes (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
               "Sheet worksheet list initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read the list length");
  nr = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Length: %02x",nr);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the list");
  for (i=0 ; i < nr; i++) {
    psiconv_progress(config,lev+3,off+len,"Going to read element %d",i);
    psiconv_progress(config,lev+4,off+len,
                     "Going to read the initial byte (%02x expected)",0x00);
    temp = psiconv_read_u8(config,buf,lev+4,off+len,&res);
    if (res)
      goto ERROR2;
    if (temp != 0x00) {
      psiconv_warn(config,lev+4,off+len,
               "Sheet worksheet element initial byte unknown value (ignored)");
      psiconv_debug(config,lev+4,off+len,"Initial byte: %02x",temp);
    }
    len ++;

    psiconv_progress(config,lev+4,off+len,"Going to read the worksheet offset");
    offset = psiconv_read_u32(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR2;
    psiconv_debug(config,lev+4,off+len,"Offset: %08x",offset);
    len += 4;

    if ((res =  psiconv_parse_sheet_worksheet(config,buf,lev+4,offset,NULL,
                                              &worksheet)))
      goto ERROR2;
    if ((res = psiconv_list_add(*result,worksheet)))
      goto ERROR3;
    free(worksheet);
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of worksheet list (total length: %08x)", len);

  return 0;

ERROR3:
  psiconv_free_sheet_worksheet(worksheet);
ERROR2:
  psiconv_free_sheet_worksheet_list(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of worksheet list failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_cell_layout(const psiconv_config config,
                                    const psiconv_buffer buf, int lev,
                                    psiconv_u32 off, int *length,
                                    psiconv_sheet_cell_layout result)

{
  int res=0;
  int len=0;
  int leng;
  psiconv_u8 temp;

  psiconv_progress(config,lev+1,off,"Going to read a sheet cell layout");

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the first byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
               "Worksheet section initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read the default formats flag");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR1;
  len ++;

  if (temp & 0x01) {
    psiconv_progress(config,lev+3,off+len,"Going to read the default paragraph codes");
    if ((res = psiconv_parse_paragraph_layout_list(config,buf,lev+3,off+len,&leng,
                                               result->paragraph)))
      goto ERROR1;
    len += leng;
  }

  if (temp & 0x02) {
    psiconv_progress(config,lev+3,off+len,"Going to read the default character codes");
    if ((res = psiconv_parse_character_layout_list(config,buf,lev+3,off+len,&leng,
                                               result->character)))
      goto ERROR1;
    len += leng;
  }
  
  if (temp & 0x04) {
    psiconv_progress(config,lev+3,off+len, "Going to read the default number format");
    psiconv_parse_sheet_numberformat(config,buf,lev+3,off+len,&leng,
                                     result->numberformat);
    len += leng;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet cell layout (total length: %08x)", len);

  return 0;

ERROR1:
  psiconv_error(config,lev+1,off,"Reading of sheet cell layout failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}
  

int psiconv_parse_sheet_worksheet(const psiconv_config config,
                                  const psiconv_buffer buf, int lev,
                                  psiconv_u32 off, int *length,
                                  psiconv_sheet_worksheet *result)
{
  int res=0;
  psiconv_u32 temp,cells_off,grid_off,rows_off,cols_off,unknown_off;
  int len=0;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read the sheet worksheet section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial bytes (%02x expected)",0x04);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x04) {
    psiconv_warn(config,lev+2,off+len,
              "Worksheet section initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len, "Going to read the flags byte");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Flags byte: %02x",temp);
  (*result)->show_zeros = (temp & 0x01)?psiconv_bool_true:psiconv_bool_false;
  if (temp & 0xfe) {
    psiconv_warn(config,lev+2,off+len,
               "Worksheet section flags byte unknown bits (ignored)");
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read the default cell layout");
  if (!((*result)->default_layout = psiconv_basic_cell_layout()))
    goto ERROR2;
  if ((res = psiconv_parse_sheet_cell_layout(config,buf,lev+2,off+len,&leng,
                                             (*result)->default_layout)))
    goto ERROR3;
  len += leng;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read the offset of the row defaults Section");
  rows_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Offset: %04x",rows_off);
  len += 4;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read the offset of the column defaults Section");
  cols_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Offset: %04x",cols_off);
  len += 4;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read the offset of the Cells List");
  cells_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Offset: %04x",cells_off);
  len += 4;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read the offset of the Grid Section");
  grid_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Offset: %04x",grid_off);
  len += 4;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read the offset of the 3rd ??? Section");
  unknown_off = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Offset: %04x",unknown_off);
  len += 4;

  psiconv_progress(config,lev+2,off+len,
                    "Going to read a long of the 3rd ??? Section "
                    "(%08x expected)",0x00);
  temp = psiconv_read_u32(config,buf,lev+2,unknown_off,&res);
  if (res)
    goto ERROR3;
  if (temp != 0x00) {
    psiconv_warn(config,lev+2,unknown_off,
                 "Unknown worksheet subsection has unknown contents (ignored)");
    psiconv_debug(config,lev+2,unknown_off,"Offset: %04x",temp);
  }
  len += 4;

  psiconv_progress(config,lev+2,off+len,"Going to read the row defaults");
  if ((res = psiconv_parse_sheet_line_list(config,buf,lev+2,rows_off,NULL,
                                           &(*result)->row_default_layouts,
                                           (*result)->default_layout)))
    goto ERROR3;

  psiconv_progress(config,lev+2,off+len,"Going to read the column defaults");
  if ((res = psiconv_parse_sheet_line_list(config,buf,lev+2,cols_off,NULL,
                                           &(*result)->col_default_layouts,
                                           (*result)->default_layout)))
    goto ERROR4;

  psiconv_progress(config,lev+2,off+len,"Going to read the cells list");
  if ((res = psiconv_parse_sheet_cell_list(config,buf,lev+2,cells_off,NULL,
                                           &(*result)->cells,
                                           (*result)->default_layout,
                                           (*result)->row_default_layouts,
                                           (*result)->col_default_layouts)))
    goto ERROR5;
  

  psiconv_progress(config,lev+2,off+len,"Going to read the grid section");
  if ((res = psiconv_parse_sheet_grid_section(config,buf,lev+2,grid_off,NULL,
                                           &(*result)->grid)))
    goto ERROR6;


  
  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet worksheet section (total length: %08x)", len);
  return 0;

ERROR6:
  psiconv_free_sheet_cell_list((*result)->cells);
ERROR5:
  psiconv_free_sheet_line_list((*result)->col_default_layouts);
ERROR4:
  psiconv_free_sheet_line_list((*result)->row_default_layouts);
ERROR3:
  psiconv_free_sheet_cell_layout((*result)->default_layout); 
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Worksheet Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_line(const psiconv_config config,
                             const psiconv_buffer buf, int lev,
                             psiconv_u32 off, int *length,
                             psiconv_sheet_line *result,
                             const psiconv_sheet_cell_layout default_layout)
{
  int res=0;
  int len=0;
  int leng;


  psiconv_progress(config,lev+1,off,"Going to read a sheet line");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,"Going to read the line number");
  (*result)->position = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
  if (res) 
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Line number: %d\n",(*result)->position);
  len += leng;

  if (!((*result)->layout = psiconv_clone_cell_layout(default_layout)))
    goto ERROR2;
  if ((res = psiconv_parse_sheet_cell_layout(config,buf,lev+2,off+len,
                                              &leng,(*result)->layout))) 
      goto ERROR3;
  len += leng;

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of the sheet line (total length: %08x)", len);
  return 0;
  
ERROR3:
  psiconv_free_sheet_cell_layout((*result)->layout);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of the sheet line failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}


int psiconv_parse_sheet_line_list(const psiconv_config config,
                                 const psiconv_buffer buf, int lev,
                                 psiconv_u32 off, int *length,
                                 psiconv_sheet_line_list *result,
                                 const psiconv_sheet_cell_layout default_layout)
{
  int res=0;
  int len=0;
  psiconv_u32 temp;
  psiconv_sheet_line line;
  psiconv_u32 listlen,i;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read the sheet line list");
  if (!(*result = psiconv_list_new(sizeof(struct psiconv_sheet_line_s))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet line list initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the number of defined lines");
  listlen = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Number of defined lines: %d",listlen);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read all lines");
  for (i = 0; i < listlen; i++) {
    psiconv_progress(config,lev+3,off+len,"Going to read line %d",i);
    if ((res = psiconv_parse_sheet_line(config,buf,lev+3,off+len,&leng,&line,
                                        default_layout)))
      goto ERROR2;
    if ((res = psiconv_list_add(*result,line)))
      goto ERROR3;
    free(line);
    len += leng;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet line list (total length: %08x)", len);
  return 0;

ERROR3:
  psiconv_free_sheet_line(line);
ERROR2:
  psiconv_free_sheet_line_list(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Line List failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_variable(const psiconv_config config,
                                 const psiconv_buffer buf, int lev,
                                 psiconv_u32 off, int *length,
                                 psiconv_sheet_variable *result)
{
  int res=0;
  int len=0;
  psiconv_u32 marker;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read a sheet variable");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len, "Going to read the variable name");
  (*result)->name = psiconv_read_string(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the type marker");
  marker = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Marker: %02x",marker);
  len ++;

  if (marker == 0x00) {
    (*result)->type = psiconv_var_int;
    psiconv_progress(config,lev+2,off+len,"Going to read a signed integer");
    (*result)->data.dat_int = psiconv_read_sint(config,buf,lev+2,off+len,&leng,&res);
    if (res)
      goto ERROR3;
    psiconv_debug(config,lev+2,off+len,"Value: %d",(*result)->data.dat_int);
    len += leng;
  } else if (marker == 0x01) {
    (*result)->type = psiconv_var_float;
    psiconv_progress(config,lev+2,off+len,"Going to read a floating point number");
    (*result)->data.dat_float = psiconv_read_float(config,buf,lev+2,off+len,&leng,
                                                   &res);
    if (res)
      goto ERROR3;
    psiconv_debug(config,lev+2,off+len,"Value: %f",(*result)->data.dat_float);
    len += leng;
  } else if (marker == 0x02) {
    (*result)->type = psiconv_var_string;
    psiconv_progress(config,lev+2,off+len,"Going to read a string");
    (*result)->data.dat_string = psiconv_read_string(config,buf,lev+2,off+len,
						     &leng, &res);
    if (res)
      goto ERROR3;
    len += leng;
  } else if (marker == 0x03) {
    (*result)->type = psiconv_var_cellref;
    psiconv_progress(config,lev+2,off+len,"Going to read a cell reference");
    (*result)->data.dat_cellref = psiconv_read_var_cellref(config,buf,lev+2,off+len,
                                                           &leng, &res);
    if (res)
      goto ERROR3;
    len += leng;
  } else if (marker == 0x04) {
    (*result)->type = psiconv_var_cellblock;
    psiconv_progress(config,lev+2,off+len,"Going to read a cell block reference");
    (*result)->data.dat_cellblock = psiconv_read_var_cellblock(config,buf,lev+2,
                                                              off+len,
                                                              &leng, &res);
    if (res)
      goto ERROR3;
    len += leng;
  } else {
    psiconv_error(config,lev+2,off+len,"Sheet variable unknown type marker");
    res = -PSICONV_E_PARSE;
    goto ERROR3;
  }
    
  psiconv_progress(config,lev+2,off+len,"Going to read the variable number");
  (*result)->number = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR4;
  psiconv_debug(config,lev+2,off+len,"Number: %08x",(*result)->number);
  len += 4;

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet variable (total length: %08x)", len);
  return 0;

ERROR4:
  if ((*result)->type == psiconv_var_string)
    free((*result)->data.dat_string);
ERROR3:
  free((*result)->name);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Variable failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}


int psiconv_parse_sheet_variable_list(const psiconv_config config,
                                      const psiconv_buffer buf, int lev,
                                      psiconv_u32 off, int *length,
                                      psiconv_sheet_variable_list *result)
{
  int res=0;
  int len=0;
  psiconv_u32 temp;
  psiconv_sheet_variable variable;
  psiconv_u32 listlen,i;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read the sheet variable list");
  if (!(*result = psiconv_list_new(sizeof(struct psiconv_sheet_variable_s))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the initial byte (%02x expected)",0x02);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x02) {
    psiconv_warn(config,lev+2,off+len,
                 "Sheet variable list initial byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Initial byte: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the number of variables");
  listlen = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Number of variables: %d",listlen);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read all variables");
  for (i = 0; i < listlen; i++) {
    psiconv_progress(config,lev+3,off+len,"Going to read variable %d",i);
    if ((res = psiconv_parse_sheet_variable(config,buf,lev+3,off+len,&leng,&variable)))
      goto ERROR2;
    if ((res = psiconv_list_add(*result,variable)))
      goto ERROR3;
    len += leng;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet variabels list (total length: %08x)", len);
  return 0;

ERROR3:
  psiconv_free_sheet_variable(variable);
ERROR2:
  psiconv_list_free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Variable list failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_grid_section(const psiconv_config config,
                                     const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_sheet_grid_section *result)
{
  int res=0,i;
  int len=0,leng;
  psiconv_u32 temp;

  psiconv_progress(config,lev+1,off,"Going to read the sheet grid section");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len, "Going to read the first flags byte");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  (*result)->show_column_titles = temp&0x01?psiconv_bool_true:
                                            psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,
                "Show column titles: %s",
                (*result)->show_column_titles?"true":"false");
  (*result)->show_row_titles = temp&0x02?psiconv_bool_true:psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,
                "Show row titles: %s",
                (*result)->show_row_titles?"true":"false");
  (*result)->show_vertical_grid = temp&0x04?psiconv_bool_true:
                                            psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,
                "Show vertical grid: %s",
                (*result)->show_vertical_grid?"true":"false");
  (*result)->show_horizontal_grid = temp&0x07?psiconv_bool_true:
                                              psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,
                "Show horizontal grid: %s",
                (*result)->show_horizontal_grid?"true":"false");
  (*result)->freeze_rows = temp&0x80?psiconv_bool_true:psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,
                "Freeze rows: %s",
                (*result)->freeze_rows?"true":"false");
  if ((temp & 0x70) != 0x30) {
     psiconv_warn(config,lev+2,off+len,
                  "Grid section first flag byte has unknown bits (ignored)");
     psiconv_debug(config,lev+2,off+len,"Bits: %02x (%02x expected)",temp&0x70,0x30);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len, "Going to read the second flags byte");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  (*result)->freeze_columns = temp&0x01?psiconv_bool_true:psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,
                "Freeze columns: %s", (*result)->freeze_columns?"true":"false");
  if ((temp & 0xfe) != 0x80) {
     psiconv_warn(config,lev+2,off+len,
                  "Grid section second flag byte has unknown bits (ignored)");
     psiconv_debug(config,lev+2,off+len,"Bits: %02x (%02x expected)",temp&0xfe,0x80);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,
                   "Going to an unknown byte (%02x expected)",0x90);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (temp != 0x90) {
    psiconv_warn(config,lev+2,off+len,
                 "Grid section third byte unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Value: %02x",temp);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len, "Going to read the fourth flags byte");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  (*result)->show_page_breaks = temp&0x04?psiconv_bool_true:psiconv_bool_false;
  psiconv_debug(config,lev+2,off+len,
                "Show page breaks: %s", 
                (*result)->show_page_breaks?"true":"false");
  if ((temp & 0xfc) != 0x00) {
     psiconv_warn(config,lev+2,off+len,
                  "Grid section fourth flag byte has unknown bits (ignored)");
     psiconv_debug(config,lev+2,off+len,"Bits: %02x (%02x expected)",temp&0xfc,0x00);
  }
  len ++;

  psiconv_progress(config,lev+2,off+len,"Going to read the first visible row");
  (*result)->first_row = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"First row: %d",(*result)->first_row);
  len += 4;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the first visible column");
  (*result)->first_column = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"First column: %d",(*result)->first_column);
  len += 4;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the last visible row");
  (*result)->last_row = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Last row: %d",(*result)->last_row);
  len += 4;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the last visible column");
  (*result)->last_column = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Last column: %d",(*result)->last_column);
  len += 4;
  
  psiconv_progress(config,lev+2,off+len,"Going to read the default row height");
  (*result)->default_row_height = psiconv_read_length(config,buf,lev+2,off+len,
                                                      &leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Default row height: %f",
                (*result)->default_row_height);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the row heights list");
  if ((res = psiconv_parse_sheet_grid_size_list(config,buf,lev+2,off+len,&leng,
                                          &(*result)->row_heights)))
    goto ERROR2;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the default column height");
  (*result)->default_column_width = psiconv_read_length(config,buf,lev+2,off+len,
                                                         &leng,&res);
  if (res)
    goto ERROR3;
  psiconv_debug(config,lev+2,off+len,"Default column width: %f",
                (*result)->default_column_width);
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the column heights list");
  if ((res = psiconv_parse_sheet_grid_size_list(config,buf,lev+2,off+len,&leng,
                                          &(*result)->column_heights)))
    goto ERROR3;
  len += leng;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read an unknown word (%04x expected)",0x00);
  temp = psiconv_read_u16(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR4;
  if (temp != 0x00) {
    psiconv_warn(config,lev+2,off+len,
                 "Grid section unknown word has unknown value (ignored)");
    psiconv_debug(config,lev+2,off+len,"Value: %04x",temp);
  }
  len += 2;

  psiconv_progress(config,lev+2,off+len,"Going to read the row breaks list");
  if ((res = psiconv_parse_sheet_grid_break_list(config,buf,lev+2,off+len,&leng,
                                           &(*result)->row_page_breaks)))
    goto ERROR4;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the column breaks list");
  if ((res = psiconv_parse_sheet_grid_break_list(config,buf,lev+2,off+len,&leng,
                                           &(*result)->column_page_breaks)))
    goto ERROR5;
  len += leng;

  
  psiconv_progress(config,lev+2,off+len,
                   "Going to read 22 unknown bytes (%02x expected)",0x00);
  for (i = 0; i < 22 ; i++) {
    temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR6;
    if (temp != 0x00) {
      psiconv_warn(config,lev+2,off+len,
                   "Grid section unknown byte %d has unknown value (ignored)",
                    i);
      psiconv_debug(config,lev+2,off+len,"Value: %02x",temp);
    }
    len += 1;
  }
  
  if ((*result)->freeze_rows || (*result)->freeze_columns) {

    psiconv_progress(config,lev+2,off+len,"Going to read number of frozen rows");
    (*result)->frozen_rows = psiconv_read_u32(config,buf,lev+2,off+len, &res);
    if (res)
      goto ERROR6;
    psiconv_debug(config,lev+2,off+len,"Number of frozen rows: %d",
                  (*result)->frozen_rows);
    len += leng;

    psiconv_progress(config,lev+2,off+len,"Going to read number of frozen columns");
    (*result)->frozen_columns = psiconv_read_u32(config,buf,lev+2,off+len, &res);
    if (res)
      goto ERROR6;
    psiconv_debug(config,lev+2,off+len,"Number of frozen columns: %d",
                  (*result)->frozen_columns);
    len += leng;

    psiconv_progress(config,lev+2,off+len,"Going to read first unfrozen row");
    (*result)->first_unfrozen_row_displayed = psiconv_read_u32(config,buf,lev+2,
                                                               off+len, &res);
    if (res)
      goto ERROR6;
    psiconv_debug(config,lev+2,off+len,"First row: %d",
                  (*result)->first_unfrozen_row_displayed);
    len += leng;

    psiconv_progress(config,lev+2,off+len,"Going to read first unfrozen column");
    (*result)->first_unfrozen_column_displayed = psiconv_read_u32(config,buf,lev+2,
                                                                  off+len,&res);
    if (res)
      goto ERROR6;
    psiconv_debug(config,lev+2,off+len,"First column: %d",
                  (*result)->first_unfrozen_column_displayed);
    len += leng;
  } else 
    (*result)->frozen_rows = (*result)->frozen_columns = 
                             (*result)->first_unfrozen_row_displayed =
                             (*result)->first_unfrozen_column_displayed = 0;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read 3 unknown bytes (%02x expected)",0xff);
  for (i = 0; i < 3 ; i++) {
    temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR6;
    if (temp != 0xff) {
      psiconv_warn(config,lev+2,off+len,
                   "Grid section unknown byte %d has unknown value (ignored)",
                    i);
      psiconv_debug(config,lev+2,off+len,"Value: %02x",temp);
    }
    len ++;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet grid section (total length: %08x)", len);
  return 0;

ERROR6:
  psiconv_free_sheet_grid_break_list((*result)->column_page_breaks);
ERROR5:
  psiconv_free_sheet_grid_break_list((*result)->row_page_breaks);
ERROR4:
  psiconv_free_sheet_grid_size_list((*result)->column_heights);
ERROR3:
  psiconv_free_sheet_grid_size_list((*result)->row_heights);
ERROR2:  
  free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Grid Section failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}


int psiconv_parse_sheet_grid_size_list(const psiconv_config config,
                                  const psiconv_buffer buf, int lev,
                                  psiconv_u32 off, int *length,
                                  psiconv_sheet_grid_size_list *result)
{
  int res=0;
  int len=0,i;
  int leng,listlen;
  psiconv_sheet_grid_size size;

  psiconv_progress(config,lev+1,off,"Going to read a sheet grid size list");
  if (!(*result = psiconv_list_new(sizeof(struct psiconv_sheet_grid_size_s))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the number of elements");
  listlen = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Number of elements: %d",listlen);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read all elements");
  for (i = 0; i < listlen; i++) {
    psiconv_progress(config,lev+3,off+len,"Going to read element %d",i);
    if ((res = psiconv_parse_sheet_grid_size(config,buf,lev+3,off+len,&leng,&size)))
      goto ERROR2;
    if ((res = psiconv_list_add(*result,size)))
      goto ERROR3;
    free(size);
    len += leng;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet grid size list (total length: %08x)", len);
  return 0;

ERROR3:
  psiconv_free_sheet_grid_size(size);
ERROR2:
  psiconv_list_free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Grid Size List failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

int psiconv_parse_sheet_grid_size(const psiconv_config config,
                                    const psiconv_buffer buf, int lev,
                                    psiconv_u32 off, int *length,
                                    psiconv_sheet_grid_size *result)
{
  int res=0;
  int len=0;
  int leng;

  psiconv_progress(config,lev+1,off,"Going to read a sheet grid size");

  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len, "Going to read the row or column number");
  (*result)->line_number = psiconv_read_u32(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Line number: %d\n",(*result)->line_number);
  len += 4;

  psiconv_progress(config,lev+2,off+len, "Going to read the row or column height");
  (*result)->size = psiconv_read_length(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Size: %f\n",(*result)->size);
  len += leng;

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet grid size(total length: %08x)", len);
  return 0;

ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Grid Size failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}


int psiconv_parse_sheet_grid_break_list(const psiconv_config config,
                                  const psiconv_buffer buf, int lev,
                                  psiconv_u32 off, int *length,
                                  psiconv_sheet_grid_break_list *result)
{
  int res=0;
  int len=0,i;
  int leng,listlen;
  psiconv_u32 nr;

  psiconv_progress(config,lev+1,off,"Going to read a sheet grid break list");
  if (!(*result = psiconv_list_new(sizeof(psiconv_u32))))
    goto ERROR1;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the number of elements");
  listlen = psiconv_read_X(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR2;
  psiconv_debug(config,lev+2,off+len,"Number of elements: %d",listlen);
  len += leng;
  
  psiconv_progress(config,lev+2,off+len,"Going to read all elements");
  for (i = 0; i < listlen; i++) {
    psiconv_progress(config,lev+3,off+len,"Going to read element %d",i);
    nr = psiconv_read_u32(config,buf,lev+3,off+len,&res);
    if (res)
       goto ERROR2;
    if ((res = psiconv_list_add(*result,&nr)))
      goto ERROR2;
    len += leng;
  }

  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of sheet grid break list (total length: %08x)", len);
  return 0;

ERROR2:
  psiconv_list_free(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of Sheet Grid break List failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}

