/*
    parse_routines.h - Part of psiconv, a PSION 5 file formats converter
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

#ifndef PSICONV_PARSE_ROUTINES_H
#define PSICONV_PARSE_ROUTINES_H

#include <psiconv/configuration.h>
#include <psiconv/general.h>
#include <psiconv/data.h>
#include <psiconv/buffer.h>
#include <psiconv/common.h>
#include <psiconv/parse.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* ******************
   * parse_simple.c *
   ****************** */

extern psiconv_u8 psiconv_read_u8(const psiconv_config config,
                                  const psiconv_buffer buf,int lev,
                                  psiconv_u32 off, int *status);
extern psiconv_u16 psiconv_read_u16(const psiconv_config config,
                                    const psiconv_buffer buf,int lev,
                                    psiconv_u32 off, int *status);
extern psiconv_u32 psiconv_read_u32(const psiconv_config config,
                                    const psiconv_buffer buf,int lev,
                                    psiconv_u32 off, int *status);
extern psiconv_s32 psiconv_read_sint(const psiconv_config config,
                                    const psiconv_buffer buf,int lev,
                                    psiconv_u32 off, int *length, int *status);

extern psiconv_u32 psiconv_read_S(const psiconv_config config,
                                  const psiconv_buffer buf, int lev, 
                                  psiconv_u32 off, int *length, int *status);
extern psiconv_u32 psiconv_read_X(const psiconv_config config,
                                  const psiconv_buffer buf, int lev, 
                                  psiconv_u32 off, int *length, int *status);
extern psiconv_length_t psiconv_read_length(const psiconv_config config,
                                            const psiconv_buffer buf, int lev,
                                            psiconv_u32 off, int *length, 
                                            int *status);
extern psiconv_size_t psiconv_read_size (const psiconv_config config,
                                         const psiconv_buffer buf, int lev,
                                         psiconv_u32 off, int *length, 
                                         int *status);
extern psiconv_string_t psiconv_read_string(const psiconv_config config,
                                            const psiconv_buffer buf,int lev,
                                            psiconv_u32 off,int *length, 
                                            int *status);
extern psiconv_string_t psiconv_read_short_string(const psiconv_config config,
                                            const psiconv_buffer buf,
                                            int lev,psiconv_u32 off,
                                            int *length,int *status);
extern psiconv_string_t psiconv_read_charlist(const psiconv_config config,
                                       const psiconv_buffer buf, int lev,
                                       psiconv_u32 off, int nrofchars,
                                       int *status);

extern int psiconv_parse_bool(const psiconv_config config,
                              const psiconv_buffer buf, int lev, 
                              psiconv_u32 off, int *length, 
                              psiconv_bool_t *result);

extern psiconv_float_t psiconv_read_float(const psiconv_config config,
                                   const psiconv_buffer buf, int lev,
                                   psiconv_u32 off, int *length, int *status);



/* ******************
   * parse_layout.c *
   ****************** */

extern int psiconv_parse_color(const psiconv_config config,
                               const psiconv_buffer buf, int lev, 
                               psiconv_u32 off, int *length, 
                               psiconv_color *result);

extern int psiconv_parse_font(const psiconv_config config,
                              const psiconv_buffer buf, int lev, 
                              psiconv_u32 off, int *length, 
                              psiconv_font *result);

extern int psiconv_parse_border(const psiconv_config config,
                                const psiconv_buffer buf,int lev,
                                psiconv_u32 off, int *length, 
                                psiconv_border *result);

extern int psiconv_parse_bullet(const psiconv_config config,
                               const psiconv_buffer buf,int lev,
                               psiconv_u32 off, int *length, 
                               psiconv_bullet *result);

extern int psiconv_parse_tab(const psiconv_config config,
                             const psiconv_buffer buf, int lev, 
                             psiconv_u32 off, int *length, 
                             psiconv_tab *result);

/* Note: the next two are special, because they modify an existing
   layout structure! If it exits due to an unexpected error, part
   of the structure may be modified, but it is still safe to call
   psiconv_free_{paragraph,character}_layout_list on it (and that
   is the only safe thing to do!) */

extern int psiconv_parse_paragraph_layout_list(const psiconv_config config,
                                   const psiconv_buffer buf, 
                                   int lev, psiconv_u32 off, int *length,
                                   psiconv_paragraph_layout result);

extern int psiconv_parse_character_layout_list(const psiconv_config config,
                                   const psiconv_buffer buf, 
                                   int lev, psiconv_u32 off, int *length,
                                   psiconv_character_layout result);


/* ****************
   * parse_page.c *
   **************** */

extern int psiconv_parse_page_header(const psiconv_config config,
                                     const psiconv_buffer buf,int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_page_header *result);

extern int psiconv_parse_page_layout_section(const psiconv_config config,
                                      const psiconv_buffer buf,int lev,
                                      psiconv_u32 off, int *length,
                                      psiconv_page_layout_section *result);

