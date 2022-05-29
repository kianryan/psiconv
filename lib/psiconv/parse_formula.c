/*
    parse_formula.c - Part of psiconv, a PSION 5 file formats converter
    Copyright (c) 2001-2014  Frodo Looijaard <frodo@frodo.looijaard.name>

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


struct formula_element
{
  psiconv_formula_type_t formula_type;
  int number_of_args;
  const char* name;
};

static struct formula_element formula_elements[256] =
  {
    {psiconv_formula_unknown,0,"*UNKNOWN*"}, /* 0 */
    {psiconv_formula_op_lt,2,"<"},
    {psiconv_formula_op_le,2,"<="},
    {psiconv_formula_op_gt,2,">"},
    {psiconv_formula_op_ge,2,">="},
    {psiconv_formula_op_ne,2,"<>"},
    {psiconv_formula_op_eq,2,"="},
    {psiconv_formula_op_add,2,"+"},
    {psiconv_formula_op_sub,2,"-"},
    {psiconv_formula_op_mul,2,"*"},
    {psiconv_formula_op_div,2,"/"},
    {psiconv_formula_op_pow,2,"^"},
    {psiconv_formula_op_pos,1,"+"},
    {psiconv_formula_op_neg,1,"-"},
    {psiconv_formula_op_not,1,"NOT"},
    {psiconv_formula_op_and,2,"AND"},
    {psiconv_formula_op_or,2,"OR"},     /* 10 */
    {psiconv_formula_op_con,2,"&"},
    {psiconv_formula_op_bra,1,"()"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_mark_eof,0,"End of formula"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_dat_float,0,"Floating point number"},
    {psiconv_formula_dat_int,0,"Signed integer number"}, /* 20 */
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_dat_var,0,"Named variable"},
    {psiconv_formula_dat_string,0,"String"},
    {psiconv_formula_dat_cellref,0,"Cell reference"},
    {psiconv_formula_dat_cellblock,0,"Cell block"},
    {psiconv_formula_dat_vcellblock,0,"Cell block {varargs}"},
    {psiconv_formula_mark_opsep,0,"Operand separator"},
    {psiconv_formula_mark_opend,0,"Operand list end"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},     /* 30 */
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_fun_false,0,"FALSE"},
    {psiconv_formula_fun_if,3,"IF"},
    {psiconv_formula_fun_true,0,"TRUE"},
    {psiconv_formula_fun_cell,2,"CELL"},
    {psiconv_formula_fun_errortype,0,"ERRORTYPE"},
    {psiconv_formula_fun_isblank,1,"ISBLANK"},
    {psiconv_formula_fun_iserr,1,"ISERR"},
    {psiconv_formula_fun_iserror,1,"ISERROR"},
    {psiconv_formula_fun_islogical,1,"ISLOGICAL"},
    {psiconv_formula_fun_isna,1,"ISNA"},
    {psiconv_formula_fun_isnontext,1,"ISNONTEXT"},
    {psiconv_formula_fun_isnumber,1,"ISNUMBER"},
    {psiconv_formula_fun_istext,1,"ISTEXT"},
    {psiconv_formula_fun_n,1,"N"},  /* 40 */
    {psiconv_formula_fun_type,1,"TYPE"},
    {psiconv_formula_fun_address,2,"ADDRESS"},
    {psiconv_formula_fun_column,1,"COLUMN"},
    {psiconv_formula_fun_columns,1,"COLUMNS"},
    {psiconv_formula_fun_hlookup,3,"HLOOKUP"},
    {psiconv_formula_fun_index,3,"INDEX"},
    {psiconv_formula_fun_indirect,1,"INDIRECT"},
    {psiconv_formula_fun_lookup,3,"LOOKUP"},
    {psiconv_formula_fun_offset,3,"OFFSET"},
    {psiconv_formula_fun_row,1,"ROW"},
    {psiconv_formula_fun_rows,1,"ROWS"},
    {psiconv_formula_fun_vlookup,3,"VLOOKUP"},
    {psiconv_formula_fun_char,1,"CHAR"},
    {psiconv_formula_fun_code,1,"CODE"},
    {psiconv_formula_fun_exact,2,"EXACT"},
    {psiconv_formula_fun_find,3,"FIND"}, /* 50 */
    {psiconv_formula_fun_left,2,"LEFT"},
    {psiconv_formula_fun_len,1,"LEN"},
    {psiconv_formula_fun_lower,1,"LOWER"},
    {psiconv_formula_fun_mid,3,"MID"},
    {psiconv_formula_fun_proper,1,"PROPER"},
    {psiconv_formula_fun_replace,4,"REPLACE"},
    {psiconv_formula_fun_rept,2,"REPT"},
    {psiconv_formula_fun_right,2,"RIGHT"},
    {psiconv_formula_fun_string,2,"STRING"},
    {psiconv_formula_fun_t,1,"T"},
    {psiconv_formula_fun_trim,1,"TRIM"},
    {psiconv_formula_fun_upper,1,"UPPER"},
    {psiconv_formula_fun_value,1,"VALUE"},
    {psiconv_formula_fun_date,3,"DATE"},
    {psiconv_formula_fun_datevalue,1,"DATEVALUE"},
    {psiconv_formula_fun_day,1,"DAY"},  /* 60 */
    {psiconv_formula_fun_hour,1,"HOUR"},
    {psiconv_formula_fun_minute,1,"MINUTE"},
    {psiconv_formula_fun_month,1,"MONTH"},
    {psiconv_formula_fun_now,0,"NOW"},
    {psiconv_formula_fun_second,1,"SECOND"},
    {psiconv_formula_fun_today,0,"TODAY"},
    {psiconv_formula_fun_time,3,"TIME"},
    {psiconv_formula_fun_timevalue,1,"TIMEVALUE"},
    {psiconv_formula_fun_year,1,"YEAR"},
    {psiconv_formula_fun_abs,1,"ABS"},
    {psiconv_formula_fun_acos,1,"ACOS"},
    {psiconv_formula_fun_asin,1,"ASIN"},
    {psiconv_formula_fun_atan,1,"ATAN"},
    {psiconv_formula_fun_atan2,2,"ATAN2"},
    {psiconv_formula_fun_cos,1,"COS"},
    {psiconv_formula_fun_degrees,0,"DEGREES"}, /* 70 */
    {psiconv_formula_fun_exp,1,"EXP"},
    {psiconv_formula_fun_fact,1,"FACT"},
    {psiconv_formula_fun_int,1,"INT"},
    {psiconv_formula_fun_ln,1,"LN"},
    {psiconv_formula_fun_log10,1,"LOG10"},
    {psiconv_formula_fun_mod,2,"MOD"},
    {psiconv_formula_fun_pi,0,"PI"},
    {psiconv_formula_fun_radians,1,"RADIANS"},
    {psiconv_formula_fun_rand,0,"RAND"},
    {psiconv_formula_fun_round,2,"ROUND"},
    {psiconv_formula_fun_sign,1,"SIGN"},
    {psiconv_formula_fun_sin,1,"SIN"},
    {psiconv_formula_fun_sqrt,1,"SQRT"},
    {psiconv_formula_fun_sumproduct,2,"SUMPRODUCT"},
    {psiconv_formula_fun_tan,1,"TAN"},
    {psiconv_formula_fun_trunc,1,"TRUNC"}, /* 80 */
    {psiconv_formula_fun_cterm,3,"CTERM"},
    {psiconv_formula_fun_ddb,4,"DDB"},
    {psiconv_formula_fun_fv,3,"FV"},
    {psiconv_formula_fun_irr,2,"IRR"},
    {psiconv_formula_fun_npv,2,"NPV"},
    {psiconv_formula_fun_pmt,3,"PMT"},
    {psiconv_formula_fun_pv,3,"PV"},
    {psiconv_formula_fun_rate,3,"RATE"},
    {psiconv_formula_fun_sln,3,"SLN"},
    {psiconv_formula_fun_syd,4,"SYD"},
    {psiconv_formula_fun_term,3,"TERM"},
    {psiconv_formula_fun_combin,2,"COMBIN"},
    {psiconv_formula_fun_permut,2,"PERMUT"},
    {psiconv_formula_vfn_average,-1,"AVERAGE"},
    {psiconv_formula_vfn_choose,-1,"CHOOSE"},
    {psiconv_formula_vfn_count,-1,"COUNT"},  /* 90 */
    {psiconv_formula_vfn_counta,-1,"COUNTA"},
    {psiconv_formula_vfn_countblank,-1,"COUNTBLANK"},
    {psiconv_formula_vfn_max,-1,"MAX"},
    {psiconv_formula_vfn_min,-1,"MIN"},
    {psiconv_formula_vfn_product,-1,"PRODUCT"},
    {psiconv_formula_vfn_stdevp,-1,"STDEVP"},
    {psiconv_formula_vfn_stdev,-1,"STDEV"},
    {psiconv_formula_vfn_sum,-1,"SUM"},
    {psiconv_formula_vfn_sumsq,-1,"SUMSQ"},
    {psiconv_formula_vfn_varp,-1,"VARP"},
    {psiconv_formula_vfn_var,-1,"VAR"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"}, /* A0 */
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"}, /* B0 */
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"}, /* C0 */
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"}, /* D0 */
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"}, /* E0 */
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"}, /* F0 */
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"},
    {psiconv_formula_unknown,0,"*UNKNOWN*"}};

