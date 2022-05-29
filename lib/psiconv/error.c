/*
    error.c - Part of psiconv, a PSION 5 file formats converter
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static void psiconv_default_error_handler(int kind, psiconv_u32 off, 
                                          const char *message)
{
  fprintf(stderr,"%s\n",message);
}

#define MAX_MESSAGE 1024

void psiconv_fatal(psiconv_config config, int level, psiconv_u32 off,
                   const char *format,...)
{
  char buffer[MAX_MESSAGE];
  va_list ap;
  size_t curlen;

  va_start(ap,format);
  snprintf(buffer,MAX_MESSAGE,"Fatal error (offset %08x): ",off);
  curlen = strlen(buffer);

  vsnprintf(buffer+curlen,MAX_MESSAGE-curlen,format,ap);
  if (config->error_handler) 
    config->error_handler(PSICONV_VERB_FATAL,off,buffer);
  else
    psiconv_default_error_handler(PSICONV_VERB_FATAL,off,buffer);
  va_end(ap);

  exit(1);
}

void psiconv_error(psiconv_config config, int level, psiconv_u32 off,
                  const char *format,...)
{
  char buffer[MAX_MESSAGE];
  va_list ap;
  size_t curlen;

  va_start(ap,format);

  if (config->verbosity >= PSICONV_VERB_ERROR) {
    snprintf(buffer,MAX_MESSAGE,"ERROR (offset %08x): ",off);
    curlen = strlen(buffer);

    vsnprintf(buffer+curlen,MAX_MESSAGE-curlen,format,ap);
    if (config->error_handler) 
      config->error_handler(PSICONV_VERB_ERROR,off,buffer);
    else
      psiconv_default_error_handler(PSICONV_VERB_ERROR,off,buffer);
  }
  va_end(ap);
}

void psiconv_warn(psiconv_config config, int level, psiconv_u32 off,
                  const char *format,...)
{
  char buffer[MAX_MESSAGE];
  va_list ap;
  size_t curlen;

  va_start(ap,format);

  if (config->verbosity >= PSICONV_VERB_WARN) {
    snprintf(buffer,MAX_MESSAGE,"WARNING (offset %08x): ",off);
    curlen = strlen(buffer);

    vsnprintf(buffer+curlen,MAX_MESSAGE-curlen,format,ap);
    if (config->error_handler) 
      config->error_handler(PSICONV_VERB_WARN,off,buffer);
    else
      psiconv_default_error_handler(PSICONV_VERB_WARN,off,buffer);
  }
  va_end(ap);
}

void psiconv_progress(psiconv_config config,int level, psiconv_u32 off,
                      const char *format,...)
{
  char buffer[MAX_MESSAGE];
  va_list ap;
  size_t curlen;
  int i;

  va_start(ap,format);
  if (config->verbosity >= PSICONV_VERB_PROGRESS) {
    snprintf(buffer,MAX_MESSAGE,"%08x ",off);
    curlen = strlen(buffer);

    for (i = 0; (i < level) && (i+curlen+3 < MAX_MESSAGE); i++)
      buffer[i+curlen] = '=';
    curlen += i;
  
    buffer[curlen] = '>';
    buffer[curlen+1] = ' ';
    buffer[curlen+2] = '\0';
    curlen += 2;

    vsnprintf(buffer+curlen,MAX_MESSAGE-curlen,format,ap);

    if (config->error_handler)
      config->error_handler(PSICONV_VERB_PROGRESS,off,buffer);
    else
      psiconv_default_error_handler(PSICONV_VERB_PROGRESS,off,buffer);
  }

  va_end(ap);
}


void psiconv_debug(psiconv_config config, int level, psiconv_u32 off,
                   const char *format,...)
{
  char buffer[MAX_MESSAGE];
  va_list ap;
  size_t curlen;
  int i;

  va_start(ap,format);
  if (config->verbosity >= PSICONV_VERB_DEBUG) {
    snprintf(buffer,MAX_MESSAGE,"%08x ",off);
    curlen = strlen(buffer);

    for (i = 0; (i < level) && (i+curlen+3 < MAX_MESSAGE); i++)
      buffer[i+curlen] = '-';
    curlen += i;
  
    buffer[curlen] = '>';
    buffer[curlen+1] = ' ';
    buffer[curlen+2] = '\0';
    curlen += 2;

    vsnprintf(buffer+curlen,MAX_MESSAGE-curlen,format,ap);

    if (config->error_handler)
      config->error_handler(PSICONV_VERB_DEBUG,off,buffer);
    else
      psiconv_default_error_handler(PSICONV_VERB_DEBUG,off,buffer);
  }
  va_end(ap);
}
