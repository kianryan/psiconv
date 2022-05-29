/*
    empty.c - Part of psiconv, a PSION 5 file formats converter
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

#include <psiconv/generate.h>

#include <stdio.h>
#include <stdlib.h>

void help(void)
{
  fprintf(stderr,"Syntax: empty TYPE FILENAME\n"
                 "  TYPE may be Word or TextEd; only the first character is checked\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  FILE *fp;
  psiconv_buffer buf;
  psiconv_file psionfile;
  psiconv_file_type_t type=0;
  psiconv_config config;


  if (argc < 3) 
    help();

  if ((argv[1][0] == 't') || (argv[1][0] == 'T'))
    type = psiconv_texted_file;
  else if ((argv[1][0] == 'w') || (argv[1][0] == 'W'))
    type = psiconv_word_file;
  else  {
    help();
  }

  config = psiconv_config_default();
  psiconv_config_read(NULL,&config);
  psionfile = psiconv_empty_file(type);

  if (psiconv_write(config,&buf,psionfile)) {
    fprintf(stderr,"Generate error\n");
    exit(1);
  }
  if (!(fp = fopen(argv[2],"w"))) {
    perror("Can't open file");
    exit(1);
  }
  if ((psiconv_buffer_fwrite_all(buf,fp))) {
      perror("Can't fwrite file");
      exit(1);
  }
  exit(0);
}
