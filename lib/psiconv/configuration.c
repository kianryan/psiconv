/*
    configuration.c - Part of psiconv, a PSION 5 file formats converter
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
#include "compat.h"
#include "error.h"
#include "unicode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "configuration.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifndef CONFIGURATION_SEARCH_PATH
#define CONFIGURATION_SEARCH_PATH PSICONVETCDIR "/psiconv.conf:~/.psiconv.conf"
#endif
static struct psiconv_config_s default_config = 
    { PSICONV_VERB_WARN, 2, 0,0,0,psiconv_bool_false,NULL,'?','?',{ 0 },psiconv_bool_false };

static void psiconv_config_parse_statement(const char *filename,
                                    int linenr,
                                    const char *var, int value,
                                    psiconv_config *config);

static void psiconv_config_parse_line(const char *filename, int linenr, 
                               const char *line, psiconv_config *config);

static void psiconv_config_parse_file(const char *filename, 
                                      psiconv_config *config);

psiconv_config psiconv_config_default(void)
{
  psiconv_config result;
  result = malloc(sizeof(*result));
  *result = default_config;
  psiconv_unicode_select_characterset(result,1);
  return result;
}

void psiconv_config_free(psiconv_config config)
{
  free(config);
}

void psiconv_config_parse_statement(const char *filename,
                                    int linenr,
                                    const char *var, int value,
                                    psiconv_config *config)
{
  int charnr;

  if (!(strcasecmp(var,"verbosity"))) {
    if ((value >= 1) && (value <= 5)) 
      (*config)->verbosity = value;
    else 
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "Verbosity should be between 1 and 5",filename,linenr);
  } else if (!(strcasecmp(var,"color"))) {
    if ((value == 0) || (value == 1))
      (*config)->color = value;
    else
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "Color should be 0 or 1",filename,linenr);
  } else if (!(strcasecmp(var,"colordepth"))) {
    if ((value > 0) && (value <= 32)) 
      (*config)->colordepth = value;
    else
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "ColorDepth should be between 1 and 32",filename,linenr);
  } else if (!(strcasecmp(var,"redbits"))) {
    if ((value >= 0) && (value <= 32)) 
      (*config)->redbits = value;
    else
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "RedBits should be between 1 and 32 or 0",filename,linenr);
  } else if (!(strcasecmp(var,"greenbits"))) {
    if ((value >= 0) && (value <= 32)) 
      (*config)->greenbits = value;
    else
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "GreenBits should be between 1 and 32 or 0",filename,linenr);
  } else if (!(strcasecmp(var,"bluebits"))) {
    if ((value >= 0) && (value <= 32)) 
      (*config)->bluebits = value;
    else
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "BlueBits should be between 1 and 32 or 0",filename,linenr);
  } else if (!(strcasecmp(var,"characterset"))) {
    if ((value >= 0) && (value <= 1)) 
      psiconv_unicode_select_characterset(*config,value);
    else
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "CharacterSet should be between 0 and 1",
		    filename,linenr);
  } else if (!(strcasecmp(var,"unknownunicodechar"))) {
    if ((value >= 1) && (value < 0x10000)) 
      (*config)->unknown_unicode_char = value;
    else
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "UnknownUnicodeChar should be between 1 and 65535",
		    filename,linenr);
  } else if (!(strcasecmp(var,"unknownepocchar"))) {
    if ((value >= 1) && (value < 0x100)) 
      (*config)->unknown_epoc_char = value;
    else
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "UnknownEPOCChar should be between 1 and 255",
		    filename,linenr);
  } else if (sscanf(var,"char%d",&charnr) == strlen(var)) {
    if ((charnr < 0) || (charnr > 255))
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "CharXXX should have XXX between 0 and 255",
		    filename,linenr);
    if ((value >= 1) && (value <= 0x10000)) 
      (*config)->unicode_table[charnr] = value;
    else
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "CharXXX should be between 1 and 65535",
		    filename,linenr);
  } else {
    psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	          "Unknown variable %s",filename,linenr,var);
  }
  psiconv_debug(*config,0,0,"Configuration file %s, line %d: "
                "Set variable %s to %d",filename,linenr,var,value);
}


void psiconv_config_parse_line(const char *filename, int linenr, 
                               const char *line, psiconv_config *config) 
{

  int sovar,eovar,soval,eoval,eol;
  char *var;
  long val;

  psiconv_debug(*config,0,0,"Going to parse line %d: %s",linenr,line);
  sovar = 0;
  while (line[sovar] && (line[sovar] < 32))
    sovar ++;
  if (!line[sovar] || line[sovar] == '#')
    return;
  eovar = sovar;
  while (line[eovar] && (((line[eovar] >= 'A') && (line[eovar] <= 'Z')) || 
	                 ((line[eovar] >= 'a') && (line[eovar] <= 'z'))))
    eovar ++;
  if (sovar == eovar) {
    psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	          "Syntax error (no variable found)",filename,linenr);
    return;
  }
  soval = eovar;
  while (line[soval] && (line[soval] <= 32))
    soval ++;
  if (line[soval] != '=') {
    psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	          "Syntax error (no = token found)",filename,linenr);
    return;
  }
  soval ++;
  while (line[soval] && (line[soval] <= 32))
    soval ++;
  eoval = soval;
  while (line[eoval] && ((line[eoval] >= '0') && (line[eoval] <= '9')))
    eoval ++;
  if (eoval == soval) {
    psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	          "Syntax error (no value found)",filename,linenr);
    return;
  }
  if (soval - eoval > 7) {
    psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	          "Syntax error (value too large)",filename,linenr);
    return;
  }
  eol = eoval;
  while (line[eol] && (line[eol] < 32))
    eol ++;
  if (line[eol]) {
    psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	          "Syntax error (trailing garbage)",filename,linenr);
    return;
  }

  var = malloc(eovar - sovar + 1);
  memcpy(var,line + sovar, eovar - sovar);
  var[eovar-sovar] = 0;

  val = atol(line + soval);

  psiconv_config_parse_statement(filename,linenr,var,val,config);
  free(var);
}


void psiconv_config_parse_file(const char *filename, psiconv_config *config) 
{
  int file,linenr;
  struct stat stat_buf;
  off_t filesize,bytes_left,bytes_read,sol,eol;
  char *filebuffer,*filebuffer_ptr;

  psiconv_progress(*config,0,0,
                   "Going to access configuration file %s",filename);

  /* Try to open the file; it may fail, if it does not exist for example */
  if ((file = open(filename,O_RDONLY)) == -1) 
    goto ERROR0;

  /* Read the contents of the file into filebuffer. This may fail */
  if (fstat(file,&stat_buf)) {
    if (close(file))
      psiconv_error(*config,0,0,"Configuration file %s: "
                   "Couldn't close file",filename);
    return;
  }
  
  filesize = stat_buf.st_size;
  if (!(filebuffer = malloc(filesize + 1))) {
    psiconv_error(*config,0,0,"Configuration file %s: "
                  "Out of memory error",filename);
    goto ERROR1;
  }

  filebuffer_ptr = filebuffer;
  bytes_left = filesize;
  bytes_read = 1; /* Dummy for the first time through the loop */
  while ((bytes_read > 0) && bytes_left) {
    bytes_read = read(file,filebuffer_ptr,bytes_left);
    if (bytes_read > 0) {
      filebuffer_ptr += bytes_read;
      bytes_left -= bytes_read;
    }
  }

  /* On NFS, the first read may fail and this is not fatal */
  if (bytes_left && (bytes_left != filesize)) {
    psiconv_error(*config,0,0,"Configuration file %s: "
                 "Couldn't read file into memory",filename);
    goto ERROR2;
  }

  if (close(file)) {
    psiconv_error(*config,0,0,"Configuration file %s: "
                 "Couldn't close file",filename);
    file = -1;
    goto ERROR2;
  }
  file = -1;

  psiconv_progress(*config,0,0,
                   "Going to parse configuration file %s: ",filename);
  /* Now we walk through the file to isolate lines */
  linenr = 0;
  sol = 0;

  while (sol < filesize) {
    linenr ++;
    eol = sol;
    while ((eol < filesize) && (filebuffer[eol] != 13) &&
	   (filebuffer[eol] != 10) && (filebuffer[eol] != 0))
      eol ++;
      
    if ((eol < filesize) && (filebuffer[eol] == 0)) {
      psiconv_error(*config,0,0,"Configuration file %s, line %d: "
	            "Unexpected character \000 found",filename,linenr);
      goto ERROR2;
    }
    if ((eol < filesize + 1) &&
	(((filebuffer[eol] == 13) && (filebuffer[eol+1] == 10))  || 
	 ((filebuffer[eol] == 10) && (filebuffer[eol+1] == 13)))) {
      filebuffer[eol] = 0;
      eol ++;
    }
    filebuffer[eol] = 0;
    psiconv_config_parse_line(filename,linenr,filebuffer + sol,config);
    sol = eol+1;
  }
  free(filebuffer);
  return;
 
