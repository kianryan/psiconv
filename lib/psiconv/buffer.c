/*
    buffer.c - Part of psiconv, a PSION 5 file formats converter
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
#include "compat.h"

#include <stdlib.h>

#include "list.h"
#include "error.h"
#include "buffer.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

typedef struct psiconv_relocation_s {
  psiconv_u32 offset;
  int id;
} *psiconv_relocation;

struct psiconv_buffer_s {
  psiconv_list reloc_target; /* of struct relocation_s */
  psiconv_list reloc_ref; /* of struct relocation_s */
  psiconv_list data; /* of psiconv_u8 */
};

static psiconv_u32 unique_id = 1;

psiconv_u32 psiconv_buffer_unique_id(void)
{
  return unique_id ++;
}

psiconv_buffer psiconv_buffer_new(void)
{
  psiconv_buffer buf;
  if (!(buf = malloc(sizeof(*buf))))
    goto ERROR1;
  if (!(buf->data = psiconv_list_new(sizeof(psiconv_u8)))) 
    goto ERROR2;
  if (!(buf->reloc_target = psiconv_list_new(
                                   sizeof(struct psiconv_relocation_s)))) 
    goto ERROR3;
  if (!(buf->reloc_ref = psiconv_list_new(
                                   sizeof(struct psiconv_relocation_s)))) 
    goto ERROR4;
  return buf;
ERROR4:
  psiconv_list_free(buf->reloc_target);
ERROR3:
  psiconv_list_free(buf->data);
ERROR2:
  free(buf);
ERROR1:
  return NULL;
}

void psiconv_buffer_free(psiconv_buffer buf)
{
  psiconv_list_free(buf->reloc_ref);
  psiconv_list_free(buf->reloc_target);
  psiconv_list_free(buf->data);
  free(buf);
}

psiconv_u32 psiconv_buffer_length(const psiconv_buffer buf)
{
  return psiconv_list_length(buf->data);
}

psiconv_u8 *psiconv_buffer_get(const psiconv_buffer buf, psiconv_u32 off)
{
  return psiconv_list_get(buf->data,off);
}

int psiconv_buffer_add(psiconv_buffer buf,psiconv_u8 data)
{
  return psiconv_list_add(buf->data,&data);
}

size_t psiconv_buffer_fread(psiconv_buffer buf, size_t size, FILE *f)
{
  return psiconv_list_fread(buf->data,size,f);
}

int psiconv_buffer_fread_all(psiconv_buffer buf, FILE *f)
{
  return psiconv_list_fread_all(buf->data,f);
}

int psiconv_buffer_fwrite_all(const psiconv_buffer buf, FILE *f)
{
  return psiconv_list_fwrite_all(buf->data,f);
}

int psiconv_buffer_subbuffer(psiconv_buffer *buf, const psiconv_buffer org,
                             psiconv_u32 offset, psiconv_u32 length)
{
  int i;
  int res;
  psiconv_u8 *data;
  if (! (*buf = psiconv_buffer_new())) {
    res = PSICONV_E_NOMEM;
    goto ERROR1;
  }
  for (i = 0; i < length; i++) {
    if (!(data = psiconv_buffer_get(org,offset+i))) {
      res = PSICONV_E_OTHER;
      goto ERROR2;
    }
    if ((res = psiconv_buffer_add(*buf,*data))) {
      goto ERROR2;
    }
  }
  return 0;

ERROR2:
	psiconv_buffer_free(*buf);
ERROR1:
	return res;
}

int psiconv_buffer_concat(psiconv_buffer buf, const psiconv_buffer extra)
{
  int res;
  psiconv_u32 i;
  psiconv_relocation reloc;


  for (i = 0; i < psiconv_list_length(extra->reloc_target); i++) {
    if (!(reloc = psiconv_list_get(extra->reloc_target,i))) 
      return -PSICONV_E_OTHER;
    reloc->offset += psiconv_list_length(buf->data);
    if ((res=psiconv_list_add(buf->reloc_target,reloc)))
      return res;
  }
  for (i = 0; i < psiconv_list_length(extra->reloc_ref); i++) {
    if (!(reloc = psiconv_list_get(extra->reloc_ref,i))) 
      return -PSICONV_E_OTHER;
    reloc->offset += psiconv_list_length(buf->data);
    if ((res = psiconv_list_add(buf->reloc_ref,reloc)))
      return res;
  }
  return psiconv_list_concat(buf->data,extra->data);
}

int psiconv_buffer_resolve(psiconv_buffer buf)
{
  int res;
  psiconv_u32 i,j,temp;
  psiconv_relocation target,ref;

  for (i = 0; i < psiconv_list_length(buf->reloc_ref);i++) {
    if (!(ref = psiconv_list_get(buf->reloc_ref,i))) 
      return -PSICONV_E_OTHER;
    for (j = 0;  j < psiconv_list_length(buf->reloc_target);j++) {
      if (!(target = psiconv_list_get(buf->reloc_target,j))) 
        return -PSICONV_E_OTHER;
      if (ref->id == target->id) {
        temp = target->offset & 0xff;
        if ((res = psiconv_list_replace(buf->data,ref->offset,&temp)))
          return -PSICONV_E_OTHER;
        temp = (target->offset >> 8) & 0xff;
        if ((res = psiconv_list_replace(buf->data,ref->offset + 1,&temp)))
          return -PSICONV_E_OTHER;
        temp = (target->offset >> 16) & 0xff;
        if ((res = psiconv_list_replace(buf->data,ref->offset + 2,&temp)))
          return -PSICONV_E_OTHER;
        temp = (target->offset >> 24) & 0xff;
        if ((res = psiconv_list_replace(buf->data,ref->offset + 3,&temp)))
          return -PSICONV_E_OTHER;
        break;
      }
    }
    if (j == psiconv_list_length(buf->reloc_target)) 
      return -PSICONV_E_OTHER;
  }
  psiconv_list_empty(buf->reloc_target);
  psiconv_list_empty(buf->reloc_ref);
  return -PSICONV_E_OK;
}

int psiconv_buffer_add_reference(psiconv_buffer buf,int id)
{
  struct psiconv_relocation_s reloc;
  int res,i;
  psiconv_u8 data;

  reloc.offset = psiconv_list_length(buf->data);
  reloc.id = id;
  if ((res = psiconv_list_add(buf->reloc_ref,&reloc)))
    return res;
  data = 0x00;
  for (i = 0; i < 4; i++) 
    if ((res = psiconv_list_add(buf->data,&data)))
      return res;
  return -PSICONV_E_OK;
}

int psiconv_buffer_add_target(psiconv_buffer buf, int id)
{
  struct psiconv_relocation_s reloc;

  reloc.offset = psiconv_list_length(buf->data);
  reloc.id = id;
  return psiconv_list_add(buf->reloc_target,&reloc);
}
