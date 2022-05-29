/*
    buffer.h - Part of psiconv, a PSION 5 file formats converter
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

#ifndef PSICONV_BUFFER_H
#define PSICONV_BUFFER_H

/* A psiconv_buffer is a buffer of raw byte data. It is used when parsing
   or generating a Psion file. You can use references within it that
   are resolved at a later time. */

#include <psiconv/general.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Always use psiconv_buffer, never struct psiconv_buffer_s */
/* No need to export the actual internal format */
typedef struct psiconv_buffer_s *psiconv_buffer;

/* Allocate a new buffer. Returns NULL when not enough memory is available.
   All other functions assume you have called this function first! */
extern psiconv_buffer psiconv_buffer_new(void);

/* Free a buffer and reclaim its memory. Never use a buffer again after
   calling this (unless you do a psiconv_buffer_new on it first) */
extern void psiconv_buffer_free(psiconv_buffer buf);

/* Get the length of the data */
extern psiconv_u32 psiconv_buffer_length(const psiconv_buffer buf);

/* Get one byte of data. Returns NULL if you are trying to read past
   the end of the buffer. Do not use this; instead use psiconv_read_u8
   and friends */
extern psiconv_u8 *psiconv_buffer_get(const psiconv_buffer buf, 
                                      psiconv_u32 off);

/* Add one byte of data to the end. Returns 0 on success, and an error
   code on failure. Do not use this; instead use psiconv_write_u8 and
   friends */
extern int psiconv_buffer_add(psiconv_buffer buf,psiconv_u8 data);

/* Do an fread to the buffer. Returns the number of read bytes. See
   fread(3) for more information. */
extern size_t psiconv_buffer_fread(psiconv_buffer buf,size_t size, FILE *f);

/* Read a complete file to the buffer. Returns 0 on success, and an
   error code on failure. */
extern int psiconv_buffer_fread_all(psiconv_buffer buf, FILE *f);

/* Write a complete buffer to file. Returns 0 on success, and an
   error code on failure. */
extern int psiconv_buffer_fwrite_all(const psiconv_buffer buf, FILE *f);

/* Concatenate two buffers: the second buffer is appended to the first.
   References are updated too. Buffer extra is untouched after this and must
   still be freed if you want to get rid of it. */
extern int psiconv_buffer_concat(psiconv_buffer buf,
                                  const psiconv_buffer extra);

/* Add a target to the reference list. This does not really change the
   buffer data in any way. The id needs to be unique. The target is
   added at the current end of the buffer. */
extern int psiconv_buffer_add_target(psiconv_buffer buf, int id);

/* Add a reference to a target to the reference list. The id does not
   need to be defined already, though it must be by the time you call
   psiconv_buffer_resolve. The reference is added to the current end
   of the buffer, and space is allocated for it. References are always
   longs (psiconv_u32). */
extern int psiconv_buffer_add_reference(psiconv_buffer buf,int id);

/* Resolve all references and empty the reference list. */
extern int psiconv_buffer_resolve(psiconv_buffer buf);

/* Get a unique reference id */
extern psiconv_u32 psiconv_buffer_unique_id(void);

/* Extract part of a buffer and put it into a new buffer. Note that 
   references and targets are not copied; you will have to resolve them 
   beforehand (but as this function is meant for reading buffers, they
   will usually not be used). */
extern int psiconv_buffer_subbuffer(psiconv_buffer *buf, 
                                    const psiconv_buffer org,
                                    psiconv_u32 offset, psiconv_u32 length);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* def PSICONV_BUFFER_H */
