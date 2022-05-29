/*
    list.c - Part of psiconv, a PSION 5 file formats converter
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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "general.h"
#include "list.h"
#include "error.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif


static int psiconv_list_resize(psiconv_list l,psiconv_u32 nr);

struct psiconv_list_s {
  psiconv_u32 cur_len;
  psiconv_u32 max_len;
  size_t el_size;
  void *els;
};

psiconv_list psiconv_list_new(size_t element_size)
{
  psiconv_list l;
  l = malloc(sizeof(*l));
  if (!l)
    return NULL;
  l->cur_len = 0;
  l->max_len = 0;
  l->el_size=element_size;
  l->els = NULL; 
  return l;
} 

void psiconv_list_free(psiconv_list l)
{
  if (l->max_len)
    free(l->els);
  free(l);
  l = NULL;
}

void psiconv_list_free_el(psiconv_list l, void free_el(void *el))
{
  psiconv_list_foreach_el(l,free_el);
  psiconv_list_free(l);
}

psiconv_u32 psiconv_list_length(const psiconv_list l)
{
  return l->cur_len;
}

int psiconv_list_is_empty(const psiconv_list l)
{
  return l->cur_len == 0;
}

void psiconv_list_empty(psiconv_list l)
{
  l->cur_len = 0;
}

void *psiconv_list_get(const psiconv_list l, psiconv_u32 indx)
{
  if (indx >= l->cur_len)
    return NULL;
  else
    return ((char *) (l->els)) + indx * l->el_size;
}

int psiconv_list_add(psiconv_list l, const void *el)
{
  int res;
  if ((res = psiconv_list_resize(l,l->cur_len + 1)))
    return res;
  memcpy(((char *) (l->els)) + l->cur_len * l->el_size, el, l->el_size);
  l->cur_len ++;
  return 0;
}

int psiconv_list_pop(psiconv_list l, void *el)
{
  if (! l->cur_len)
    return -PSICONV_E_OTHER;
  l->cur_len --;
  memcpy(el,((char *)(l->els)) + l->cur_len * l->el_size,l->el_size);
  return -PSICONV_E_OK;
}

int psiconv_list_replace(psiconv_list l, psiconv_u32 indx, const void *el)
{
  if (indx >= l->cur_len)
    return -PSICONV_E_OTHER;
  memcpy(((char *) (l->els)) + indx * l->el_size,el, l->el_size);
  return -PSICONV_E_OK;
}

void psiconv_list_foreach_el(psiconv_list l, void action(void *el))
{
  psiconv_u32 i;
  for (i = 0; i < l->cur_len; i ++)
    action(psiconv_list_get(l,i));
}

psiconv_list psiconv_list_clone(const psiconv_list l)
{
  psiconv_list l2;
  psiconv_u32 i;
  l2 = psiconv_list_new(l->el_size);
  if (!l2)
    return NULL;
  for (i = 0; i < l->cur_len; i ++)
    if (psiconv_list_add(l2,psiconv_list_get(l,i))) {
      psiconv_list_free(l2);
      return NULL;
    }
  return l2;
  
}

size_t psiconv_list_fread(psiconv_list l,size_t size, FILE *f)
{
  size_t res;
  if (psiconv_list_resize(l,l->cur_len + size))
    return 0;
  res = fread(((char *) (l->els)) + l->cur_len * l->el_size,l->el_size,size,f);
  l->cur_len += res;
  return res;
}

int psiconv_list_fread_all(psiconv_list l, FILE *f)
{
  while (!feof(f)) {
    if (!psiconv_list_fread(l,1024,f) && !feof(f)) 
      return -PSICONV_E_NOMEM;
  }
  return -PSICONV_E_OK;
}

int psiconv_list_fwrite_all(const psiconv_list l, FILE *f)
{
  psiconv_u32 pos = 0;
  psiconv_u32 written;
  psiconv_u32 len = psiconv_list_length(l);
  while (pos < len) {
    if (!(written = fwrite(((char *)(l->els)) + pos * l->el_size,l->el_size,
                           len - pos,f)))
      return -PSICONV_E_OTHER;
    pos += written;
  }
  return -PSICONV_E_OK;
}

int psiconv_list_resize(psiconv_list l,psiconv_u32 nr)
{
  void * temp;
  if (nr > l->max_len) {
    l->max_len = 1.1 * nr;
    l->max_len += 16 - l->max_len % 16;
    temp = realloc(l->els,l->max_len * l->el_size);
    if (temp) {
      l->els = temp;
      return -PSICONV_E_OK;
    } else
      return -PSICONV_E_NOMEM;
  }
  return -PSICONV_E_OK;
}

int psiconv_list_concat(psiconv_list l, const psiconv_list extra)
{
  int res;
  if (l->el_size != extra->el_size)
    return -PSICONV_E_OTHER;
  if ((res = psiconv_list_resize(l,
                     l->cur_len + extra->cur_len)))
    return res;
  /* Unreadable but correct. */
  memcpy(((char *) (l->els)) + l->cur_len * l->el_size,extra->els,
         extra->cur_len * extra->el_size);
  l->cur_len += extra->cur_len;
  return 0;
}


