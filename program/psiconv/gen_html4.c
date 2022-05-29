/*  
    gen_html.c - Part of psiconv, a PSION 5 file formats converter
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

#include <string.h>
#include <stdlib.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define TEMPSTR_LEN 100

static void text(const psiconv_config config,psiconv_list list,
                 psiconv_string_t data,const encoding enc);
static void header(const psiconv_config config, psiconv_list list, 
                   const encoding enc);
static void footer(const psiconv_config config, psiconv_list list, 
                   const encoding enc);
static void characters(const psiconv_config config, psiconv_list list,
                const psiconv_string_t textstr,
		const psiconv_character_layout layout,const encoding enc);
static void paragraph(const psiconv_config config, psiconv_list list,
               psiconv_paragraph para, const encoding enc);
static void paragraphs(const psiconv_config config, psiconv_list list,
                psiconv_text_and_layout paragraphs, const encoding enc);
static void gen_word(const psiconv_config config, psiconv_list list,
                           const psiconv_word_f file, const encoding enc);
static void gen_texted(const psiconv_config config, psiconv_list list,
                    const psiconv_texted_f file, const encoding enc);
static int gen_html4(const psiconv_config config, psiconv_list list,
              const psiconv_file file, const char *dest,
	      const encoding enc);


void text(const psiconv_config config,psiconv_list list,
          psiconv_string_t data,const encoding enc)
{
  int i;
  for (i = 0; i < psiconv_unicode_strlen(data); i++) {
    if ((data[i] == 0x06) || (data[i] == 0x07) || (data[i] == 0x08))
      output_simple_chars(config,list,"<BR>",enc);
    else if ((data[i] == 0x0b) || (data[i] == 0x0c))
      output_simple_chars(config,list,"-",enc);
    else if ((data[i] == 0x0f) || (data[i] == 0x09) || (data[i] == 0x0a))
      output_simple_chars(config,list," ",enc);
    else if (data[i] >= 0x20)
      output_char(config,list,data[i],enc);
  }
}

void header(const psiconv_config config, psiconv_list list, const encoding enc)
{
  output_simple_chars(config,list,"<!DOCTYPE html PUBLIC "
                                  "\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
                                  "\"http://www.w3.org/TR/html4/loose.dtd\">\n",
                      enc);
  output_simple_chars(config,list,"<HTML>\n",enc);
  output_simple_chars(config,list,"<HEAD>\n",enc);
  output_simple_chars(config,list,"<META HTTP-EQUIV=\"Content-Type\" "
                                  "CONTENT=\"text/html; charset=",enc);
  output_simple_chars(config,list,enc==ENCODING_UTF8?"UTF-8":
                                  enc==ENCODING_UCS2?"UTF-16BE":
				  enc==ENCODING_ASCII?"US-ASCII":
				  "",enc);
  output_simple_chars(config,list,"\">\n",enc);
  output_simple_chars(config,list,"<TITLE>EPOC32 file "
                                  "converted by psiconv</TITLE>\n",enc);
  output_simple_chars(config,list,"</HEAD>\n",enc);
  output_simple_chars(config,list,"<BODY>\n",enc);
}

void footer(const psiconv_config config, psiconv_list list, const encoding enc)
{
  output_simple_chars(config,list,"</BODY>\n",enc);
  output_simple_chars(config,list,"</HTML>\n",enc);
}

int character_layout_equal(const psiconv_character_layout l1,
                           const psiconv_character_layout l2)
{
  int font_size1,font_size2; 

  font_size1 = l1->font_size < 8  ?1:
               l1->font_size < 10 ?2:
               l1->font_size < 13 ?3:
               l1->font_size < 17 ?4:
               l1->font_size < 24 ?5:
               l1->font_size < 36 ?6:7;
  font_size2 = l2->font_size < 8  ?1:
               l2->font_size < 10 ?2:
               l2->font_size < 13 ?3:
               l2->font_size < 17 ?4:
               l2->font_size < 24 ?5:
               l2->font_size < 36 ?6:7;

  return (l1 && l2 &&
          (l1->color->red == l2->color->red) &&
          (l1->color->green == l2->color->green) &&
          (l1->color->blue == l2->color->blue) &&
          (font_size1 == font_size2) &&
          (l1->italic == l2->italic) &&
          (l1->bold == l2->bold) &&
          (l1->super_sub == l2->super_sub) &&
          (l1->underline == l2->underline) &&
          (l1->strikethrough == l2->strikethrough) &&
          (l1->font->screenfont == l2->font->screenfont));
}
                                                                                
void characters(const psiconv_config config, psiconv_list list,
                const psiconv_string_t textstr,
		const psiconv_character_layout layout,const encoding enc)
{
  char tempstr[TEMPSTR_LEN];

  output_simple_chars(config,list,"<FONT face=\"",enc);
  output_simple_chars(config,list,
	        layout->font->screenfont == psiconv_font_serif?"serif":
	        layout->font->screenfont == psiconv_font_sansserif?"sans-serif":
                layout->font->screenfont == psiconv_font_nonprop?"monospace":
	        layout->font->screenfont == psiconv_font_misc?"fantasy":"",
		enc);
  output_simple_chars(config,list,"\"",enc);
  
  if ((layout->font_size < 10)  || (layout->font_size >= 13)) {
    output_simple_chars(config,list," size=",enc);
    output_simple_chars(config,list,
                        layout->font_size < 8  ?"1":
                        layout->font_size < 10 ?"2":
                        layout->font_size < 13 ?"3":
                        layout->font_size < 17 ?"4":
                        layout->font_size < 24 ?"5":
                        layout->font_size < 36 ?"6":"7",enc);
  }
  if ((layout->color->red != 0) || (layout->color->green != 0) || 
      (layout->color->blue != 0)) {
    snprintf(tempstr,TEMPSTR_LEN,"%02x%02x%02x",
	     layout->color->red,layout->color->green,layout->color->blue);
    output_simple_chars(config,list," color=#",enc);
    output_simple_chars(config,list,tempstr,enc);
  }
  output_simple_chars(config,list,">",enc);


  if (layout->italic)
    output_simple_chars(config,list,"<I>",enc);
  if (layout->bold)
    output_simple_chars(config,list,"<B>",enc);
  if (layout->super_sub != psiconv_normalscript)
    output_simple_chars(config,list,
	                layout->super_sub == psiconv_superscript?"<SUP>":
	                layout->super_sub == psiconv_subscript?"<SUB>":
			"",enc);
  if (layout->strikethrough)
    output_simple_chars(config,list,"<S>",enc);
  if (layout->underline)
    output_simple_chars(config,list,"<U>",enc);

  text(config,list,textstr,enc);
  
  if (layout->underline)
    output_simple_chars(config,list,"</U>",enc);
  if (layout->strikethrough)
    output_simple_chars(config,list,"</S>",enc);
  if (layout->super_sub != psiconv_normalscript)
    output_simple_chars(config,list,
	                layout->super_sub == psiconv_superscript?"</SUP>":
	                layout->super_sub == psiconv_subscript?"</SUB>":
			"",enc);
  if (layout->bold)
    output_simple_chars(config,list,"</B>",enc);
  if (layout->italic)
    output_simple_chars(config,list,"</I>",enc);
  output_simple_chars(config,list,"</FONT>",enc);
}

void paragraph(const psiconv_config config, psiconv_list list,
               psiconv_paragraph para, const encoding enc)
{
  int i,charnr,start,len;
  psiconv_string_t text;
  psiconv_in_line_layout layout,next_layout;


  output_simple_chars(config,list,
                      para->base_paragraph->bullet->on?"<UL><LI":"<P",enc);

  if (para->base_paragraph->justify_hor == psiconv_justify_centre) 
    output_simple_chars(config,list," align=center",enc);
  else if (para->base_paragraph->justify_hor == psiconv_justify_right) 
    output_simple_chars(config,list," align=right",enc);
  else if (para->base_paragraph->justify_hor == psiconv_justify_full) 
    output_simple_chars(config,list," align=justify",enc);

  output_simple_chars(config,list,">",enc);

  if (psiconv_list_length(para->in_lines) == 0) {
    if (psiconv_unicode_strlen(para->text))
      characters(config,list,para->text,para->base_character,enc);
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
	characters(config,list,text,layout->layout,enc);
	free(text);
      }
      charnr += layout->length;
      start = -1;
    }
  }
  output_simple_chars(config, list,
	              para->base_paragraph->bullet->on?"</UL>\n":"\n",enc);
}

void paragraphs(const psiconv_config config, psiconv_list list,
                psiconv_text_and_layout paragraphs, const encoding enc)
{
  int i;
  psiconv_paragraph para;
  for (i = 0; i < psiconv_list_length(paragraphs); i++) {
    if (!(para = psiconv_list_get(paragraphs,i))) {
      fputs("Internal datastructure corruption\n",stderr);
      exit(1);
    }
    paragraph(config,list,para,enc);
  }
}

void gen_word(const psiconv_config config, psiconv_list list,
                    const psiconv_word_f file, const encoding enc)
{
  if (!file)
    return;

  header(config,list,enc);
  paragraphs(config,list,file->paragraphs,enc);
  footer(config,list,enc);
}


void gen_texted(const psiconv_config config, psiconv_list list,
                    const psiconv_texted_f file, const encoding enc)
{
  header(config,list,enc);
  paragraphs(config,list,file->texted_sec->paragraphs,enc);
  footer(config,list,enc);
}

int gen_html4(const psiconv_config config, psiconv_list list,
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
      "HTML4",
      "HTML 4.01 Transitional, without CSS",
      FORMAT_WORD | FORMAT_TEXTED,
      gen_html4
    },
    {
      NULL,
      NULL,
      0,
      NULL
    }
  };


void init_html4(void)
{
  int i;
  for (i = 0; fileformats[i].name; i++)
    psiconv_list_add(fileformat_list,fileformats+i);
}