ERROR2:
  free(filebuffer);
ERROR1:
  if ((file != -1) && close(file))
    psiconv_error(*config,0,0,"Configuration file %s: "
                 "Couldn't close file",filename);
ERROR0:
  return;
}

void psiconv_config_read(const char *extra_config_files,
                         psiconv_config *config)
{
  char *path,*pathptr,*filename,*filename_old;
  const char *home;
  int filename_len;

  /* Make path be the complete search path, colon separated */
  if (extra_config_files && strlen(extra_config_files)) {
    path = malloc(strlen(CONFIGURATION_SEARCH_PATH) + 
                  strlen(extra_config_files) + 2);
    strcpy(path,CONFIGURATION_SEARCH_PATH);
    strcat(path,":");
    strcat(path,extra_config_files);
  } else {
    path = strdup(CONFIGURATION_SEARCH_PATH);
  }

  pathptr = path;
  while (strlen(pathptr)) {
    /* Isolate the next filename */
    filename_len = (index(pathptr,':')?(index(pathptr,':') - pathptr):
	                               strlen(pathptr));
    filename = malloc(filename_len + 1);
    filename = strncpy(filename,pathptr,filename_len);
    filename[filename_len] = 0;
    pathptr += filename_len;
    if (strlen(pathptr))
      pathptr ++;

    /* Do ~ substitution */
    if ((filename[0] == '~') && ((filename[1] == '/') || filename[1] == 0)) {
      home = getenv("HOME");
      if (home) {
        filename_old = filename;
	filename = malloc(strlen(filename_old) + strlen(home));
	strcpy(filename,home);
	strcpy(filename + strlen(filename),filename_old+1);
	free(filename_old);
      }
    }
    
    psiconv_config_parse_file(filename,config);
    free(filename);
  }
  free(path);
}
