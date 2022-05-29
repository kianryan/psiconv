/*
 * gen_image.c - Part of psiconv, a PSION 5 file formats converter
 * Copyright (c) 1999-2014  Frodo Looijaard <frodo@frodo.looijaard.name>
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
#include "psiconv/data.h"
#include "gen.h"
#include "psiconv.h"
#include <string.h>
#include <stdlib.h>

#ifdef IMAGEMAGICK
#include "magick-aux.h"
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef IMAGEMAGICK
static Image *get_paint_data_section(psiconv_paint_data_section sec);
static void image_to_list(psiconv_list list,Image *image,const char *dest);
static void gen_image_list(const psiconv_config config,psiconv_list list,
                           const psiconv_list sections, const char *dest);
static void gen_clipart(const psiconv_config config,psiconv_list list,
                              const psiconv_clipart_f f, const char *dest);
static void gen_mbm(const psiconv_config config,psiconv_list list,
                          const psiconv_mbm_f f, const char *dest);
static void gen_sketch(const psiconv_config config,psiconv_list list,
                             const psiconv_sketch_f f, const char *dest);
static int gen_image(psiconv_config config, psiconv_list list,
                     const psiconv_file file, const char *dest,
		     const encoding encoding_type);

/* This is ridiculously simple using ImageMagick. Without it, it would
   be quite somewhat harder - it will be left for later on.
   Note that we ignore any errors. Dangerous... */

Image *get_paint_data_section(psiconv_paint_data_section sec)
{
  Image *image;
  float *pixel, *p, *red, *green, *blue;
  int x,y;
  ExceptionInfo exc;

  GetExceptionInfo(&exc);
  red = sec->red;
  green = sec->green;
  blue = sec->blue;
  p = pixel = malloc(sec->xsize * sec->ysize * 3 * sizeof(float));
  for (y = 0; y < sec->ysize; y++) {
    for (x = 0; x < sec->xsize; x++) {
      *p++ = *red++;
      *p++ = *green++;
      *p++ = *blue++;
    }
  }

  image = ConstituteImage(sec->xsize,sec->ysize,"RGB",FloatPixel,pixel,&exc);
  if (! image || (exc.severity != UndefinedException)) {
    MagickError(exc.severity,exc.reason,exc.description);
    exit(1);
  }
  free(pixel);

  DestroyExceptionInfo(&exc);

  return image;
}


void image_to_list(psiconv_list list,Image *image,const char *dest)
{
  ImageInfo *image_info;
  ExceptionInfo exc;
  size_t length;
  unsigned char *data;
  int i;

  strcpy(image->magick,dest);
  image_info = CloneImageInfo(NULL);
  GetExceptionInfo(&exc);
  data = ImageToBlob(image_info,image,&length,&exc);
  if (!data || (exc.severity != UndefinedException)) {
    MagickError(exc.severity,exc.reason,exc.description);
    exit(1);
  }
  for (i = 0; i < length; i++) {
    if (psiconv_list_add(list,data+i)) {
      fprintf(stderr,"Out of memory error");
      exit(1);
    }
  }
  DestroyExceptionInfo(&exc);
  DestroyImageInfo(image_info);
}

void gen_image_list(const psiconv_config config,psiconv_list list,
                           const psiconv_list sections, const char *dest)
{
  psiconv_paint_data_section section;
  const MagickInfo *mi;
  ImageInfo *image_info;
  Image *image = NULL;
  Image *last_image = NULL;
  Image *this_image, *images;
  ExceptionInfo exc;
  int i;

  GetExceptionInfo(&exc);
  mi = GetMagickInfo(dest,&exc);
  if (!mi || (exc.severity != UndefinedException)) {
    MagickError(exc.severity,exc.reason,exc.description);
    exit(1);
  }

  if ((psiconv_list_length(sections) < 1) || 
      ((psiconv_list_length(sections)) > 1 && ! (mi->adjoin))) {
    fprintf(stderr,"This image type supports only one image\n");
    exit(1);
  }
 
  for (i = 0; i < psiconv_list_length(sections); i++) {
    if (!(section = psiconv_list_get(sections,i))) {
      fprintf(stderr,"Internal data structures corrupted\n");
      exit(1);
    }
    this_image = get_paint_data_section(section);
    if (! image) {
      image = this_image;
    } else {
      last_image->next=this_image;
      this_image->previous=last_image;
    }
    last_image = this_image;
  }

  image_info = CloneImageInfo(NULL);
  if (image->next) {
    images = CoalesceImages(image,&exc);
    if (!images || (exc.severity != UndefinedException)) {
      MagickError(exc.severity,exc.reason,exc.description);
      exit(1);
    }
  } else
    images = image;

  image_to_list(list,image,dest);

  DestroyExceptionInfo(&exc);
  DestroyImageInfo(image_info);
  if (image != images)
    DestroyImages(image);
  DestroyImages(images);
}

void gen_clipart(const psiconv_config config,psiconv_list list,
                              const psiconv_clipart_f f, const char *dest)
{
  int i;
  psiconv_list sections;
  psiconv_clipart_section section;

  if (!(sections = psiconv_list_new(sizeof(*section->picture)))) {
    fprintf(stderr,"Out of memory error\n");
    exit(1);
  }
  for (i = 0; i < psiconv_list_length(f->sections); i ++) {
    if (!(section = psiconv_list_get(f->sections,i))) {
      fprintf(stderr,"Internal data structures corrupted\n");
      exit(1);
    }
    if ((psiconv_list_add(sections,section->picture))) {
      fprintf(stderr,"Out of memory error\n"); 
      exit(1);
    }
  }
  gen_image_list(config,list,sections,dest);
  psiconv_list_free(sections);
}

void gen_mbm(const psiconv_config config,psiconv_list list,
                          const psiconv_mbm_f f, const char *dest)
{
  gen_image_list(config,list,f->sections,dest);
}


void gen_sketch(const psiconv_config config,psiconv_list list,
                             const psiconv_sketch_f f, const char *dest)
{
  Image *image;
 
  image = get_paint_data_section(f->sketch_sec->picture);
  image_to_list(list,image,dest);
  DestroyImage(image);
}


int gen_image(psiconv_config config, psiconv_list list,
                     const psiconv_file file, const char *dest,
		     const encoding encoding_type)
{
  if (file->type == psiconv_mbm_file) 
    gen_mbm(config,list,(psiconv_mbm_f) file->file,dest);
  else if (file->type == psiconv_clipart_file) 
    gen_clipart(config,list,
	              (psiconv_clipart_f) file->file,dest);
  else 
  if (file->type == psiconv_sketch_file) {
    gen_sketch(config, list,(psiconv_sketch_f) file->file,dest);
  } else
    return -1;
  return 0;
}

#endif

void init_image(void)
{
  struct fileformat_s ff;
#if IMAGEMAGICK
  const MagickInfo **mi;
  int i;
#if IMAGEMAGICK_API == 100
  InitializeMagick(NULL);
#endif
  ff.output = gen_image;
  mi = GetMagickFileList();
  i = 0;
  while (mi[i]) {
    if (mi[i]->encoder) {
      ff.name = strdup(mi[i]->name);
      ff.description = strdup(mi[i]->description);
      ff.supported_format = FORMAT_CLIPART_SINGLE | FORMAT_MBM_SINGLE | 
	                    FORMAT_SKETCH;
      if (mi[i]->adjoin) 
	ff.supported_format |= FORMAT_MBM_MULTIPLE | FORMAT_CLIPART_MULTIPLE;
      psiconv_list_add(fileformat_list,&ff);
    }
    i++;
  }
#endif
}