/* ******************
   * parse_common.c *
   ****************** */

extern int psiconv_parse_header_section(const psiconv_config config,
                                        const psiconv_buffer buf,int lev,
                                        psiconv_u32 off, int *length, 
                                        psiconv_header_section *result);

extern int psiconv_parse_section_table_section(const psiconv_config config,
                                      const psiconv_buffer buf, 
                                      int lev, psiconv_u32 off, 
                                      int *length,
                                      psiconv_section_table_section *result);

extern int psiconv_parse_application_id_section(const psiconv_config config,
                                      const psiconv_buffer buf, 
                                      int lev, psiconv_u32 off, int *length,
                                      psiconv_application_id_section *result);

extern int psiconv_parse_text_section(const psiconv_config config,
                                      const psiconv_buffer buf,int lev,
                                      psiconv_u32 off, int *length,
                                      psiconv_text_and_layout *result);

extern int psiconv_parse_styled_layout_section(const psiconv_config config,
                                      const psiconv_buffer buf,
                                      int lev,psiconv_u32 off,
                                      int *length,
                                      psiconv_text_and_layout result,
                                      const psiconv_word_styles_section styles);

extern int psiconv_parse_styleless_layout_section(const psiconv_config config,
                                      const psiconv_buffer buf,
                                      int lev,psiconv_u32 off,
                                      int *length,
                                      psiconv_text_and_layout result,
                                      const psiconv_character_layout base_char,
                                      const psiconv_paragraph_layout base_para);

extern int psiconv_parse_embedded_object_section
                                 (const psiconv_config config,
		                 const psiconv_buffer buf, int lev,
                                 psiconv_u32 off, int *length,
				 psiconv_embedded_object_section *result);

extern int psiconv_parse_object_display_section(const psiconv_config config,
                                        const psiconv_buffer buf,
                                        int lev,
                                        psiconv_u32 off, int *length,
                                        psiconv_object_display_section *result);

extern int psiconv_parse_object_icon_section(const psiconv_config config,
                                      const psiconv_buffer buf,int lev,
                                      psiconv_u32 off, int *length,
                                      psiconv_object_icon_section *result);


 


/* ******************
   * parse_texted.c *
   ****************** */

extern int psiconv_parse_texted_section(const psiconv_config config,
                                        const psiconv_buffer buf,int lev,
                                        psiconv_u32 off, int *length,
                                        psiconv_texted_section *result,
                                        psiconv_character_layout base_char,
                                        psiconv_paragraph_layout base_para);


/* ****************
   * parse_word.c *
   **************** */

extern int psiconv_parse_word_status_section(const psiconv_config config,
                                       const psiconv_buffer buf, int lev,
                                       psiconv_u32 off, int *length, 
                                       psiconv_word_status_section *result);

extern int psiconv_parse_word_styles_section(const psiconv_config config,
                                       const psiconv_buffer buf, int lev,
                                       psiconv_u32 off, int *length,
                                       psiconv_word_styles_section *result);


/* *****************
   * parse_sheet.c *
   ***************** */

extern int psiconv_parse_sheet_status_section(const psiconv_config config,
                                        const psiconv_buffer buf, int lev,
                                        psiconv_u32 off, int *length, 
                                        psiconv_sheet_status_section *result);

extern int psiconv_parse_sheet_formula_list(const psiconv_config config,
                                        const psiconv_buffer buf, int lev,
                                        psiconv_u32 off, int *length,
                                        psiconv_formula_list *result);

extern int psiconv_parse_formula(const psiconv_config config,
                                 const psiconv_buffer buf, int lev,
                                 psiconv_u32 off, int *length,
                                 psiconv_formula *result);

extern int psiconv_parse_sheet_workbook_section(const psiconv_config config,
                                        const psiconv_buffer buf, 
                                        int lev,
                                        psiconv_u32 off, int *length, 
                                        psiconv_sheet_workbook_section *result);
extern int psiconv_parse_sheet_cell(const psiconv_config config,
                             const psiconv_buffer buf, int lev,
                             psiconv_u32 off, int *length,
                             psiconv_sheet_cell *result,
                             const psiconv_sheet_cell_layout default_layout,
                             const psiconv_sheet_line_list row_default_layouts,
                             const psiconv_sheet_line_list col_default_layouts);

extern int psiconv_parse_sheet_cell_list(const psiconv_config config,
                             const psiconv_buffer buf, int lev,
                             psiconv_u32 off, int *length,
                             psiconv_sheet_cell_list *result,
                             const psiconv_sheet_cell_layout default_layout,
                             const psiconv_sheet_line_list row_default_layouts,
                             const psiconv_sheet_line_list col_default_layouts);

extern int psiconv_parse_sheet_worksheet_list(const psiconv_config config,
                                        const psiconv_buffer buf,
                                        int lev,
                                        psiconv_u32 off, int *length,
                                        psiconv_sheet_worksheet_list *result);

