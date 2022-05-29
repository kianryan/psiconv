/*  gen_html.c - Part of psiconv, a PSION 5 file formats converter
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

#include <psiconv/configuration.h>
#include <psiconv/data.h>
#include "general.h"
#include "gen.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define TEMPSTR_LEN 100

static void text(const psiconv_config config,psiconv_list list,
          psiconv_string_t data,const encoding enc);
static void color(const psiconv_config config, psiconv_list list,
           psiconv_color color,int may_be_transparant, const encoding enc);
static void border(const psiconv_config config, psiconv_list list,
            psiconv_border_kind_t border,const encoding enc);
static void style_name(const psiconv_config config, psiconv_list list,
                const psiconv_string_t name,const encoding enc);
static int character_layout_equal(const psiconv_character_layout l1,
                           const psiconv_character_layout l2);
static void character_layout_diffs(const psiconv_config config,
                            psiconv_list list,
                            const psiconv_character_layout new,
			    const psiconv_character_layout base,
			    const encoding enc);
static void paragraph_layout_diffs(const psiconv_config config,
                            psiconv_list list,
                            const psiconv_paragraph_layout new,
			    const psiconv_paragraph_layout base,
			    const encoding enc);
static void style(const psiconv_config config, psiconv_list list,
           const psiconv_word_style style,
           const psiconv_paragraph_layout base_para,
           const psiconv_character_layout base_char,
           const encoding enc);
static void styles(const psiconv_config config, psiconv_list list,
            const psiconv_word_styles_section styles_sec,const encoding enc);
static void header(const psiconv_config config, psiconv_list list,
            const psiconv_word_styles_section styles_sec,const encoding enc);
static void footer(const psiconv_config config, psiconv_list list,
                   const encoding enc);
static void characters(const psiconv_config config, psiconv_list list,
                const psiconv_string_t textstr,
		const psiconv_character_layout layout,
		const psiconv_character_layout base,
		const encoding enc);
static void paragraphs(const psiconv_config config, psiconv_list list,
                psiconv_text_and_layout paragraphs,
	        const psiconv_word_styles_section styles,
		const encoding enc);
static void paragraph(const psiconv_config config, psiconv_list list,
               const psiconv_paragraph para,
	       const psiconv_word_styles_section styles_sec,
	       const encoding enc);
static void gen_word(const psiconv_config config, psiconv_list list,
                  const psiconv_word_f file, const encoding enc);
static void gen_texted(const psiconv_config config, psiconv_list list,
                    const psiconv_texted_f file, const encoding enc);
static int gen_html5(const psiconv_config config, psiconv_list list,
             const psiconv_file file, const char *dest,
	     const encoding enc);



void text(const psiconv_config config,psiconv_list list,
          psiconv_string_t data,const encoding enc)
{
  int i;
  for (i = 0; i < psiconv_unicode_strlen(data); i++) {
    if ((data[i] == 0x06) || (data[i] == 0x07) || (data[i] == 0x08))
      output_simple_chars(config,list,"<br/>",enc);
    else if ((data[i] == 0x0b) || (data[i] == 0x0c))
      output_simple_chars(config,list,"-",enc);
    else if ((data[i] == 0x0f) || (data[i] == 0x09) || (data[i] == 0x0a))
      output_simple_chars(config,list," ",enc);
    else if (data[i] >= 0x20)
      output_char(config,list,data[i],enc);
  }
}

void color(const psiconv_config config, psiconv_list list,
           psiconv_color color,int may_be_transparant, const encoding enc)
{
  char tempstr[TEMPSTR_LEN];
  if (may_be_transparant &&
      (color->red == 0xff) &&
      (color->blue == 0xff) &&
      (color->green == 0xff))
    output_simple_chars(config,list,"white",enc);
  else {
    snprintf(tempstr,TEMPSTR_LEN,"rgb(%d,%d,%d)",
	     color->red,
	     color->green,
	     color->blue);
    output_simple_chars(config,list,tempstr,enc);
  }
}

void border(const psiconv_config config, psiconv_list list,
            psiconv_border_kind_t border,const encoding enc)
{
  output_simple_chars(config,list,
           border == psiconv_border_none?"none":
           border == psiconv_border_solid?"solid":
           border == psiconv_border_double?"double":
           border == psiconv_border_dotted?"dotted":
           border == psiconv_border_dashed?"dashed":
           border == psiconv_border_dotdashed?"dashed":
           border == psiconv_border_dotdotdashed?"dashed":"",enc);
}

void style_name(const psiconv_config config, psiconv_list list,
                const psiconv_string_t name,const encoding enc)
{
  psiconv_string_t name_copy;
  int i;

  if (!name)
    return;

  if (!(name_copy = psiconv_unicode_strdup(name))) {
    fputs("Out of memory error\n",stderr);
    exit(1);
  }
  for (i = 0; i < psiconv_unicode_strlen(name_copy); i++) {
    if ((name_copy[i] < 0x21) ||
	((name_copy[i] >= 0x7f) && name_copy[i] <= 0xa0))
      name_copy[i] = '_';
  }
  output_string(config,list,name_copy,enc);
  free(name_copy);
}

char style_heading(const psiconv_config config, psiconv_list list,
                const psiconv_string_t name,const encoding enc)
{
  psiconv_string_t name_copy;
  int i, j;

  if (!name)
    return 0;

  if (!(name_copy = psiconv_unicode_strdup(name))) {
    fputs("Out of memory error\n",stderr);
    exit(1);
  }
  // replace special chars
  for (i = 0; i < psiconv_unicode_strlen(name_copy); i++) {
    if ((name_copy[i] < 0x21) ||
	((name_copy[i] >= 0x7f) && name_copy[i] <= 0xa0))
      name_copy[i] = '_';
  }

  // look for heading
  char heading[] = "Heading";
  int found = 0;
  for (i = 0; i < psiconv_unicode_strlen(name_copy); i++) {
    if (name_copy[i] == heading[0]) {
      found = 1;
      for (j = 0; j < strlen(heading); j++) {
        found = found && (name_copy[i + j] == heading[j]);
        if (found != 1) break;
      }
    }
    if (found == 1) break;
  }

  char heading_char = 0;
  if (found) {
    heading_char = name_copy[(i+strlen(heading)+1)];
    free(name_copy);
  }
  return heading_char;
}

/* Check whether the same layout information would be generated */
int character_layout_equal(const psiconv_character_layout l1,
                           const psiconv_character_layout l2)
{
  return (l1 && l2 &&
          (l1->color->red == l2->color->red) &&
          (l1->color->green == l2->color->green) &&
          (l1->color->blue == l2->color->blue) &&
          (l1->back_color->red == l2->back_color->red) &&
          (l1->back_color->green == l2->back_color->green) &&
          (l1->back_color->blue == l2->back_color->blue) &&
          (l1->font_size == l2->font_size) &&
          (l1->italic == l2->italic) &&
          (l1->bold == l2->bold) &&
          (l1->super_sub == l2->super_sub) &&
          (l1->underline == l2->underline) &&
          (l1->strikethrough == l2->strikethrough) &&
          (l1->font->screenfont == l2->font->screenfont));
}

