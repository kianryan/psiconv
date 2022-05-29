/*
    data.c - Part of psiconv, a PSION 5 file formats converter
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
#include "data.h"
#include "list.h"
#include "unicode.h"
#include "error.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static psiconv_color clone_color(psiconv_color color);
static psiconv_font clone_font(psiconv_font font);
static psiconv_border clone_border(psiconv_border border);
static psiconv_bullet clone_bullet(psiconv_bullet bullet);
static psiconv_all_tabs clone_all_tabs(psiconv_all_tabs all_tabs);
static void psiconv_free_style_aux(void *style);
static void psiconv_free_in_line_layout_aux(void * layout);
static void psiconv_free_paragraph_aux(void * paragraph);
static void psiconv_free_paint_data_section_aux(void * section);
static void psiconv_free_clipart_section_aux(void * section);
static void psiconv_free_formula_aux(void *data);
static void psiconv_free_sheet_worksheet_aux (void *data);
static void psiconv_free_sheet_variable_aux(void * variable);
static void psiconv_free_sheet_cell_aux(void *cell);
static void psiconv_free_sheet_line_aux(void *line);
static void psiconv_free_sheet_worksheet_aux (void *data);

static psiconv_word_styles_section psiconv_empty_word_styles_section(void);
static psiconv_text_and_layout psiconv_empty_text_and_layout(void);
static psiconv_texted_section psiconv_empty_texted_section(void);
static psiconv_page_header psiconv_empty_page_header(void);
static psiconv_page_layout_section psiconv_empty_page_layout_section(void);
static psiconv_word_status_section psiconv_empty_word_status_section(void);
static psiconv_word_f psiconv_empty_word_f(void);
static psiconv_sheet_status_section psiconv_empty_sheet_status_section(void);
static psiconv_formula_list psiconv_empty_formula_list(void);
static psiconv_sheet_workbook_section 
                                  psiconv_empty_sheet_workbook_section(void);
static psiconv_sheet_f psiconv_empty_sheet_f(void);
static psiconv_texted_f psiconv_empty_texted_f(void);
static psiconv_paint_data_section psiconv_empty_paint_data_section(void);
static psiconv_pictures psiconv_empty_pictures(void);
static psiconv_mbm_f psiconv_empty_mbm_f(void);
static psiconv_sketch_section psiconv_empty_sketch_section(void);
static psiconv_sketch_f psiconv_empty_sketch_f(void);
static psiconv_clipart_f psiconv_empty_clipart_f(void);
static psiconv_cliparts psiconv_empty_cliparts(void);


/* Note: these defaults seem to be hard-coded somewhere outside the
   files themself. */
psiconv_character_layout psiconv_basic_character_layout(void)
{
  /* Make the structures static, to oblige IRIX */
  static struct psiconv_color_s black = 
  {
    0x00,                  /* red */
    0x00,                  /* green */
    0x00,                  /* blue */
  };
  static struct psiconv_color_s white = 
  {
    0xff,                  /* red */
    0xff,                  /* green */
    0xff,                  /* blue */
  };
  static psiconv_ucs2 font_times[16] = { 'T','i','m','e','s',' ',
                                         'N','e','w',' ',
					 'R','o','m','a','n',0 };
  static struct psiconv_font_s font =
  {
    font_times,            /* name */
    3                      /* screenfont */
  };
  struct psiconv_character_layout_s cl = 
  {
    &black,                /* color               */
    &white,                /* back_color          */
    10.0,                  /* font_size           */
    psiconv_bool_false,    /* italic              */
    psiconv_bool_false,    /* bold                */
    psiconv_normalscript,  /* super_sub           */
    psiconv_bool_false,    /* underline           */
    psiconv_bool_false,    /* strikethrough       */
    &font,                  /* font                */
  };

  return psiconv_clone_character_layout(&cl);
}

/* Note: these defaults seem to be hard-coded somewhere outside the
   files themself. */
