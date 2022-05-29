/*
    rewrite.c - Part of psiconv, a PSION 5 file formats converter
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

#include <psiconv/parse.h>
#include <psiconv/generate.h>
#include <psiconv/configuration.h>

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  FILE *fp;
  psiconv_buffer buf;
  psiconv_file psionfile;
  psiconv_config config;

  /* psiconv_verbosity = PSICONV_VERB_DEBUG; */

  if (argc < 3) {
    fprintf(stderr,"Not enough arguments\n");
    fprintf(stderr,"Syntax: INFILE OUTFILE\n");
    exit(1);
  } 

  config = psiconv_config_default();
  psiconv_config_read(NULL,&config);

  if (!(fp = fopen(argv[1],"r"))) {
    perror("Can't open file");
    exit(1);
  }
  if (!(buf=psiconv_buffer_new())) {
    perror("Can't allocate buf");
    exit(1);
  }
  if ((psiconv_buffer_fread_all(buf,fp))) {
    perror("Can't fread file");
    exit(1);
  }
  fclose(fp);
  if ((psiconv_parse(config,buf,&psionfile))) {
    fprintf(stderr,"Parse error\n");
    exit(1);
  }

  psiconv_buffer_free(buf);
  buf = NULL;
  if (psiconv_write(config,&buf,psionfile)) {
    fprintf(stderr,"Generate error\n");
    exit(1);
  }
  psiconv_free_file(psionfile);
  if (!(fp = fopen(argv[2],"w"))) {
    perror("Can't open file");
    exit(1);
  }
  if ((psiconv_buffer_fwrite_all(buf,fp))) {
      perror("Can't fwrite file");
      exit(1);
  }
  fclose(fp);
  psiconv_buffer_free(buf);
  psiconv_config_free(config);
  exit(0);
}