extern int psiconv_parse_sheet_worksheet(const psiconv_config config,
                                         const psiconv_buffer buf, int lev,
                                         psiconv_u32 off, int *length,
                                         psiconv_sheet_worksheet *result);

extern int psiconv_parse_sheet_numberformat(const psiconv_config config,
                                     const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_sheet_numberformat result);

extern int psiconv_parse_sheet_cell_layout(const psiconv_config config,
                                    const psiconv_buffer buf, int lev,
                                    psiconv_u32 off, int *length,
                                    psiconv_sheet_cell_layout result);

extern int psiconv_parse_sheet_line(const psiconv_config config,
                                const psiconv_buffer buf, int lev,
                                psiconv_u32 off, int *length,
                                psiconv_sheet_line *result,
                                const psiconv_sheet_cell_layout default_layout);

extern int psiconv_parse_sheet_line_list(const psiconv_config config,
                                const psiconv_buffer buf, int lev,
                                psiconv_u32 off, int *length,
                                psiconv_sheet_line_list *result,
                                const psiconv_sheet_cell_layout default_layout);

extern int psiconv_parse_sheet_name_section(const psiconv_config config,
                                     const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_sheet_name_section *result);

extern int psiconv_parse_sheet_info_section(const psiconv_config config,
                                     const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_sheet_info_section *result);

extern int psiconv_parse_sheet_variable(const psiconv_config config,
                                      const psiconv_buffer buf, int lev, 
                                      psiconv_u32 off, int *length, 
                                      psiconv_sheet_variable *result);

extern int psiconv_parse_sheet_variable_list(const psiconv_config config,
                                      const psiconv_buffer buf, int lev,
                                      psiconv_u32 off, int *length,
                                      psiconv_sheet_variable_list *result);

extern int psiconv_parse_sheet_grid_section(const psiconv_config config,
                                     const psiconv_buffer buf, int lev,
                                     psiconv_u32 off, int *length,
                                     psiconv_sheet_grid_section *result);

extern int psiconv_parse_sheet_grid_size_list(const psiconv_config config,
                                  const psiconv_buffer buf, int lev,
                                  psiconv_u32 off, int *length,
                                  psiconv_sheet_grid_size_list *result);

extern int psiconv_parse_sheet_grid_size(const psiconv_config config,
                                    const psiconv_buffer buf, int lev,
                                    psiconv_u32 off, int *length,
                                    psiconv_sheet_grid_size *result);

extern int psiconv_parse_sheet_grid_break_list(const psiconv_config config,
                                  const psiconv_buffer buf, 
                                  int lev, psiconv_u32 off, int *length,
                                  psiconv_sheet_grid_break_list *result);




/* *****************
   * parse_image.c *
   ***************** */

extern int psiconv_parse_paint_data_section(const psiconv_config config,
                                     const psiconv_buffer buf,int lev,
                                     psiconv_u32 off, int *length, 
                                     int isclipart,
                                     psiconv_paint_data_section *result);

extern int psiconv_parse_jumptable_section(const psiconv_config config,
                                        const psiconv_buffer buf,int lev,
                                        psiconv_u32 off, int *length,
                                        psiconv_jumptable_section *result);

extern int psiconv_parse_sketch_section(const psiconv_config config,
                                 const psiconv_buffer buf, int lev,
                                 psiconv_u32 off, int *length, 
                                 psiconv_sketch_section *result);

extern int psiconv_parse_clipart_section(const psiconv_config config,
                                  const psiconv_buffer buf, int lev,
                                 psiconv_u32 off, int *length,
                                 psiconv_clipart_section *result);





/* ****************
   * parse_driver.c *
   **************** */

extern int psiconv_parse_texted_file(const psiconv_config config,
                                     const psiconv_buffer buf,int lev, 
                                     psiconv_u32 off, psiconv_texted_f *result);

extern int psiconv_parse_word_file(const psiconv_config config,
                                   const psiconv_buffer buf,int lev, 
                                   psiconv_u32 off, psiconv_word_f *result);

extern int psiconv_parse_mbm_file(const psiconv_config config,
                                  const psiconv_buffer buf,int lev, 
                                  psiconv_u32 off, psiconv_mbm_f *result);

extern int psiconv_parse_sketch_file(const psiconv_config config,
                                     const psiconv_buffer buf,int lev, 
                                     psiconv_u32 off, psiconv_sketch_f *result);

extern int psiconv_parse_clipart_file(const psiconv_config config,
                                   const psiconv_buffer buf,int lev, 
                                   psiconv_u32 off, psiconv_clipart_f *result);
extern int psiconv_parse_sheet_file(const psiconv_config config,
                                    const psiconv_buffer buf,int lev, 
                                    psiconv_u32 off, psiconv_sheet_f *result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PSICONV_PARSE_ROUTINES_H */
