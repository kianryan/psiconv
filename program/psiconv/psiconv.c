/*
    psiconv.c - Part of psiconv, a PSION 5 file formats converter
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

/* Driver program */

#include "config.h"
#include "compat.h"
#include <getopt.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef IMAGEMAGICK
#include "magick-aux.h"
#endif

#include <psiconv/data.h>
#include <psiconv/list.h>
#include <psiconv/parse.h>
#include <psiconv/configuration.h>
#include "psiconv.h"
#include "gen.h"

static void print_help(void);
static void print_version(void);
static void strtoupper(char *str);

void print_help(void)
{
  fileformat ff;
  int i,j;

  puts("Syntax: psiconv [OPTIONS..] [FILE]");
  puts("Convert the psion file FILE to other formats");
  puts("If FILE is not specified, use stdin");
  puts("  -c, --configfile=FILE Read extra configuration file after normal ones");
  puts("  -e, --encoding=ENC    Output encoding (default: UTF8)");
  puts("  -h, --help            Display this help and exit");
  puts("  -n, --noise=LEVEL     Select what to print on stderr (overrides psiconv.conf)");
  puts("  -o, --outputfile      Output to file instead of stdout");
  puts("  -T, --type=FILETYPE   Output type (default: XHTML or TIFF");
  puts("  -V, --version         Display the program version and exit");
  puts("");
  puts("The following encodings are currently supported:");
  puts("  UTF8    Variable length Unicode encoding");
  puts("  UCS2    Fixed 16-bit length Unicode encoding");
  puts("  Psion   The encoding your Psion uses (as in psiconv.conf)");
  puts("  ASCII   7-bit ASCII (other symbols are substituted by '?')");
  puts("");
  puts("The following noise levels are currently supported:");
  puts("  1 or F: Fatal errors only");
  puts("  2 or E: Errors");
  puts("  3 or W: Warnings");
  puts("  4 or P: Progress indicators");
  puts("  5 or D: Debug data");
  puts("");
  puts("The following abbreviations are used in the output types list:");
  puts("  C  - processes ClipArt files");
  puts("  c  - processes ClipArt files containing only one image");
  puts("  M  - processes MBM files");
  puts("  m  - processes MBM files containing only one image");
  puts("  S  - processes Sketch files");
  puts("  T  - processes TextEd files");
  puts("  W  - processes Word files");
  puts("");
  puts("The following output types are known:");
  for (i = 0; i < psiconv_list_length(fileformat_list); i ++) {
    ff = psiconv_list_get(fileformat_list,i);
    printf("  %s",ff->name);
    for (j = strlen(ff->name); j < 10; j++)
      putchar(' ');
    printf("[%c%c%c%c%c] ",
	   ff->supported_format & FORMAT_CLIPART_MULTIPLE?'C':
	   ff->supported_format & FORMAT_CLIPART_SINGLE?'c':' ',
	   ff->supported_format & FORMAT_MBM_MULTIPLE?'M':
	   ff->supported_format & FORMAT_MBM_SINGLE?'m':' ',
	   ff->supported_format & FORMAT_SKETCH?'S':' ',
	   ff->supported_format & FORMAT_TEXTED?'T':' ',
	   ff->supported_format & FORMAT_WORD?'W':' ');
    puts(ff->description);
  }
  puts("");
  puts("When using UTF8 with LaTeX type, the resulting LaTeX source should be converted");
  puts(" to a suitable encoding for your LaTeX installation before being typeset");
}

void print_version(void)
{
  printf("Version %s\n",VERSION);
}

void strtoupper(char *str)
{
  int i;
  for (i = 0; i < strlen(str); i ++)
    str[i] = toupper(str[i]);
}