psiconv_paragraph_layout psiconv_basic_paragraph_layout(void)
{
  static psiconv_ucs2 font_times[16] = { 'T','i','m','e','s',' ',
                                         'N','e','w',' ',
					 'R','o','m','a','n',0 };

  static struct psiconv_font_s font =
  {
    font_times,           /* name */
    2                      /* screenfont */
  };
  static struct psiconv_color_s black = 
  {
    0x00,                  /* red */
    0x00,                  /* green */
    0x00,                  /* blue */
  };
  static struct psiconv_color_s white = 
  {
    0xff,                  /* red */
    0xff,                  /* green */
    0xff,                  /* blue */
  };
  static struct psiconv_border_s no_border =
  {
    psiconv_border_none,   /* kind                */
    1,                     /* thickness           */
    &black                 /* color               */
  };
  static struct psiconv_bullet_s bullet =
  {
    psiconv_bool_false,    /* on                  */
    10.0,                  /* font_size           */
    0x201d,                /* character           */
    psiconv_bool_true,     /* indent              */
    &black,                /* color               */
    &font,                 /* font                */
  };
  static struct psiconv_all_tabs_s tabs = 
  {
    0.64,                  /* normal              */
    NULL                   /* kind                */
  };
  struct psiconv_paragraph_layout_s pl =
  {
    &white,                /* back_color          */
    0.0,                   /* indent_left         */
    0.0,                   /* indent_right        */
    0.0,                   /* indent_first        */
    psiconv_justify_left,  /* justify_hor         */
    psiconv_justify_middle,/* justify_ver         */
    10.0,                  /* linespacing         */
    psiconv_bool_false,    /* linespacing_exact   */
    0.0,                   /* space_above         */
    0.0,                   /* space_below         */
    psiconv_bool_false,    /* keep_together       */
    psiconv_bool_false,    /* keep_with_next      */
    psiconv_bool_false,    /* on_next_page        */
    psiconv_bool_false,    /* no_widow_protection */
    psiconv_bool_false,    /* wrap_to_fit_cell    */
    0.0,                   /* left_margin         */
    &bullet,               /* bullet              */
    &no_border,            /* left_border         */
    &no_border,            /* right_border        */
    &no_border,            /* top_border          */
    &no_border,            /* bottom_border       */
    &tabs,                 /* tabs                */
  };
  psiconv_paragraph_layout res;
  
  if (!(pl.tabs->extras = psiconv_list_new(sizeof(struct psiconv_tab_s))))
    return NULL;
  res = psiconv_clone_paragraph_layout(&pl);
  psiconv_list_free(pl.tabs->extras);
  return res;
}
  
psiconv_color clone_color(psiconv_color color)
{
  psiconv_color result;
  if (!(result = malloc(sizeof(*result))))
    return NULL;
  *result = *color;
  return result;
}
  
