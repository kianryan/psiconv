/*
    list.h - Part of psiconv, a PSION 5 file formats converter
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

/* A generic list type. In C++, this would be much neater. All elements must
   be of the same size (solve it with pointers, if needed) */

#ifndef PSICONV_LIST_H
#define PSICONV_LIST_H

#include <stddef.h>
#include <stdio.h>

#include <psiconv/general.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Always use psiconv_list, never struct psiconv_list */
/* No need to export the actual internal format */
typedef struct psiconv_list_s *psiconv_list; 

/* Before using a list, call list_new. It takes the size of a single element
   as its argument. Always compute it with a sizeof() expression, just to be
   safe. The returned list is empty. 
   If there is not enough memory available, NULL is returned. You should 
   always test for this explicitely, because the other functions do not
   like a psiconv_list argument that is equal to NULL */
extern psiconv_list psiconv_list_new(size_t element_size);

/* This frees the list. If elements contain pointers that need to be freed
   separately, call list_free_el below. */
extern void psiconv_list_free(psiconv_list l);

/* This calls free_el first for each element, before doing a list_free.
   Note that you should *not* do 'free(el)' at any time; that is taken care of
   automatically. */
extern void psiconv_list_free_el(psiconv_list l, void free_el(void *el));

/* Return the number of allocated elements */
extern psiconv_u32 psiconv_list_length(const psiconv_list l);

/* Return 1 if the list is empty, 0 if not */
extern int psiconv_list_is_empty(const psiconv_list l);

/* Empty a list. Note this does not reclaim any memory space! */
extern void psiconv_list_empty(psiconv_list l);

/* Get an element from the list, and return a pointer to it. Note: you can
   directly modify this element, but be careful not to write beyond the
   element memory space. 
   If indx is out of range, NULL is returned. */
extern void * psiconv_list_get(const psiconv_list l, psiconv_u32 indx);

/* Add an element at the end of the list. The element is copied from the
   supplied element. Of course, this does not help if the element contains
   pointers. 
   As the lists extends itself, it may be necessary to allocate new
   memory. If this fails, a negative error-code is returned. If everything,
   succeeds, 0 is returned. */
extern int psiconv_list_add(psiconv_list l, const void *el);

/* Remove the last element from the list, and copy it to el. Note that
   this will not reduce the amount of space reserved for the list.
   An error code is returned, which will be 0 zero if everything
   succeeded. It is your own responsibility to make sure enough
   space is allocated to el. */
extern int psiconv_list_pop(psiconv_list l, void *el);

/* Replace an element within the list. The element is copied from the
   supplied element. Fails if you try to write at or after the end of
   the list. */
extern int psiconv_list_replace(psiconv_list l, psiconv_u32 indx, 
                                const void *el);

/* Do some action for each element. Note: you can directly modify the
   elements supplied to action, and they will be changed in the list,
   but never try a free(el)! */
extern void psiconv_list_foreach_el(psiconv_list l, void action(void *el));

/* Clone the list, that is, copy it. If elements contain pointers, you
   should call the next routine. If not enough memory is available,
   NULL is returned. */
extern psiconv_list psiconv_list_clone(const psiconv_list l);

/* Read upto size_t elements from file f, and put them at the end of list l.
   Returned is the actual number of elements added. This assumes the file
   layout and the memory layout of elements is the same. Note that if
   not enough memory could be allocated, 0 is simply returned. */
extern size_t psiconv_list_fread(psiconv_list l,size_t size, FILE *f);

/* Read the whole file f to list l. Returns 0 on succes, and an errorcode
   on failure. */
extern int psiconv_list_fread_all(psiconv_list l, FILE *f);

/* Write the whole list l to the opened file f. Returns 0 on succes, and 
   an errorcode on failure. */
extern int psiconv_list_fwrite_all(const psiconv_list l, FILE *f);

/* Concatenate two lists. The element sized does not have to be the same,
   but the result may be quite unexpected if it is not. */
int psiconv_list_concat(psiconv_list l, const psiconv_list extra);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