void character_layout_diffs(const psiconv_config config, psiconv_list list,
                            const psiconv_character_layout new,
			    const psiconv_character_layout base,
			    const encoding enc)
{
  char tempstr[TEMPSTR_LEN];

  if (!base || (new->color->red != base->color->red) ||
      (new->color->green != base->color->green) ||
      (new->color->blue != base->color->blue)) {
    output_simple_chars(config,list,"color:",enc);
    color(config,list,new->color,0,enc);
    output_simple_chars(config,list,";",enc);
  }

  if (!base || (new->back_color->red != base->back_color->red) ||
      (new->back_color->green != base->back_color->green) ||
      (new->back_color->blue != base->back_color->blue)) {
    output_simple_chars(config,list,"background-color:",enc);
    color(config,list,new->back_color,1,enc);
    output_simple_chars(config,list,";",enc);
  }

  if (!base || (new->font_size != base->font_size)) {
    output_simple_chars(config,list,"font-size:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",new->font_size);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"pt;",enc);
  }

  if (!base || (new->italic != base->italic)) {
    output_simple_chars(config,list,"font-style:",enc);
    output_simple_chars(config,list,new->italic?"italic":"normal",enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base || (new->bold != base->bold)) {
    output_simple_chars(config,list,"font-weight:",enc);
    output_simple_chars(config,list,new->bold?"bold":"normal",enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base || (new->super_sub != base->super_sub)) {
    output_simple_chars(config,list,"font-style:",enc);
    output_simple_chars(config,list,
	                new->super_sub==psiconv_superscript?"super":
	                new->super_sub==psiconv_subscript?"sub":
			"normal",enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base || (new->underline != base->underline) ||
      (new->strikethrough != base->strikethrough)) {
    output_simple_chars(config,list,"text-decoration:",enc);
    output_simple_chars(config,list,new->underline?"underline":
	                            new->strikethrough?"line-through":
				    "none",enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base || (new->font->screenfont != base->font->screenfont)) {
    output_simple_chars(config,list,"font-family:",enc);
    output_simple_chars(config,list,
	           new->font->screenfont == psiconv_font_serif?"serif":
	           new->font->screenfont == psiconv_font_sansserif?"sans-serif":
	           new->font->screenfont == psiconv_font_nonprop?"monospace":
	           new->font->screenfont == psiconv_font_misc?"fantasy":"",
		   enc);
  }
}

void paragraph_layout_diffs(const psiconv_config config, psiconv_list list,
                            const psiconv_paragraph_layout new,
			    const psiconv_paragraph_layout base,
			    const encoding enc)
{
  char tempstr[TEMPSTR_LEN];
  float pad_left_base=0.0,pad_left_new,text_indent_base=0.0,text_indent_new;

  if (new->bullet->on) {
    pad_left_new = new->indent_left < new->indent_first?
                   new->indent_left:new->indent_first;
    text_indent_new = 0.0;
  } else {
    pad_left_new = new->indent_left;
    text_indent_new = new->indent_first;
  }
  if (base) {
    if (base->bullet->on) {
      pad_left_base = base->indent_left < base->indent_first?
                     base->indent_left:base->indent_first;
      text_indent_base = 0.0;
    } else {
      pad_left_base = base->indent_left;
      text_indent_base = base->indent_first;
    }
  }


  if (!base || (new->back_color->red != base->back_color->red) ||
      (new->back_color->green != base->back_color->green) ||
      (new->back_color->blue != base->back_color->blue)) {
    output_simple_chars(config,list,"background-color:",enc);
    color(config,list,new->back_color,1,enc);
    output_simple_chars(config,list,";",enc);
  }

  if (!base || (pad_left_new != pad_left_base)) {
    output_simple_chars(config,list,"padding-left:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",pad_left_new);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"cm;",enc);
  }

  if (!base || (new->indent_right != base->indent_right)) {
    output_simple_chars(config,list,"padding-right:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",new->indent_right);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"cm;",enc);
  }

  if (!base || (text_indent_new != text_indent_base)) {
    output_simple_chars(config,list,"text-indent:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",text_indent_new);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"cm;",enc);
  }

  if (!base || (new->justify_hor != base ->justify_hor)) {
    output_simple_chars(config,list,"text-align:",enc);
    output_simple_chars(config,list,
	                new->justify_hor==psiconv_justify_left?"left":
                        new->justify_hor==psiconv_justify_centre?"center":
                        new->justify_hor==psiconv_justify_right?"right":
                        new->justify_hor==psiconv_justify_full?"justify":
			"",enc);
    output_simple_chars(config,list,";",enc);
  }

#if 0
  /* This gave bad output... */
  if (!base || (new->linespacing != base->linespacing)) {
    output_simple_chars(config,list,"line-height:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",new->linespacing);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"pt;",enc);
  }