psiconv_font clone_font(psiconv_font font)
{
  psiconv_font result;
  if(!(result = malloc(sizeof(*result))))
    goto ERROR1;
  *result = *font;
  if (!(result->name = psiconv_unicode_strdup(result->name)))
    goto ERROR2;
  return result;
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_border clone_border(psiconv_border border)
{
  psiconv_border result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  *result = *border;
  if(!(result->color = clone_color(result->color)))
    goto ERROR2;
  return result;
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_bullet clone_bullet(psiconv_bullet bullet)
{
  psiconv_bullet result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  *result = *bullet;
  if (!(result->font = clone_font(result->font)))
    goto ERROR2;
  if (!(result->color = clone_color(result->color)))
    goto ERROR3;
  return result;
ERROR3:
  psiconv_free_font(result->font);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_all_tabs clone_all_tabs(psiconv_all_tabs all_tabs)
{
  psiconv_all_tabs result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  *result = *all_tabs;
  if (!(result->extras = psiconv_list_clone(result->extras)))
    goto ERROR2;
  return result;
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_character_layout psiconv_clone_character_layout
                                           (psiconv_character_layout ls)
{
  psiconv_character_layout result;

  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  *result = *ls;
  if (!(result->color = clone_color(result->color)))
    goto ERROR2;
  if (!(result->back_color = clone_color(result->back_color)))
    goto ERROR3;
  if (!(result->font = clone_font(result->font)))
    goto ERROR4;
  return result;
ERROR4:
  psiconv_free_color(result->back_color);
ERROR3:
  psiconv_free_color(result->color);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_paragraph_layout psiconv_clone_paragraph_layout
                                          (psiconv_paragraph_layout ls)
{
  psiconv_paragraph_layout result;

  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  *result = *ls;
  if (!(result->back_color = clone_color(result->back_color)))
    goto ERROR2;
  if (!(result->bullet = clone_bullet(result->bullet)))
    goto ERROR3;
  if (!(result->left_border = clone_border(result->left_border)))
    goto ERROR4;
  if (!(result->right_border = clone_border(result->right_border)))
    goto ERROR5;
  if (!(result->top_border = clone_border(result->top_border)))
    goto ERROR6;
  if (!(result->bottom_border = clone_border(result->bottom_border)))
    goto ERROR7;
  if (!(result->tabs = clone_all_tabs(result->tabs)))
    goto ERROR8;
  return result;
ERROR8:
  psiconv_free_border(result->bottom_border);
ERROR7:
  psiconv_free_border(result->top_border);
ERROR6:
  psiconv_free_border(result->right_border);
ERROR5:
  psiconv_free_border(result->left_border);
ERROR4:
  psiconv_free_bullet(result->bullet);
ERROR3:
  psiconv_free_color(result->back_color);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_word_style psiconv_get_style (psiconv_word_styles_section ss, int nr)
{
  if (nr == 0)
    return ss->normal;
  else
    return psiconv_list_get(ss->styles,0xff - nr);
}

int psiconv_find_style(const psiconv_word_styles_section ss,
                       const psiconv_ucs2 *name,
                       int *nr)
{
  const psiconv_ucs2 value_normal[] = { 'N','o','r','m','a','l',0 };
  psiconv_word_style style;
  int i;

  if (!nr)
    return PSICONV_E_OTHER;
  if(!psiconv_unicode_strcmp(value_normal,name)) {
    *nr = 0;
    return 0;
  }
  for (i = 0; i < psiconv_list_length(ss->styles);i++) {
    if (!(style = psiconv_list_get(ss->styles,i)))
      return PSICONV_E_NOMEM;
    if (!psiconv_unicode_strcmp(style->name,name)) {
      *nr = 0xff - i;
      return 0;
    }
  }
  *nr = 0;
  return PSICONV_E_OTHER;
}


psiconv_formula psiconv_get_formula (psiconv_formula_list ss, int nr)
{
  return psiconv_list_get(ss,psiconv_list_length(ss)-nr-1);
}

/* TODO: What if a cell is both in a default row and a default column?!? */
psiconv_sheet_cell_layout psiconv_get_default_layout 
                                       (psiconv_sheet_line_list row_defaults,
                                        psiconv_sheet_line_list col_defaults,
                                        psiconv_sheet_cell_layout cell_default,
                                        int row,int col)
{
  int i;
  psiconv_sheet_line line;
  for (i = 0;i < psiconv_list_length(row_defaults);i++) {
    line = psiconv_list_get(row_defaults,i);
    if (line->position == row)
      return line->layout;
  }
  for (i = 0;i < psiconv_list_length(col_defaults);i++) {
    line = psiconv_list_get(col_defaults,i);
    if (line->position == col)
      return line->layout;
  }
  return cell_default;
}


void psiconv_free_color (psiconv_color color)
{
  if (color)
    free(color);
}

void psiconv_free_border(psiconv_border border)
{
  if (border) {
    psiconv_free_color(border->color);
    free(border);
  }
}

void psiconv_free_font(psiconv_font font)
{
  if (font) {
    if (font->name) 
      free(font->name);
    free(font);
  }
}

void psiconv_free_bullet(psiconv_bullet bullet)
{
  if (bullet) {
    psiconv_free_color(bullet->color);
    psiconv_free_font(bullet->font);
    free(bullet);
  }
}

void psiconv_free_character_layout(psiconv_character_layout layout)
{
  if (layout) {
    psiconv_free_color(layout->color);
    psiconv_free_color(layout->back_color);
    psiconv_free_font(layout->font);
    free(layout);
  }
}

void psiconv_free_tab(psiconv_tab tab)
{
  if (tab)
    free(tab);
}

void psiconv_free_tabs(psiconv_all_tabs tabs)
{
  if (tabs) {
    psiconv_list_free(tabs->extras);
    free(tabs);
  }
}

void psiconv_free_paragraph_layout(psiconv_paragraph_layout layout)
{
  if (layout) {
    psiconv_free_color(layout->back_color);
    psiconv_free_bullet(layout->bullet);
    psiconv_free_border(layout->left_border);
    psiconv_free_border(layout->right_border);
    psiconv_free_border(layout->top_border);
    psiconv_free_border(layout->bottom_border);
    psiconv_free_tabs(layout->tabs);
    free(layout);
  }
}

void psiconv_free_style_aux(void *style)
{
  if(((psiconv_word_style) style)->name)
    free(((psiconv_word_style) style)->name);
  psiconv_free_character_layout(((psiconv_word_style) style)->character);
  psiconv_free_paragraph_layout(((psiconv_word_style) style)->paragraph);
}

void psiconv_free_word_style(psiconv_word_style style)
{
  if (style) {
    psiconv_free_style_aux(style);
    free(style);
  }
}

void psiconv_free_word_style_list(psiconv_word_style_list style_list)
{
  if (style_list)
    psiconv_list_free_el(style_list,psiconv_free_style_aux);
}

void psiconv_free_word_styles_section(psiconv_word_styles_section styles)
{
  if (styles) {
    psiconv_free_word_style(styles->normal);
    psiconv_free_word_style_list(styles->styles);
    free(styles);
  }
}

void psiconv_free_header_section(psiconv_header_section header)
{
  if (header)
    free(header);
}

void psiconv_free_section_table_entry(psiconv_section_table_entry entry)
{
  if (entry)
    free(entry);
}

void psiconv_free_section_table_section(psiconv_section_table_section section)
{
  if (section) 
    psiconv_list_free(section);
}

void psiconv_free_application_id_section(psiconv_application_id_section section)
{
  if (section) {
    if (section->name)
      free(section->name);
    free(section);
  }
}

void psiconv_free_object_icon_section(psiconv_object_icon_section section)
{
  if (section) {
    if (section->icon_name)
      free(section->icon_name);
    free(section);
  }
}

void psiconv_free_object_display_section(psiconv_object_display_section section)
{
  if (section)
    free(section);
}

void psiconv_free_embedded_object_section
                                     (psiconv_embedded_object_section object)
{
  if (object) {
    psiconv_free_object_icon_section(object->icon);
    psiconv_free_object_display_section(object->display);
    psiconv_free_file(object->object);
    free(object);
  }
}

void psiconv_free_in_line_layout_aux(void * layout)
{
  psiconv_free_character_layout(((psiconv_in_line_layout) layout)->layout);
  psiconv_free_embedded_object_section
                                (((psiconv_in_line_layout) layout)->object);
}

void psiconv_free_in_line_layout(psiconv_in_line_layout layout)
{
  if (layout) {
    psiconv_free_in_line_layout_aux(layout);
    free(layout);
  }
}

void psiconv_free_in_line_layouts(psiconv_in_line_layouts layouts)
{
  if (layouts) 
    psiconv_list_free_el(layouts,&psiconv_free_in_line_layout_aux);
}

void psiconv_free_replacement(psiconv_replacement replacement)
{
  if (replacement) 
    free(replacement);
}

void psiconv_free_replacements(psiconv_replacements replacements)
{
  if (replacements) 
    psiconv_list_free(replacements);
}

void psiconv_free_paragraph_aux(void * paragraph)
{
  if(((psiconv_paragraph) paragraph)->text)
    free(((psiconv_paragraph) paragraph)->text);
  psiconv_free_character_layout(((psiconv_paragraph) paragraph)
                                                       ->base_character);
  psiconv_free_paragraph_layout(((psiconv_paragraph) paragraph)
                                                       ->base_paragraph);
  psiconv_free_in_line_layouts(((psiconv_paragraph) paragraph)
                                                       ->in_lines);
  psiconv_free_replacements(((psiconv_paragraph) paragraph)
                                                       ->replacements);
}

void psiconv_free_paragraph(psiconv_paragraph paragraph)
{
  if (paragraph) {
    psiconv_free_paragraph_aux(paragraph);
    free(paragraph);
  }
}

void psiconv_free_text_and_layout(psiconv_text_and_layout text)
{
  if (text)
    psiconv_list_free_el(text,&psiconv_free_paragraph_aux);
}

void psiconv_free_texted_section(psiconv_texted_section section)
{
  if (section) {
    psiconv_free_text_and_layout(section->paragraphs);
    free(section);
  }
}

void psiconv_free_page_header(psiconv_page_header header)
{
  if (header) {
    psiconv_free_character_layout(header->base_character_layout);
    psiconv_free_paragraph_layout(header->base_paragraph_layout);
    psiconv_free_texted_section(header->text);
    free(header);
  }
}

void psiconv_free_page_layout_section(psiconv_page_layout_section section)
{
  if (section) {
    psiconv_free_page_header(section->header);
    psiconv_free_page_header(section->footer);
    free(section);
  }
}

void psiconv_free_word_status_section(psiconv_word_status_section section)
{
  if (section) 
    free(section);
}

void psiconv_free_word_f(psiconv_word_f file)
{
  if (file) { 
    psiconv_free_page_layout_section(file->page_sec);
    psiconv_free_text_and_layout(file->paragraphs);
    psiconv_free_word_status_section(file->status_sec);
    psiconv_free_word_styles_section(file->styles_sec);
    free(file);
  }
}

void psiconv_free_sheet_status_section(psiconv_sheet_status_section section)
{
  if (section) 
    free(section);
}

void psiconv_free_sheet_numberformat(psiconv_sheet_numberformat numberformat)
{
  if (numberformat)
    free(numberformat);
}

void psiconv_free_sheet_cell_layout(psiconv_sheet_cell_layout layout)
{
  psiconv_free_paragraph_layout(layout->paragraph);
  psiconv_free_character_layout(layout->character);
  psiconv_free_sheet_numberformat(layout->numberformat);
}

void psiconv_free_sheet_cell_aux(void *cell)
{
  psiconv_sheet_cell data = cell;

  psiconv_free_sheet_cell_layout(data->layout);

  if ((data->type == psiconv_cell_string) && (data->data.dat_string))
    free(data->data.dat_string);
}

void psiconv_free_sheet_cell(psiconv_sheet_cell cell)
{
  if (cell) {
    psiconv_free_sheet_cell_aux(cell);
    free(cell);
  }
}

void psiconv_free_sheet_cell_list(psiconv_sheet_cell_list list)
{
  if (list) 
    psiconv_list_free_el(list,psiconv_free_sheet_cell_aux);
}

void psiconv_free_sheet_line_aux(void *line)
{
  psiconv_sheet_line data = line;

  psiconv_free_sheet_cell_layout(data->layout);
}

void psiconv_free_sheet_line(psiconv_sheet_line line)
{
  if (line) {
    psiconv_free_sheet_line_aux(line);
    free(line);
  }
}


void psiconv_free_sheet_line_list(psiconv_sheet_line_list list)
{
  if (list) 
    psiconv_list_free_el(list,psiconv_free_sheet_line_aux);
}

void psiconv_free_sheet_grid_break_list(psiconv_sheet_grid_break_list list)
{
  if (list)
    psiconv_list_free(list);
}

void psiconv_free_sheet_grid_size(psiconv_sheet_grid_size s)
{
  if (s)
    free(s);
}

void psiconv_free_sheet_grid_size_list(psiconv_sheet_grid_size_list list)
{
  if (list)
    psiconv_list_free(list);
}

void psiconv_free_sheet_grid_section(psiconv_sheet_grid_section sec)
{
  if (sec) {
    psiconv_free_sheet_grid_size_list(sec->row_heights);
    psiconv_free_sheet_grid_size_list(sec->column_heights);
    psiconv_free_sheet_grid_break_list(sec->row_page_breaks);
    psiconv_free_sheet_grid_break_list(sec->column_page_breaks);
    free(sec);
  }
}

void psiconv_free_sheet_worksheet_aux (void *data)
{
  psiconv_sheet_worksheet section = data;
  psiconv_free_sheet_cell_layout(section->default_layout);
  psiconv_free_sheet_cell_list(section->cells);
  psiconv_free_sheet_line_list(section->row_default_layouts);
  psiconv_free_sheet_line_list(section->col_default_layouts);
  psiconv_free_sheet_grid_section(section->grid);
}

void psiconv_free_sheet_worksheet(psiconv_sheet_worksheet sheet)
{
  if (sheet) {
    psiconv_free_sheet_worksheet_aux(sheet);
    free(sheet);
  }
}

void psiconv_free_sheet_worksheet_list(psiconv_sheet_worksheet_list list)
{
  if (list) 
    psiconv_list_free_el(list,psiconv_free_sheet_worksheet_aux);
}

void psiconv_free_formula_aux(void *data)
{
  psiconv_formula formula;
  formula = data;
  if (formula->type == psiconv_formula_dat_string) 
    free(formula->data.dat_string);
  else if ((formula->type != psiconv_formula_dat_int) &&
           (formula->type != psiconv_formula_dat_var) &&
           (formula->type != psiconv_formula_dat_float) &&
           (formula->type != psiconv_formula_dat_cellref) &&
           (formula->type != psiconv_formula_dat_cellblock) &&
           (formula->type != psiconv_formula_dat_vcellblock) &&
           (formula->type != psiconv_formula_mark_opsep) &&
           (formula->type != psiconv_formula_mark_opend) &&
           (formula->type != psiconv_formula_mark_eof)  &&
           (formula->type != psiconv_formula_unknown)) 
    psiconv_free_formula_list(formula->data.fun_operands);
}

void psiconv_free_formula(psiconv_formula formula)
{
  if (formula) {
    psiconv_free_formula_aux(formula);
    free(formula);
  }
}

void psiconv_free_formula_list(psiconv_formula_list list)
{
  if (list) 
    psiconv_list_free_el(list,psiconv_free_formula_aux);
}

void psiconv_free_sheet_name_section(psiconv_sheet_name_section section)
{
  if (section) {
    if(section->name)
      free(section->name);
    free(section);
  }
}

void psiconv_free_sheet_info_section(psiconv_sheet_info_section section)
{
  if (section) {
    free(section);
  }
}

void psiconv_free_sheet_variable_aux(void * variable)
{
  psiconv_sheet_variable var = variable;
  if (var->name)
    free(var->name);
  if (var->type == psiconv_var_string)
    free(var->data.dat_string);
}

void psiconv_free_sheet_variable(psiconv_sheet_variable var)
{
  if (var) {
    psiconv_free_sheet_variable_aux(var);
    free(var);
  }
}

void psiconv_free_sheet_variable_list(psiconv_sheet_variable_list list)
{
  if (list) 
    psiconv_list_free_el(list,psiconv_free_sheet_variable_aux);
}

void psiconv_free_sheet_workbook_section(psiconv_sheet_workbook_section section)
{
  if (section) {
    psiconv_free_formula_list(section->formulas);
    psiconv_free_sheet_worksheet_list(section->worksheets);
    psiconv_free_sheet_name_section(section->name);
    psiconv_free_sheet_info_section(section->info);
    psiconv_free_sheet_variable_list(section->variables);
    free(section);
  }
}

void psiconv_free_sheet_f(psiconv_sheet_f file)
{
  if (file) { 
    psiconv_free_page_layout_section(file->page_sec);
    psiconv_free_sheet_status_section(file->status_sec);
    psiconv_free_sheet_workbook_section(file->workbook_sec);
    free(file);
  }
}

void psiconv_free_texted_f(psiconv_texted_f file)
{
  if (file) {
    psiconv_free_page_layout_section(file->page_sec);
    psiconv_free_texted_section(file->texted_sec);
    free(file);
  }
}

void psiconv_free_paint_data_section_aux(void * section)
{
  if (((psiconv_paint_data_section) section)->red)
    free(((psiconv_paint_data_section)section) -> red);
  if (((psiconv_paint_data_section) section)->green)
    free(((psiconv_paint_data_section)section) -> green);
  if (((psiconv_paint_data_section) section)->blue)
    free(((psiconv_paint_data_section)section) -> blue);
}

void psiconv_free_paint_data_section(psiconv_paint_data_section section)
{
  if (section) {
    psiconv_free_paint_data_section_aux(section);
    free(section);
  }
}

void psiconv_free_pictures(psiconv_pictures section)
{
  if (section)
    psiconv_list_free_el(section,&psiconv_free_paint_data_section_aux);
}

void psiconv_free_jumptable_section (psiconv_jumptable_section section)
{
  if (section)
    psiconv_list_free(section);
} 

void psiconv_free_mbm_f(psiconv_mbm_f file)
{
  if (file) {
    psiconv_free_pictures(file->sections);
    free(file);
  }
}

void psiconv_free_sketch_section(psiconv_sketch_section sec)
{
  if (sec) {
    psiconv_free_paint_data_section(sec->picture);
    free(sec);
  }
}

void psiconv_free_sketch_f(psiconv_sketch_f file)
{
  if (file) {
    psiconv_free_sketch_section(file->sketch_sec);
    free(file);
  }
}

void psiconv_free_clipart_section_aux(void *section)
{
  if (section)
    psiconv_free_paint_data_section(((psiconv_clipart_section) section)->picture);
}

void psiconv_free_clipart_section(psiconv_clipart_section section)
{
  if (section) {
    psiconv_free_clipart_section_aux(section);
    free(section);
  }
}

void psiconv_free_cliparts(psiconv_cliparts section)
{
  if (section)
    psiconv_list_free_el(section,&psiconv_free_clipart_section_aux);
}

void psiconv_free_clipart_f(psiconv_clipart_f file)
{
  if (file) {
    psiconv_free_cliparts(file->sections);
    free(file);
  }
}

void psiconv_free_file(psiconv_file file)
{
  if (file) {
    if (file->type == psiconv_word_file) 
      psiconv_free_word_f((psiconv_word_f) file->file);
    else if (file->type == psiconv_texted_file)
      psiconv_free_texted_f((psiconv_texted_f) file->file);
    else if (file->type == psiconv_mbm_file)
      psiconv_free_mbm_f((psiconv_mbm_f) file->file);
    else if (file->type == psiconv_sketch_file)
      psiconv_free_sketch_f((psiconv_sketch_f) file->file);
    else if (file->type == psiconv_clipart_file)
      psiconv_free_clipart_f((psiconv_clipart_f) file->file);
    else if (file->type == psiconv_sheet_file)
      psiconv_free_sheet_f((psiconv_sheet_f) file->file);
    free(file);
  }
}

int psiconv_compare_color(const psiconv_color value1, 
                          const psiconv_color value2)
{
  if (!value1 || !value2)
    return 1;
  if ((value1->red == value2->red) &&
      (value1->green == value2->green) &&
      (value1->blue == value2->blue))
    return 0;
  else
    return 1;
}

int psiconv_compare_font(const psiconv_font value1,
                         const psiconv_font value2)
{
  if (!value1 || !value2 || !value1->name || !value2->name)
    return 1;
  if ((value1->screenfont == value2->screenfont) &&
      !psiconv_unicode_strcmp(value1->name,value2->name))
    return 0;
  else
    return 1;
}

int psiconv_compare_border(const psiconv_border value1,
                           const psiconv_border value2)
{
  if (!value1 || !value2)
    return 1;
  if ((value1->kind == value2->kind) &&
      (value1->thickness == value2->thickness) &&
      !psiconv_compare_color(value1->color,value2->color))
    return 0;
  else
    return 1;
}

int psiconv_compare_bullet(const psiconv_bullet value1,
                           const psiconv_bullet value2)
{
  if (!value1 || !value2)
    return 1;
  if ((value1->on == value2->on) &&
      (value1->font_size == value2->font_size) &&
      (value1->character == value2->character) &&
      (value1->indent == value2->indent) &&
      !psiconv_compare_color(value1->color,value2->color) &&
      !psiconv_compare_font(value1->font,value2->font))
    return 0;
  else
    return 1;
}

int psiconv_compare_tab(const psiconv_tab value1, const psiconv_tab value2)
{
  if (!value1 || !value2)
    return 1;
  if ((value1->location == value2->location) &&
      (value1->kind == value2->kind))
    return 0;
  else
    return 1;
}

int psiconv_compare_all_tabs(const psiconv_all_tabs value1,
                             const psiconv_all_tabs value2)
{
  int i;

  if (!value1 || !value2 || !value1->extras || !value2->extras)
    return 1;
  
  if ((value1->normal != value2->normal) ||
      psiconv_list_length(value1->extras) != 
                                        psiconv_list_length(value2->extras))
    return 1;
  for (i = 0; i < psiconv_list_length(value1->extras); i++) 
    if (psiconv_compare_tab(psiconv_list_get(value1->extras,i),
                            psiconv_list_get(value2->extras,i)))
    
      return 1;
  return 0;
}

int psiconv_compare_paragraph_layout(const psiconv_paragraph_layout value1,
                                     const psiconv_paragraph_layout value2)
{
  if (!value1 || !value2)
    return 1;
  if ((value1->indent_left == value2->indent_left) &&
      (value1->indent_right == value2->indent_right) &&
      (value1->indent_first == value2->indent_first) &&
      (value1->justify_hor == value2->justify_hor) &&
      (value1->justify_ver == value2->justify_ver) &&
      (value1->linespacing == value2->linespacing) &&
      (value1->space_above == value2->space_above) &&
      (value1->space_below == value2->space_below) &&
      (value1->keep_together == value2->keep_together) &&
      (value1->keep_with_next == value2->keep_with_next) &&
      (value1->on_next_page == value2->on_next_page) &&
      (value1->no_widow_protection == value2->no_widow_protection) &&
      (value1->border_distance == value2->border_distance) &&
      !psiconv_compare_color(value1->back_color,value2->back_color) &&
      !psiconv_compare_bullet(value1->bullet,value2->bullet) &&
      !psiconv_compare_border(value1->left_border,value2->left_border) &&
      !psiconv_compare_border(value1->right_border,value2->right_border) &&
      !psiconv_compare_border(value1->top_border,value2->top_border) &&
      !psiconv_compare_border(value1->bottom_border,value2->bottom_border) &&
      !psiconv_compare_all_tabs(value1->tabs,value2->tabs))
    return 0;
  else
    return 1;
}


int psiconv_compare_character_layout(const psiconv_character_layout value1,
                                     const psiconv_character_layout value2)
{
  if (!value1 || !value2)
    return 1;
  if ((value1->font_size == value2->font_size) &&
      (value1->italic == value2->italic) &&
      (value1->bold == value2->bold) &&
      (value1->super_sub == value2->super_sub) &&
      (value1->underline == value2->underline) &&
      (value1->strikethrough == value2->strikethrough) &&
      !psiconv_compare_color(value1->color,value2->color) &&
      !psiconv_compare_color(value1->back_color,value2->back_color) &&
      !psiconv_compare_font(value1->font,value2->font))
    return 0;
  else
    return 1;
}



psiconv_word_styles_section psiconv_empty_word_styles_section(void)
{
  psiconv_word_styles_section result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->styles = psiconv_list_new(sizeof(struct psiconv_word_style_s))))
    goto ERROR2;
  if (!(result->normal = malloc(sizeof(struct psiconv_word_style_s))))
    goto ERROR3;
  if (!(result->normal->character = psiconv_basic_character_layout()))
    goto ERROR4;
  if (!(result->normal->paragraph = psiconv_basic_paragraph_layout()))
    goto ERROR5;
  result->normal->hotkey = 'N';
  result->normal->name = NULL;
  result->normal->built_in = psiconv_bool_true;
  result->normal->outline_level = 0;
  return result;
ERROR5:
  psiconv_free_character_layout(result->normal->character);
ERROR4:
  free(result->normal);
ERROR3:
  psiconv_list_free(result->styles);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_text_and_layout psiconv_empty_text_and_layout(void)
{
  return psiconv_list_new(sizeof(struct psiconv_paragraph_s));
}

psiconv_texted_section psiconv_empty_texted_section(void)
{
  psiconv_texted_section result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->paragraphs = psiconv_empty_text_and_layout()))
    goto ERROR2;
  return result;
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_page_header psiconv_empty_page_header(void)
{
  psiconv_page_header result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  result->on_first_page = psiconv_bool_true;
  if (!(result->base_paragraph_layout = psiconv_basic_paragraph_layout()))
    goto ERROR2;
  if (!(result->base_character_layout = psiconv_basic_character_layout()))
    goto ERROR3;
  if (!(result->text = psiconv_empty_texted_section()))
    goto ERROR4;
  return result;
ERROR4:
  psiconv_free_character_layout(result->base_character_layout);
ERROR3:
  psiconv_free_paragraph_layout(result->base_paragraph_layout);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_page_layout_section psiconv_empty_page_layout_section(void)
{
  psiconv_page_layout_section result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  result->first_page_nr = 1;
  result->header_dist = result->footer_dist = 1.27;
  result->left_margin = result->right_margin = 3.175;
  result->top_margin = result->bottom_margin = 2.54;
  result->page_width = 21.0;
  result->page_height = 29.7;
  result->landscape = psiconv_bool_false;
  if (!(result->header = psiconv_empty_page_header()))
    goto ERROR2;
  if (!(result->footer = psiconv_empty_page_header()))
    goto ERROR3;
  return result;
ERROR3:
  psiconv_free_page_header(result->header);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_word_status_section psiconv_empty_word_status_section(void)
{
  psiconv_word_status_section result;
  if (!(result = malloc(sizeof(*result))))
    return NULL;
  result->show_tabs = result->show_spaces = result->show_paragraph_ends =
        result->show_hard_minus = result->show_hard_space = 
        result->fit_lines_to_screen = psiconv_bool_false;
  result->show_full_pictures = result->show_full_graphs =
        result->show_top_toolbar = result->show_side_toolbar = 
        psiconv_bool_true;
  result->cursor_position = 0;
  result->display_size = 1000;
  return result;
}

psiconv_word_f psiconv_empty_word_f(void)
{
  psiconv_word_f result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->page_sec = psiconv_empty_page_layout_section()))
    goto ERROR2;
  if (!(result->paragraphs = psiconv_empty_text_and_layout()))
    goto ERROR3;
  if (!(result->status_sec = psiconv_empty_word_status_section()))
    goto ERROR4;
  if (!(result->styles_sec = psiconv_empty_word_styles_section()))
    goto ERROR5;
  return result;
ERROR5:
  psiconv_free_word_status_section(result->status_sec);
ERROR4:
  psiconv_free_text_and_layout(result->paragraphs);
ERROR3:
  psiconv_free_page_layout_section(result->page_sec);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_sheet_status_section psiconv_empty_sheet_status_section(void)
{
  psiconv_sheet_status_section result;
  if (!(result = malloc(sizeof(*result))))
    return NULL;
  result->show_horizontal_scrollbar = result->show_vertical_scrollbar = 
        psiconv_triple_auto;
  result->show_graph = psiconv_bool_false;
  result->show_top_sheet_toolbar = result->show_side_sheet_toolbar = 
        result->show_top_graph_toolbar = result->show_side_graph_toolbar = 
        psiconv_bool_true;
  result->cursor_row = result->cursor_column = 0;
  result->sheet_display_size = result->graph_display_size = 1000;
  return result;
}

psiconv_formula_list psiconv_empty_formula_list(void)
{
  return psiconv_list_new(sizeof(struct psiconv_formula_s));
}

psiconv_sheet_workbook_section psiconv_empty_sheet_workbook_section(void)
{
  psiconv_sheet_workbook_section result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->formulas = psiconv_empty_formula_list()))
    goto ERROR2;
  return result;
ERROR2:
  free(result);
ERROR1:
  return NULL;
}


psiconv_sheet_f psiconv_empty_sheet_f(void)
{
  psiconv_sheet_f result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->page_sec = psiconv_empty_page_layout_section()))
    goto ERROR2;
  if (!(result->status_sec = psiconv_empty_sheet_status_section()))
    goto ERROR3;
  if (!(result->workbook_sec = psiconv_empty_sheet_workbook_section()))
    goto ERROR4;
  return result;
ERROR4:
  psiconv_free_sheet_status_section(result->status_sec);
ERROR3:
  psiconv_free_page_layout_section(result->page_sec);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_texted_f psiconv_empty_texted_f(void)
{
  psiconv_texted_f result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->page_sec = psiconv_empty_page_layout_section()))
    goto ERROR2;
  if (!(result->texted_sec = psiconv_empty_texted_section()))
    goto ERROR3;
  return result;
ERROR3:
  psiconv_free_page_layout_section(result->page_sec);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_paint_data_section psiconv_empty_paint_data_section(void)
{
  psiconv_paint_data_section result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  /* Is this correct? */
  result->xsize = result->ysize = result->pic_xsize = result->pic_ysize = 0;
  /* Probably forbidden... */
  if (!(result->red = malloc(0)))
    goto ERROR2;
  if (!(result->green = malloc(0)))
    goto ERROR3;
  if (!(result->blue = malloc(0)))
    goto ERROR4;
  return result;
ERROR4:
  free(result->green);
ERROR3:
  free(result->red);
ERROR2:
  free(result);
ERROR1:
  return NULL;
}


psiconv_pictures psiconv_empty_pictures(void)
{
  psiconv_pictures result;
  psiconv_paint_data_section pds;
  if (!(result = psiconv_list_new(sizeof(struct psiconv_paint_data_section_s))))
    goto ERROR1;
  if (!(pds = psiconv_empty_paint_data_section()))
    goto ERROR2;
  if (psiconv_list_add(result,pds))
    goto ERROR3;
  free(pds);
  return result;
ERROR3:
  psiconv_free_paint_data_section(pds);
ERROR2:
  psiconv_list_free(result);
ERROR1:
  return NULL;
}

psiconv_mbm_f psiconv_empty_mbm_f(void)
{
  psiconv_mbm_f result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->sections = psiconv_empty_pictures()))
    goto ERROR2;
  return result;
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_sketch_section psiconv_empty_sketch_section(void)
{
  psiconv_sketch_section result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  result->displayed_xsize = 320;
  result->displayed_ysize = 200;
  result->picture_data_x_offset = result->picture_data_y_offset = 
          result->form_xsize = result->form_ysize =
          result->displayed_size_x_offset = result->displayed_size_y_offset = 0;
  result->magnification_x = result->magnification_y = 1.0;
  result->cut_left = result->cut_right = result->cut_top = 
          result->cut_bottom = 0.0;
  if (!(result->picture = psiconv_empty_paint_data_section()))
    goto ERROR2;
  return result;
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_sketch_f psiconv_empty_sketch_f(void)
{
  psiconv_sketch_f result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->sketch_sec = psiconv_empty_sketch_section()))
    goto ERROR2;
  return result;
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_cliparts psiconv_empty_cliparts(void)
{
  /* Is this correct? */
  return psiconv_list_new(sizeof(struct psiconv_clipart_section_s));
}

