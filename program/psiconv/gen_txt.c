/*
 * gen_text.c - Part of psiconv, a PSION 5 file formats converter
 * Copyright (c) 1999  Andrew Johnson <anjohnson@iee.org>
 * Portions Copyright (c) 1999-2014  Frodo Looijaard <frodo@frodo.looijaard.name>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <psiconv/data.h>
#include <psiconv/list.h>
#include <psiconv/unicode.h>
#include "general.h"
#include "gen.h"
#include "psiconv.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static void output_para(const psiconv_config config,psiconv_list list,
                        const psiconv_paragraph para,encoding encoding_type);
static void gen_word(const psiconv_config config, psiconv_list list, 
                         psiconv_word_f wf, encoding encoding_type);
static void gen_texted(const psiconv_config config, psiconv_list list, 
                           psiconv_texted_f tf, encoding encoding_type);
static int gen_txt(const psiconv_config config, psiconv_list list,
			   const psiconv_file file, const char *dest,
			   const encoding encoding_type);

static struct fileformat_s ff =
  {
    "ASCII",
    "Plain text without much layout",
    FORMAT_WORD | FORMAT_TEXTED,
    gen_txt
  };


void output_para(const psiconv_config config,psiconv_list list,
                 const psiconv_paragraph para,encoding encoding_type)
{
  int i;
  if (para && para->base_paragraph && para->base_paragraph->bullet &&
      para->base_paragraph->bullet->on) {
    output_char(config,list,para->base_paragraph->bullet->character,
	            encoding_type);
    output_char(config,list,' ', encoding_type);
    output_char(config,list,' ', encoding_type);
    output_char(config,list,' ', encoding_type);
  }
  if (para && para->text) {
    for (i = 0; i < psiconv_unicode_strlen(para->text); i++) 
      switch (para->text[i]) {
	case 0x06: 
	case 0x07:
	case 0x08:
	  output_char(config,list,'\n',encoding_type); 
	  break;
	case 0x09:
	case 0x0a:
	  output_char(config,list,'\t',encoding_type);
	  break;
	case 0x0b:
	case 0x0c:
	  output_char(config,list,'-',encoding_type);
	  break;
	case 0x0f:
	  output_char(config,list,' ',encoding_type);
	  break;
	case 0x00: 
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x0e:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1a:
	case 0x1c:
	case 0x1d:
	case 0x1e:
	case 0x1f:
	  break;
	default: 
	  output_char(config,list,para->text[i],encoding_type);
	  break;
      }
    output_char(config,list,'\n',encoding_type); 
  }
}

void gen_word(const psiconv_config config, psiconv_list list, 
                         psiconv_word_f wf, encoding encoding_type)
{
  int i;
  psiconv_paragraph para;

  if (wf && wf->page_sec && wf->page_sec->header &&
      wf->page_sec->header->text && wf->page_sec->header->text->paragraphs) {
    for (i=0; 
	 i < psiconv_list_length(wf->page_sec->header->
					   text->paragraphs); i++) {
      para = psiconv_list_get(wf->page_sec->header->text->paragraphs,
				      i);
      output_para(config,list,para,encoding_type);
    }
  }
  output_char(config,list,'\n',encoding_type);
    
  if (wf && wf->paragraphs) 
    for (i=0; i < psiconv_list_length(wf->paragraphs); i++) {
	  para = psiconv_list_get(wf->paragraphs, i);
	  output_para(config, list,para,encoding_type);
  }
    
  output_char(config,list,'\n',encoding_type);

  if (wf && wf->page_sec && wf->page_sec->footer &&
      wf->page_sec->footer->text && wf->page_sec->footer->text->paragraphs) {
    for (i=0; 
	 i < psiconv_list_length(wf->page_sec->footer->
					 text->paragraphs); i++) {
      para = psiconv_list_get(wf->page_sec->footer->text->paragraphs, i);
      output_para(config,list,para,encoding_type);
    }
  }
}

void gen_texted(const psiconv_config config, psiconv_list list, 
                           psiconv_texted_f tf, encoding encoding_type)
{
  int i;
  psiconv_paragraph para;

  if (tf && tf->page_sec && tf->page_sec->header &&
      tf->page_sec->header->text && tf->page_sec->header->text->paragraphs) {
    for (i=0; 
	 i < psiconv_list_length(tf->page_sec->header->
				       text->paragraphs); i++) {
      para = psiconv_list_get(tf->page_sec->header->text->paragraphs,
				  i);
      output_para(config,list,para,encoding_type);
    }
  }
  output_char(config,list,'\n',encoding_type);
    
  if (tf && tf->texted_sec && tf->texted_sec->paragraphs) 
    for (i=0; i < psiconv_list_length(tf->texted_sec->paragraphs); i++) {
	  para = psiconv_list_get(tf->texted_sec->paragraphs, i);
	  output_para(config, list,para,encoding_type);
  }
    
  output_char(config,list,'\n',encoding_type);

  if (tf && tf->page_sec && tf->page_sec->footer &&
      tf->page_sec->footer->text && tf->page_sec->footer->text->paragraphs) {
    for (i=0; 
	 i < psiconv_list_length(tf->page_sec->footer->
					   text->paragraphs); i++) {
      para = psiconv_list_get(tf->page_sec->footer->text->paragraphs, i);
      output_para(config,list,para,encoding_type);
    }
  }
}

int gen_txt(const psiconv_config config, psiconv_list list,
			   const psiconv_file file, const char *dest,
			   const encoding encoding_type)
{
  if (file->type == psiconv_word_file) {
    gen_word(config,list,(psiconv_word_f) file->file,encoding_type);
    return 0;
  } else if (file->type == psiconv_texted_file) {
    gen_texted(config,list,(psiconv_texted_f) file->file,encoding_type);
    return 0;
  } else
    return -1;
}

void init_txt(void)
{
  psiconv_list_add(fileformat_list,&ff);
}