#endif

  if (!base || (new->space_above != base->space_above)) {
    output_simple_chars(config,list,"padding-top:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",new->space_above);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"pt;",enc);
  }

  if (!base || (new->space_below != base->space_below)) {
    output_simple_chars(config,list,"padding-bottom:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",new->space_below);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"pt;",enc);
  }

  if (!base || (new->right_border->kind != base->right_border->kind)) {
    output_simple_chars(config,list,"border-right-style:",enc);
    border(config,list,new->right_border->kind,enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base || (new->bottom_border->kind != base->bottom_border->kind)) {
    output_simple_chars(config,list,"border-bottom-style:",enc);
    border(config,list,new->bottom_border->kind,enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base || (new->top_border->kind != base->top_border->kind)) {
    output_simple_chars(config,list,"border-top-style:",enc);
    border(config,list,new->top_border->kind,enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base || (new->left_border->kind != base->left_border->kind)) {
    output_simple_chars(config,list,"border-left-style:",enc);
    border(config,list,new->left_border->kind,enc);
    output_simple_chars(config,list,";",enc);
  }

  if (!base ||
      ((new->right_border->kind != psiconv_border_none) &&
       (new->right_border->thickness != base->right_border->thickness))) {
    output_simple_chars(config,list,"border-right-width:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",new->right_border->thickness);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"pt;",enc);
  }
  if (!base ||
      ((new->bottom_border->kind != psiconv_border_none) &&
       (new->bottom_border->thickness != base->bottom_border->thickness))) {
    output_simple_chars(config,list,"border-bottom-width:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",new->bottom_border->thickness);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"pt;",enc);
  }
  if (!base ||
      ((new->top_border->kind != psiconv_border_none) &&
      ( new->top_border->thickness != base->top_border->thickness))) {
    output_simple_chars(config,list,"border-top-width:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",new->top_border->thickness);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"pt;",enc);
  }
  if (!base ||
      ((new->left_border->kind != psiconv_border_none) &&
       (new->left_border->thickness != base->left_border->thickness))) {
    output_simple_chars(config,list,"border-left-width:",enc);
    snprintf(tempstr,TEMPSTR_LEN,"%f",new->left_border->thickness);
    output_simple_chars(config,list,tempstr,enc);
    output_simple_chars(config,list,"pt;",enc);
  }

  if (!base ||
      ((new->right_border->kind != psiconv_border_none) &&
       ((new->right_border->color->red != base->right_border->color->red) ||
        (new->right_border->color->green != base->right_border->color->green)||
        (new->right_border->color->blue != base->right_border->color->blue)))) {
    output_simple_chars(config,list,"border-right-color:",enc);
    color(config,list,new->right_border->color,0,enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base ||
      ((new->top_border->kind != psiconv_border_none) &&
       ((new->top_border->color->red != base->top_border->color->red) ||
       (new->top_border->color->green != base->top_border->color->green) ||
       (new->top_border->color->blue != base->top_border->color->blue)))) {
    output_simple_chars(config,list,"border-top-color:",enc);
    color(config,list,new->top_border->color,0,enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base ||
      ((new->bottom_border->kind != psiconv_border_none) &&
       ((new->bottom_border->color->red != base->bottom_border->color->red) ||
       (new->bottom_border->color->green !=base->bottom_border->color->green)||
       (new->bottom_border->color->blue != base->bottom_border->color->blue)))){
    output_simple_chars(config,list,"border-bottom-color:",enc);
    color(config,list,new->bottom_border->color,0,enc);
    output_simple_chars(config,list,";",enc);
  }
  if (!base ||
      ((new->left_border->kind != psiconv_border_none) &&
       ((new->left_border->color->red != base->left_border->color->red) ||
       (new->left_border->color->green != base->left_border->color->green) ||
       (new->left_border->color->blue != base->left_border->color->blue)))) {
    output_simple_chars(config,list,"border-left-color:",enc);
    color(config,list,new->left_border->color,0,enc);
    output_simple_chars(config,list,";",enc);
  }
}

void style(const psiconv_config config, psiconv_list list,
           const psiconv_word_style style,
           const psiconv_paragraph_layout base_para,
           const psiconv_character_layout base_char,
           const encoding enc)
{
  output_simple_chars(config,list,"*.style_",enc);
  style_name(config,list,style->name,enc);
  output_simple_chars(config,list," {",enc);
  paragraph_layout_diffs(config,list,style->paragraph,base_para,enc);
  character_layout_diffs(config,list,style->character,base_char,enc);
  output_simple_chars(config,list,"}\n",enc);
}

void styles(const psiconv_config config, psiconv_list list,
            const psiconv_word_styles_section styles_sec,const encoding enc)
{
  int i;
  psiconv_word_style styl;
  psiconv_character_layout base_char;
  psiconv_paragraph_layout base_para;

  if (!(base_char = psiconv_basic_character_layout())) {
    fputs("Out of memory error\n",stderr);
    exit(1);
  }
  if (!(base_para = psiconv_basic_paragraph_layout())) {
    fputs("Out of memory error\n",stderr);
    exit(1);
  }

  output_simple_chars(config,list,"<style>\n",enc);
/*  output_simple_chars(config,list,"<![CDATA[\n",enc); */

  output_simple_chars(config,list,"body {",enc);
  paragraph_layout_diffs(config,list,base_para,NULL,enc);
  character_layout_diffs(config,list,base_char,NULL,enc);
  output_simple_chars(config,list,"}\n",enc);

  if (styles_sec) {
    style(config,list,styles_sec->normal,base_para,base_char,enc);

    for (i = 0; i < psiconv_list_length(styles_sec->styles); i++) {
      if (!(styl = psiconv_list_get(styles_sec->styles,i))) {
	fputs("Internal datastructure corruption\n",stderr);
	exit(1);
      }
      style(config,list,styl,base_para,base_char,enc);
    }
  }

/*  output_simple_chars(config,list,"]]>\n",enc); */
  output_simple_chars(config,list,"</style>\n",enc);
}

void header(const psiconv_config config, psiconv_list list,
            const psiconv_word_styles_section styles_sec,const encoding enc)
{
  output_simple_chars(config,list,
                      "<!DOCTYPE html>\n",enc);
  output_simple_chars(config,list,"<html lang=\"en\">",enc);
 output_simple_chars(config,list,"<head>\n",enc);
  output_simple_chars(config,list,"<meta http-equiv=\"Content-Type\" "
                                  "content=\"text/html; charset=",enc);
  output_simple_chars(config,list,enc==ENCODING_UTF8?"UTF-8":
                                  enc==ENCODING_UCS2?"UTF-16BE":
				  enc==ENCODING_ASCII?"US-ASCII":
				  "",enc);
  output_simple_chars(config,list,"\">\n",enc);
  output_simple_chars(config,list,"<title>EPOC32 file "
                                  "converted by psiconv</title>\n",enc);
  styles(config,list,styles_sec,enc);
  output_simple_chars(config,list,"</head>\n",enc);
  output_simple_chars(config,list,"<body>\n",enc);
}

void footer(const psiconv_config config, psiconv_list list, const encoding enc)
{
  output_simple_chars(config,list,"</body>\n",enc);
  output_simple_chars(config,list,"</html>\n",enc);
}

void characters(const psiconv_config config, psiconv_list list,
                const psiconv_string_t textstr,
		const psiconv_character_layout layout,
		const psiconv_character_layout base,
		const encoding enc)
{
  psiconv_list templist;

  if (!(templist = psiconv_list_new(sizeof(psiconv_u8)))) {
    fputs("Out of memory error\n",stderr);
    exit(1);
  }
  character_layout_diffs(config,templist,layout,base,enc);

  if (psiconv_list_length(templist)) {
    output_simple_chars(config,list,"<span style=\"",enc);
    if (psiconv_list_concat(list,templist)) {
      fputs("Out of memory error\n",stderr);
      exit(1);
    }
    output_simple_chars(config,list,"\">",enc);
  }

  text(config,list,textstr,enc);

  if (psiconv_list_length(templist)) {
    output_simple_chars(config,list,"</span>",enc);
  }

  psiconv_list_free(templist);
}

void paragraph(const psiconv_config config, psiconv_list list,
               const psiconv_paragraph para,
	       const psiconv_word_styles_section styles_sec,
	       const encoding enc)
{
  int i,charnr,start,len;
  psiconv_string_t text;
  psiconv_in_line_layout layout,next_layout;
  psiconv_word_style style = NULL;
  psiconv_paragraph_layout base_para;
  psiconv_character_layout base_char;
  psiconv_list templist;
  char open_tag[11];
  char close_tag[11];

  if (!(templist = psiconv_list_new(sizeof(psiconv_u8)))) {
    fputs("Out of memory error\n",stderr);
    exit(1);
  }

  if (styles_sec) {
    if (!(style = psiconv_get_style(styles_sec,para->base_style))) {
      fputs("Unknown style found; data corrupt\n",stderr);
      exit(1);
    }
    base_para = style->paragraph;
    base_char = style->character;
  } else {
    base_para = psiconv_basic_paragraph_layout();
    base_char = psiconv_basic_character_layout();
    if (!base_para || !base_char) {
      fputs("Out of memory error\n",stderr);
      exit(1);
    }
  }

  if (para->base_paragraph->bullet->on) {
    strcpy(open_tag, "<ul><li ");
    strcpy(close_tag, "</li></ul>");
  } else {

    // TODO: Placeholder for finding header
    char heading_value = style_heading(config,list,style->name,enc);

    if (heading_value) {
      char heading_open[11] = "<h";
      char heading_close[11] = "</h";

      // strcat(heading_open, "3");
      // strcat(heading_close, "3");

      strncat(heading_open, &heading_value, 1);
      strncat(heading_close, &heading_value, 1);

      strcat(heading_open, " ");
      strcat(heading_close, ">");

      strcpy(open_tag, heading_open);
      strcpy(close_tag, heading_close);

      // strcpy(open_tag, strcat(heading_open, &heading_value));
      // strcpy(close_tag, strcat(heading_close, &heading_value));
    } else {
      strcpy(open_tag, "<p ");
      strcpy(close_tag, "</p>");
    }
  }

  output_simple_chars(config, list,open_tag, enc);

  if (styles_sec) {
    output_simple_chars(config,list,"class=\"style_",enc);
    style_name(config,list,style->name,enc);
    output_simple_chars(config,list,"\" ",enc);
  }

  paragraph_layout_diffs(config,templist,para->base_paragraph,base_para,enc);
  character_layout_diffs(config,templist,para->base_character,base_char,enc);

  if (psiconv_list_length(templist)) {
    output_simple_chars(config,list,"style=\"",enc);
    if (psiconv_list_concat(list,templist)) {
      fputs("Out of memory error\n",stderr);
      exit(1);
    }
    output_simple_chars(config,list,"\"",enc);
  }
  output_simple_chars(config,list,">",enc);

  if (psiconv_list_length(para->in_lines) == 0) {
    if (psiconv_unicode_strlen(para->text))
      characters(config,list,para->text,para->base_character,
		 para->base_character,enc);
  } else {
    charnr = 0;
    start = -1;
    for (i = 0; i < psiconv_list_length(para->in_lines); i++) {
      if (start < 0)
	start = charnr;
      if (!(layout = psiconv_list_get(para->in_lines,i))) {
	fputs("Internal data structures corruption\n",stderr);
	exit(1);
      }
      if (i+1 < psiconv_list_length(para->in_lines)) {
        if (!(next_layout = psiconv_list_get(para->in_lines,i+1))) {
	  fputs("Internal data structures corruption\n",stderr);
	  exit(1);
	}
      } else {
	next_layout = NULL;
      }
      if (next_layout &&
	  character_layout_equal(layout->layout,next_layout->layout)) {
        charnr += layout->length;
	continue;
      }
      len = charnr - start + layout->length;
      if (len) {
        if (!(text = malloc(sizeof (*text) * (len + 1)))) {
	  fputs("Out of memory error\n",stderr);
	  exit(1);
	}
	memcpy(text,para->text+start,len * sizeof(*text));
	text[len] = 0;
	characters(config,list,text,layout->layout,para->base_character,enc);
	free(text);
      }
      charnr += layout->length;
      start = -1;
    }
  }
  output_simple_chars(config, list, close_tag, enc);
  if (!styles_sec) {
    psiconv_free_paragraph_layout(base_para);
    psiconv_free_character_layout(base_char);
  }
  psiconv_list_free(templist);
}

void paragraphs(const psiconv_config config, psiconv_list list,
                psiconv_text_and_layout paragraphs,
	        const psiconv_word_styles_section styles,
		const encoding enc)
{
  int i;
  psiconv_paragraph para;
  for (i = 0; i < psiconv_list_length(paragraphs); i++) {
    if (!(para = psiconv_list_get(paragraphs,i))) {
      fputs("Internal datastructure corruption\n",stderr);
      exit(1);
    }
    paragraph(config,list,para,styles,enc);
  }
}

void gen_word(const psiconv_config config, psiconv_list list,
                  const psiconv_word_f file, const encoding enc)
{
  if (!file)
    return;

  header(config,list,file->styles_sec,enc);
  paragraphs(config,list,file->paragraphs,file->styles_sec,enc);
  footer(config,list,enc);
}


void gen_texted(const psiconv_config config, psiconv_list list,
                    const psiconv_texted_f file, const encoding enc)
{
  header(config,list,NULL,enc);
  paragraphs(config,list,file->texted_sec->paragraphs,NULL,enc);
  footer(config,list,enc);
}

int gen_html5(const psiconv_config config, psiconv_list list,
             const psiconv_file file, const char *dest,
	     const encoding enc)
{
  encoding enc1 = enc;

  if (enc == ENCODING_PSION) {
    fputs("Unsupported encoding\n",stderr);
    return -1;
  } else if (enc == ENCODING_ASCII)
    enc1 = ENCODING_ASCII_HTML;

  if (file->type == psiconv_word_file) {
    gen_word(config,list,(psiconv_word_f) file->file,enc1);
    return 0;
  } else if (file->type == psiconv_texted_file) {
    gen_texted(config,list,(psiconv_texted_f) file->file,enc1);
    return 0;
  } else
    return -1;
}


static struct fileformat_s fileformats[] =
  {
    {
      "html5",
      "html5, CSS for formatting",
      FORMAT_WORD | FORMAT_TEXTED,
      gen_html5
    },
    {
      NULL,
      NULL,
      0,
      NULL
    }
  };


void init_html5(void)
{
  int i;
  for (i = 0; fileformats[i].name; i++)
    psiconv_list_add(fileformat_list,fileformats+i);
}