psiconv_clipart_f psiconv_empty_clipart_f(void)
{
  psiconv_clipart_f result;
  if (!(result = malloc(sizeof(*result))))
    goto ERROR1;
  if (!(result->sections = psiconv_empty_cliparts()))
    goto ERROR2;
  return result;
ERROR2:
  free(result);
ERROR1:
  return NULL;
}

psiconv_file psiconv_empty_file(psiconv_file_type_t type)
{
  psiconv_file result;
  if (!(result = malloc(sizeof(*result))))
    return NULL;
  result->type = type;
  if (type == psiconv_word_file) {
    if (!(result->file =  psiconv_empty_word_f()))
      goto ERROR;
  } else if (type == psiconv_sheet_file) {
    if (!(result->file =  psiconv_empty_sheet_f()))
      goto ERROR;
  } else if (type == psiconv_texted_file) {
    if (!(result->file =  psiconv_empty_texted_f()))
      goto ERROR;
  } else if (type == psiconv_mbm_file) {
    if (!(result->file =  psiconv_empty_mbm_f()))
      goto ERROR;
  } else if (type == psiconv_sketch_file) {
    if (!(result->file =  psiconv_empty_sketch_f()))
      goto ERROR;
  } else if (type == psiconv_clipart_file) {
    if (!(result->file =  psiconv_empty_clipart_f()))
      goto ERROR;
  } else 
    goto ERROR;
  return result;
ERROR:
  free(result);
  return NULL;
}