int main(int argc, char *argv[])
{
  struct option long_options[] =
  {
    {"help",no_argument,NULL,'h'},
    {"version",no_argument,NULL,'V'},
    {"configfile",required_argument,NULL,'c'},
    {"noise",required_argument,NULL,'n'},
    {"outputfile",required_argument,NULL,'o'},
    {"type",required_argument,NULL,'T'},
    {"encoding",no_argument,NULL,'e'},
    {0,0,0,0}
  };
  const char* short_options = "hVn:o:e:T:c:";
  int option_index;
  FILE * f;
  struct stat fbuf;

  const char *inputfilename = "";
  const char *outputfilename = "";
  const char *extra_configfile = NULL;
  char *type = NULL;
  encoding encoding_type=ENCODING_UTF8;
  psiconv_list outputlist;
  int verbosity = 0;
  psiconv_config config;

  int c,i,res;
  psiconv_buffer buf;
  psiconv_file file;
  fileformat ff = NULL;

  if (!(fileformat_list = psiconv_list_new(sizeof(struct fileformat_s)))) {
    fputs("Out of memory error",stderr);
    exit(1);
  }

  init_txt();
  init_xhtml();
  init_html4();
  init_html5();
  init_image();

  while(1) {
    c = getopt_long(argc,argv,short_options, long_options, &option_index);
    if (c == -1)
      break;
    switch(c) {
      case 'h': print_help(); exit(0);
      case 'V': print_version(); exit(0);
      case 'n': switch(optarg[0]) {
		  case '1': case 'F':case 'f':
		    verbosity=PSICONV_VERB_FATAL;
		    break;
		  case '2': case 'E':case 'e':
		    verbosity=PSICONV_VERB_ERROR;
		    break;
		  case '3': case 'W':case 'w':
		    verbosity=PSICONV_VERB_WARN;
		    break;
		  case '4': case 'P':case 'p':
		    verbosity=PSICONV_VERB_PROGRESS;
		    break;
		  case '5': case 'D':case 'd':
		    verbosity=PSICONV_VERB_DEBUG;
		    break;
		  default:
		    fputs("Unknown noise level\n",stderr);
		    exit(1);
		}
                break;
      case 'o': outputfilename = strdup(optarg); break;
      case 'T': type = strdup(optarg); break;
      case 'e': if(!strcmp(optarg,"UTF8"))
		  encoding_type = ENCODING_UTF8;
		else if (!strcmp(optarg,"UCS2"))
		  encoding_type = ENCODING_UCS2;
		else if (!strcmp(optarg,"ASCII"))
		  encoding_type = ENCODING_ASCII;
		else if (!strcmp(optarg,"Psion"))
		  encoding_type = ENCODING_PSION;
		else {
		  fputs("Unknown encoding type "
		        "(try '-h' for more information\n",stderr);
		  exit(1);
		}
		break;
      case 'c': extra_configfile = strdup(optarg); break;
      case '?': case ':': fputs("Try `-h' for more information\n",stderr);
                          exit(1);
      default: fprintf(stderr,"Internal error: getopt_long returned character "
                              "code 0%o ?? (contact the author)\n", c);
               exit(1); break;
    }
  }
  if (optind < argc-1) {
    fputs("I can only convert one file!\n"
          "Try `-h' for more information\n",stderr);
    exit(1);
  } else if (optind == argc-1)
    if (!(inputfilename = strdup(argv[optind]))) {
      fputs("Out of memory error",stderr);
      exit(1);
    }

  config = psiconv_config_default();
  psiconv_config_read(extra_configfile,&config);
  if (verbosity)
    config->verbosity = verbosity;

  /* Open inputfile for reading */

  if (strlen(inputfilename) != 0) {
    if(stat(inputfilename,&fbuf) < 0) {
      perror(inputfilename);
      exit(1);
    }
    f = fopen(inputfilename,"r");
    if (! f) {
      perror(inputfilename);
      exit(1);
    }
  } else
    f = stdin;

  if (!(buf = psiconv_buffer_new())) {
    fputs("Out of memory error",stderr);
    exit(1);
  }
  if (psiconv_buffer_fread_all(buf,f)) {
    fprintf(stderr,"Failure reading file");
    exit(1);
  }

  if (strlen(inputfilename) != 0)
    if (fclose(f)) {
      perror(inputfilename);
      exit(1);
    }

  if (psiconv_parse(config,buf,&file) || (file->type == psiconv_unknown_file))
  {
     fprintf(stderr,"Parse error\n");
     exit(1);
  }

  if (!type) {
    switch(file->type) {
      case psiconv_word_file:
      case psiconv_texted_file:
      default:
	type = "XHTML"; break;
      case psiconv_mbm_file:
      case psiconv_clipart_file:
      case psiconv_sketch_file:
	type = "TIFF"; break;
    }
  } else
    strtoupper(type);

  for (i = 0; i < psiconv_list_length(fileformat_list); i ++) {
    ff = psiconv_list_get(fileformat_list,i);
    if (! strcasecmp(type,ff->name)) {
      break;
    }
  }

  if (i == psiconv_list_length(fileformat_list)) {
    fprintf(stderr,"Unknown output type: `%s'\n",type);
    exit(1);
  }

  if (!(outputlist = psiconv_list_new(sizeof(psiconv_u8)))) {
    fputs("Out of memory error\n",stderr);
    exit(1);
  }

  res = ff->output(config,outputlist,file,type,encoding_type);
  if (res) {
    fprintf(stderr,
            "Output format `%s' not permitted for this file type\n",type);
    exit(1);
  }

  psiconv_free_file(file);

  if (strlen(outputfilename) != 0) {
    f = fopen(outputfilename,"w");
    if (! f) {
      perror(inputfilename);
      exit(1);
    }
  } else
    f = stdout;

  psiconv_list_fwrite_all(outputlist,f);

  if (fclose(f)) {
    perror(inputfilename);
    exit(1);
  }

  psiconv_list_free(outputlist);

  exit(0);
}
