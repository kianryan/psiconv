/*
    gen.h - Part of psiconv, a PSION 5 file formats converter
    Copyright (c) 1990-2014  Frodo Looijaard <frodo@frodo.looijaard.name>

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

#ifndef PSICONV_GEN_H
#define PSICONV_GEN_H

#include "psiconv/data.h"
#include "psiconv.h"

void init_html5(void);

void init_xhtml(void);

void init_html4(void);

void init_txt(void);

void init_rtf(void);

void init_image(void);

void init_latex(void);

#endif /* PSICONV_GEN_H */
