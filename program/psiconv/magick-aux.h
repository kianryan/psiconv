/*
    magick-aux.h - Part of psiconv, a PSION 5 file formats converter
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

#if IMAGEMAGICK_API == 1

#include <magick/magick.h>

#else

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <magick/api.h>

#endif /* IMAGEMAGICK_API == 1 */

#if IMAGEMAGICK_API == 100

#define DestroyImages DestroyImageList

#endif

extern const MagickInfo ** GetMagickFileList(void);

#endif /* IMAGEMAGICK */