static int psiconv_parse_sheet_ref(const psiconv_config config,
                                   const psiconv_buffer buf,int lev,
                                   psiconv_u32 off, int *length,
                                   psiconv_sheet_ref_t *result)
{
  int res;
  psiconv_u16 temp;

  psiconv_progress(config,lev+1,off,"Going to read a sheet ref");
  psiconv_progress(config,lev+2,off,"Going to read the offset encoding");
  temp = psiconv_read_u16(config,buf,lev+2,off,&res);
  if (res) {
    if (length)
      *length = 0;
    return res;
  }
  psiconv_debug(config,lev+2,off,"Encoded word: %04x",temp);
  result->absolute = (temp & 0x4000)?psiconv_bool_true:psiconv_bool_false;
  result->offset = (temp & 0x3fff) * ((temp & 0x8000)?-1:1);
  psiconv_debug(config,lev+2,off,"Reference: %s offset %d",
                result->absolute?"absolute":"relative",result->offset);
  if (length)
    *length = 2;
  return 0;
}

static int psiconv_parse_sheet_cell_reference(const psiconv_config config,
                                         const psiconv_buffer buf,int lev,
                                         psiconv_u32 off, int *length,
                                         psiconv_sheet_cell_reference_t *result)
{
  int len = 0;
  int leng,res;
  psiconv_u8 temp;

  psiconv_progress(config,lev+1,off+len,"Going to read a sheet cell reference");
  psiconv_progress(config,lev+2,off+len,"Going to read the row reference");
  if ((res = psiconv_parse_sheet_ref(config,buf,lev+2,off+len,&leng,&result->row)))
    goto ERROR;
  len += leng;
  psiconv_progress(config,lev+2,off+len,"Going to read the column reference");
  if ((res = psiconv_parse_sheet_ref(config,buf,lev+2,off+len,&leng,&result->column)))
    goto ERROR;
  len += leng;

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the trailing byte (%02x expected)",0);
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR;
  if (temp != 0) {
    psiconv_warn(config,lev+2,off+len,"Unknown byte in cell reference (ignored");
    psiconv_debug(config,lev+2,off+len,"Trailing byte: %02x",temp);
  }
  len ++;
  psiconv_progress(config,lev,off+len-1,
                   "End of cell reference (total length: %08x)", len);
  if (length)
    *length = len;
  return 0;
ERROR:
  if (length)
    *length = 0;
  return res;
}

