/*
    magick-aux.c - Part of psiconv, a PSION 5 file formats converter
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

#if IMAGEMAGICK

#include "magick-aux.h"

#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#if IMAGEMAGICK

/* This used to be very ugly, but nowadays it is much better */

#if IMAGEMAGICK_API == 1 || IMAGEMAGICK_API == 2
const MagickInfo ** GetMagickFileList(void)
{
  int len,i;
  const MagickInfo *ptr;
  const MagickInfo *mi_ptr;
  const MagickInfo **copy;
  ExceptionInfo exc;
  GetExceptionInfo(&exc);
  OpenModules(&exc);

  mi_ptr = GetMagickInfo(NULL,&exc);
  for (len = 0, ptr=mi_ptr; ptr != NULL; len++, ptr = ptr->next);
  copy = malloc((len+1) * sizeof(*copy));
  for (i=0, ptr=mi_ptr; ptr != NULL; i++, ptr = ptr->next)
    copy[i] = ptr;
  copy[len] = NULL;
  return copy;
}

#elif IMAGEMAGICK_API == 3

const MagickInfo ** GetMagickFileList(void)
{
  MagickInfo **mi;
  const MagickInfo **copy;
  unsigned long len;
  int i;
  ExceptionInfo exc;
  GetExceptionInfo(&exc);
  OpenModules(&exc);
  mi = GetMagickInfoList("*",&len);
  copy = malloc((len+1) * sizeof(*copy));
  for (i = 0; i < len; i++) {
    copy[i] = mi[i];
  }
  copy[len] = NULL;
  return copy;
}

#elif IMAGEMAGICK_API == 4

const MagickInfo ** GetMagickFileList(void)
{
  const MagickInfo **mi;
  const MagickInfo **copy;
  unsigned long len;
  int i;
  ExceptionInfo exc;
  GetExceptionInfo(&exc);
  OpenModules(&exc);
  mi = GetMagickInfoList("*",&len,&exc);
  copy = malloc((len+1) * sizeof(*copy));
  for (i = 0; i < len; i++) {
    copy[i] = mi[i];
  }
  copy[len] = NULL;
  return copy;
}

#elif IMAGEMAGICK_API == 100
/* GraphicsMagick library */
const MagickInfo ** GetMagickFileList(void)
{
  int i,len;
  MagickInfo **mi;
  const MagickInfo **copy;
  ExceptionInfo exception;
  GetExceptionInfo(&exception);
  mi = GetMagickInfoArray(&exception);
  for (len = 0; mi[len]; len++);
  copy = malloc((len+1) * sizeof(*copy));
  for (i = 0; i <= len; i++) {
    copy[i] = mi[i];
  }
  return copy;
}

#endif

#endif /* IMAGEMAGICK */