static int psiconv_parse_sheet_cell_block(const psiconv_config config,
                                         const psiconv_buffer buf,int lev,
                                         psiconv_u32 off, int *length,
                                         psiconv_sheet_cell_block_t *result)
{
  int len = 0;
  int leng,res;

  psiconv_progress(config,lev+1,off+len,"Going to read a sheet cell block");
  psiconv_progress(config,lev+2,off+len,"Going to read the first cell");
  if ((res = psiconv_parse_sheet_cell_reference(config,buf,lev+2,off+len,&leng,
                                                &result->first)))
    goto ERROR;
  len += leng;
  psiconv_progress(config,lev+2,off+len,"Going to read the last cell");
  if ((res = psiconv_parse_sheet_cell_reference(config,buf,lev+2,off+len,&leng,
                                                &result->last)))
    goto ERROR;
  len += leng;
  psiconv_progress(config,lev,off+len-1,
                   "End of cell block (total length: %08x)", len);
  if (length)
    *length = len;
  return 0;
ERROR:
  if (length)
    *length = 0;
  return res;
}

static int psiconv_parse_formula_element_list(const psiconv_config config,
                                       const psiconv_buffer buf, int lev,
                                       psiconv_u32 off, int *length, 
                                       psiconv_formula *result, 
                                       psiconv_u32 maxlen)
{
  int res=0;
  int len=0;
  int leng;
  int eof = 0;
  psiconv_u8 marker,submarker,submarker2;
  psiconv_formula_list formula_stack;
  psiconv_formula formula,subformula,subformula1,subformula2,
                   subformula3,subformula4;
  psiconv_u16 temp,nr_of_subs;

  psiconv_progress(config,lev+1,off,"Going to read a formula element list");
  if (!(*result = malloc(sizeof(**result))))
    goto ERROR1;
  if (!(formula_stack = psiconv_list_new(sizeof(struct psiconv_formula_s))))
    goto ERROR2;
  if (!(formula = malloc(sizeof(*formula))))
    goto ERROR3;
  /* By setting the type to unknown, we can safely call psiconv_free_formula */
  formula->type = psiconv_formula_unknown;
  if (!(subformula1 = malloc(sizeof(*subformula1))))
    goto ERROR4;
  subformula1->type = psiconv_formula_unknown;
  if (!(subformula2 = malloc(sizeof(*subformula2))))
    goto ERROR5;
  subformula2->type = psiconv_formula_unknown;
  if (!(subformula3 = malloc(sizeof(*subformula3))))
    goto ERROR6;
  subformula3->type = psiconv_formula_unknown;
  if (!(subformula4 = malloc(sizeof(*subformula4))))
    goto ERROR7;
  subformula4->type = psiconv_formula_unknown;

  while (!eof && len+off < maxlen) {
    psiconv_progress(config,lev+3,off+len,"Going to read a formula item marker");
    marker = psiconv_read_u8(config,buf,lev+2,off+len,&res);
    if (res)
      goto ERROR8;
    psiconv_debug(config,lev+3,off+len,"Marker: %02x (%s)",marker,
                  formula_elements[marker].name);
    len ++;

    if (formula_elements[marker].formula_type == psiconv_formula_unknown) {
      psiconv_error(config,lev+3,off+len,"Unknown formula marker found!");
      goto ERROR8;
    } else if ((formula_elements[marker].formula_type == 
                                                psiconv_formula_mark_eof) ||
               (formula_elements[marker].formula_type == 
                                                psiconv_formula_mark_opend) ||
               (formula_elements[marker].formula_type == 
                                                psiconv_formula_mark_opsep)) {
      len--;
      psiconv_progress(config,lev+3,off+len,"End of this formula list");
      eof = 1;
    } else if (formula_elements[marker].formula_type == 
                                                 psiconv_formula_dat_int) {
      psiconv_progress(config,lev+3,off+len,"Next item: an integer");
      formula->data.dat_int = psiconv_read_u32(config,buf,lev+2,off+len,&res);
      if (res)
        goto ERROR8;
      formula->type = formula_elements[marker].formula_type;
      psiconv_debug(config,lev+3,off+len,"Value: %08x",formula->data.dat_int);
      len += 4;
      if ((res = psiconv_list_add(formula_stack,formula)))
        goto ERROR8;
      formula->type = psiconv_formula_unknown;
    } else if  (formula_elements[marker].formula_type ==
                                                 psiconv_formula_dat_float) {
      psiconv_progress(config,lev+3,off+len,"Next item: a float");
      formula->data.dat_float = psiconv_read_float(config,buf,lev+2,off+len,&leng,
                                                   &res);
      if (res)
        goto ERROR8;
      formula->type = formula_elements[marker].formula_type;
      psiconv_debug(config,lev+3,off+len,"Value: %f",formula->data.dat_float);
      len += leng;
      if ((res = psiconv_list_add(formula_stack,formula)))
        goto ERROR8;
      formula->type = psiconv_formula_unknown;
    } else if  (formula_elements[marker].formula_type ==
                                                 psiconv_formula_dat_cellref) {
      psiconv_progress(config,lev+3,off+len,"Next item: a cell reference");
      if ((res = psiconv_parse_sheet_cell_reference(config,buf,lev+2,off+len,&leng,
                                                  &formula->data.dat_cellref)))
        goto ERROR8;
      formula->type = formula_elements[marker].formula_type;
      len += leng;
      if ((res = psiconv_list_add(formula_stack,formula)))
        goto ERROR8;
      formula->type = psiconv_formula_unknown;
    } else if ((formula_elements[marker].formula_type ==
                                            psiconv_formula_dat_cellblock) || 
               (formula_elements[marker].formula_type ==
                                            psiconv_formula_dat_vcellblock)) {
      psiconv_progress(config,lev+3,off+len,"Next item: a cell block");
      if ((res = psiconv_parse_sheet_cell_block(config,buf,lev+2,off+len,&leng,
                                                &formula->data.dat_cellblock)))
        goto ERROR8;
      formula->type = formula_elements[marker].formula_type;
      len += leng;
      if ((res = psiconv_list_add(formula_stack,formula)))
        goto ERROR8;
      formula->type = psiconv_formula_unknown;
    } else if  (formula_elements[marker].formula_type ==
                                               psiconv_formula_dat_string) {
      psiconv_progress(config,lev+3,off+len,"Next item: a string");
      formula->data.dat_string = 
                 psiconv_read_short_string(config,buf,lev+2,off+len,&leng,&res);
      if (res)
        goto ERROR8;
      formula->type = formula_elements[marker].formula_type;
      len += leng;
      if ((res = psiconv_list_add(formula_stack,formula)))
        goto ERROR8;
      formula->type = psiconv_formula_unknown;
    } else if ((formula_elements[marker].formula_type ==
                                              psiconv_formula_dat_var)) {
      psiconv_progress(config,lev+3,off+len,"Next item: a variable reference");
      formula->data.dat_variable = psiconv_read_u32(config,buf,lev+2,off+len,&res);
      if (res)
        goto ERROR8;
      formula->type = formula_elements[marker].formula_type;
      len += 4;
      if ((res = psiconv_list_add(formula_stack,formula)))
        goto ERROR8;
      formula->type = psiconv_formula_unknown;
    } else if (formula_elements[marker].number_of_args == -1) {
      psiconv_progress(config,lev+3,off+len,"Going to parse a vararg function");
      if (!(formula->data.fun_operands = 
                      psiconv_list_new(sizeof(*formula))))
          goto ERROR8;
      formula->type = formula_elements[marker].formula_type;
      nr_of_subs = 0;
      do {
        nr_of_subs ++;
        psiconv_progress(config,lev+4,off+len,"Going to read vararg argument %d",
                         nr_of_subs);
        if ((res = psiconv_parse_formula_element_list(config,buf,lev+4,off+len,&leng,
                                                      &subformula,maxlen)))
          goto ERROR8;
        len += leng;
        if ((res = psiconv_list_add(formula->data.fun_operands,subformula))) {
          psiconv_free_formula(subformula);
          goto ERROR8;
        }
        free(subformula);
        psiconv_progress(config,lev+4,off+len,"Going to read the next marker");
        submarker = psiconv_read_u8(config,buf,lev+4,off+len,&res);
        len ++;
        if (res)
          goto ERROR8;
        submarker2 = psiconv_read_u8(config,buf,lev+4,off+len,&res);
        if (res)
          goto ERROR8;
      } while ((formula_elements[submarker].formula_type 
                                         == psiconv_formula_mark_opsep) && 
               (formula_elements[submarker2].formula_type
                                         != psiconv_formula_mark_opend));
      if ((formula_elements[submarker].formula_type == 
                                          psiconv_formula_mark_opsep) &&
          (formula_elements[submarker2].formula_type == 
                                          psiconv_formula_mark_opend)) {
        submarker=submarker2;
        len++;
      }
      if (formula_elements[submarker].formula_type 
                                             != psiconv_formula_mark_opend) {
        psiconv_error(config,lev+3,off+len,"Formula corrupted!");
        psiconv_debug(config,lev+3,off+len,"Found unexpected marker %02x",submarker);
        goto ERROR8;
      }
      psiconv_progress(config,lev+3,off+len,"Going to read the repeated marker %02x",
                       marker);
      submarker = psiconv_read_u8(config,buf,lev+3,off+len,&res);
      if (res)
        goto ERROR8;
      if (submarker != marker) {
        psiconv_error(config,lev+3,off+len,"Formula corrupted!");
        psiconv_debug(config,lev+3,off+len,"Expected marker %02x, found %02x",
                      marker,submarker);
        goto ERROR8;
      }
      len++;
      psiconv_progress(config,lev+3,off+len,
                       "Going to read the number of arguments (%d expected)",
                       nr_of_subs);
      temp = psiconv_read_u16(config,buf,lev+3,off+len,&res);
      if (res)
        goto ERROR8;
      if (temp != nr_of_subs) {
        psiconv_error(config,lev+3,off+len,"Formula corrupted!");
        psiconv_debug(config,lev+3,off+len,
                      "Read %d arguments, but formula says there are %d",
                      nr_of_subs,temp);
        goto ERROR8;
      }
      len += 2;
      if ((res = psiconv_list_add(formula_stack,formula)))
        goto ERROR8;
      formula->type = psiconv_formula_unknown;
    } else {
      if (formula_elements[marker].number_of_args > 0) 
        if ((res = psiconv_list_pop(formula_stack,subformula1)))
          goto ERROR8;
      if (formula_elements[marker].number_of_args > 1) 
        if ((res = psiconv_list_pop(formula_stack,subformula2)))
          goto ERROR8;
      if (formula_elements[marker].number_of_args > 2) 
        if ((res = psiconv_list_pop(formula_stack,subformula3)))
          goto ERROR8;
      if (formula_elements[marker].number_of_args > 3) 
        if ((res = psiconv_list_pop(formula_stack,subformula4)))
          goto ERROR8;
      if (!(formula->data.fun_operands = 
                      psiconv_list_new(sizeof(*formula))))
          goto ERROR8;
       formula->type = formula_elements[marker].formula_type;
      if (formula_elements[marker].number_of_args > 3) 
        if ((res = psiconv_list_add(formula->data.fun_operands,subformula4)))
          goto ERROR8;
      if (formula_elements[marker].number_of_args > 2) 
        if ((res = psiconv_list_add(formula->data.fun_operands,subformula3)))
          goto ERROR8;
      if (formula_elements[marker].number_of_args > 1) 
        if ((res = psiconv_list_add(formula->data.fun_operands,subformula2)))
          goto ERROR8;
      if (formula_elements[marker].number_of_args > 0) 
        if ((res = psiconv_list_add(formula->data.fun_operands,subformula1)))
          goto ERROR8;
      if ((res = psiconv_list_add(formula_stack,formula)))
        goto ERROR8;
      subformula4->type = subformula3->type = subformula2->type =
                 subformula1->type = formula->type = psiconv_formula_unknown;
    }
  }
  if ((len+off > maxlen) || !eof) {
    psiconv_error(config,lev+2,off+len,"Formula corrupted!");
    psiconv_debug(config,lev+2,off+len,"Expected end: %04x, found end: %04x",
                  maxlen,len+off);
    goto ERROR8;
  }
  if ((psiconv_list_length(formula_stack)) != 1) {
    psiconv_error(config,lev+2,off+len,"Formula corrupted!");
    psiconv_debug(config,lev+2,off+len,"More than one item left on the stack (%d)",
                   psiconv_list_length(formula_stack));
    goto ERROR8;
  }
  if ((res = psiconv_list_pop(formula_stack,*result)))
    goto ERROR8;
  psiconv_list_free(formula_stack);
  free(formula);
  
  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of formula element list (total length: %08x)", len);
  return 0;

ERROR8:
  psiconv_free_formula(subformula4);
ERROR7:
  psiconv_free_formula(subformula3);
ERROR6:
  psiconv_free_formula(subformula2);
ERROR5:
  psiconv_free_formula(subformula1);
ERROR4:
  psiconv_free_formula(formula);
ERROR3:
  psiconv_free_formula_list(formula_stack);
ERROR2:
  free (*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of formula element list failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}




int psiconv_parse_formula(const psiconv_config config,
                          const psiconv_buffer buf, int lev,
                          psiconv_u32 off, int *length, 
                          psiconv_formula *result)
{
  int res=0;
  int len=0;
  int leng;
  psiconv_u32 bytelen,formula_end;
  psiconv_u8 temp;

  psiconv_progress(config,lev+1,off,"Going to read a formula");

  psiconv_progress(config,lev+2,off+len,
                   "Going to read the formula byte length");
  bytelen = psiconv_read_S(config,buf,lev+2,off+len,&leng,&res);
  if (res)
    goto ERROR1;
  psiconv_debug(config,lev+2,off+len,"Formula byte length: %d",bytelen);
  len += leng;
  bytelen += len;
  formula_end = off + bytelen;

  psiconv_progress(config,lev+2,off+len,"Going to read the formula elements list");
  if ((res = psiconv_parse_formula_element_list(config,buf,lev+2,off+len,&leng,
                                                result,formula_end)))
    goto ERROR1;
  len += leng;

  psiconv_progress(config,lev+2,off+len,"Going to read the eof marker");
  temp = psiconv_read_u8(config,buf,lev+2,off+len,&res);
  if (res)
    goto ERROR2;
  if (formula_elements[temp].formula_type != psiconv_formula_mark_eof) {
    psiconv_error(config,lev+2,off+len,"Formula corrupted!");
    psiconv_debug(config,lev+2,off+len,"Expected marker: %02x, found byte: %02x",
                  0x15,temp);
    goto ERROR2;
  }
  len ++;

  if (off+len != formula_end) {
    psiconv_error(config,lev+2,off+len,"Formula corrupted!");
    psiconv_debug(config,lev+2,off+len,"Expected end: %04x, found end: %04x",
                  formula_end,len+off);
    goto ERROR2;
  }
  
  if (length)
    *length = len;

  psiconv_progress(config,lev,off+len-1,
                   "End of formula (total length: %08x)", len);
  return 0;

ERROR2:
  psiconv_free_formula(*result);
ERROR1:
  psiconv_error(config,lev+1,off,"Reading of formula failed");
  if (length)
    *length = 0;
   if (!res)
     return -PSICONV_E_NOMEM;
   else
    return res;
}


