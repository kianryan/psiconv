/*
    checkuid.c - Part of psiconv, a PSION 5 file formats converter
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
#include "common.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static psiconv_u32 uid1[32] = 
                         { /* bit 0  */  0x000045A0,
                           /* bit 1  */  0x00008B40,
                           /* bit 2  */  0x000006A1,
                           /* bit 3  */  0x00000D42,
                           /* bit 4  */  0x00001A84,
                           /* bit 5  */  0x00003508,
                           /* bit 6  */  0x00006A10,
                           /* bit 7  */  0x0000D420,
                           /* bit 8  */  0x45A00000,
                           /* bit 9  */  0x8B400000,
                           /* bit 10 */  0x06A10000,
                           /* bit 11 */  0x0D420000,
                           /* bit 12 */  0x1A840000,
                           /* bit 13 */  0x35080000,
                           /* bit 14 */  0x6A100000,
                           /* bit 15 */  0xD4200000,
                           /* bit 16 */  0x0000AA51,
                           /* bit 17 */  0x00004483,
                           /* bit 18 */  0x00008906,
                           /* bit 19 */  0x0000022D,
                           /* bit 20 */  0x0000045A,
                           /* bit 21 */  0x000008B4,
                           /* bit 22 */  0x00001168,
                           /* bit 23 */  0x000022D0,
                           /* bit 24 */  0xAA510000,
                           /* bit 25 */  0x44830000,
                           /* bit 26 */  0x89060000,
                           /* bit 27 */  0x022D0000,
                           /* bit 28 */  0x045A0000,
                           /* bit 29 */  0x08B40000,
                           /* bit 30 */  0x11680000,
                           /* bit 31 */  0x22D00000};

static psiconv_u32 uid2[32] = 
                         { /* bit 0  */  0x000076B4,
                           /* bit 1  */  0x0000ED68,
                           /* bit 2  */  0x0000CAF1,
                           /* bit 3  */  0x000085C3,
                           /* bit 4  */  0x00001BA7,
                           /* bit 5  */  0x0000374E,
                           /* bit 6  */  0x00006E9C,
                           /* bit 7  */  0x0000DD38,
                           /* bit 8  */  0x76B40000,
                           /* bit 9  */  0xED680000,
                           /* bit 10 */  0xCAF10000,
                           /* bit 11 */  0x85C30000,
                           /* bit 12 */  0x1BA70000,
                           /* bit 13 */  0x374E0000,
                           /* bit 14 */  0x6E9C0000,
                           /* bit 15 */  0xDD380000,
                           /* bit 16 */  0x00003730,
                           /* bit 17 */  0x00006E60,
                           /* bit 18 */  0x0000DCC0,
                           /* bit 19 */  0x0000A9A1,
                           /* bit 20 */  0x00004363,
                           /* bit 21 */  0x000086C6,
                           /* bit 22 */  0x00001DAD,
                           /* bit 23 */  0x00003B5A,
                           /* bit 24 */  0x37300000,
                           /* bit 25 */  0x6E600000,
                           /* bit 26 */  0xDCC00000,
                           /* bit 27 */  0xA9A10000,
                           /* bit 28 */  0x43630000,
                           /* bit 29 */  0x86C60000,
                           /* bit 30 */  0x1DAD0000,
                           /* bit 31 */  0x3B5A0000 };

static psiconv_u32 uid3[32] = 
                         { /* bit 0  */  0x00003331,
                           /* bit 1  */  0x00006662,
                           /* bit 2  */  0x0000CCC4,
                           /* bit 3  */  0x000089A9,
                           /* bit 4  */  0x00000373,
                           /* bit 5  */  0x000006E6,
                           /* bit 6  */  0x00000DCC,
                           /* bit 7  */  0x00001B98,
                           /* bit 8  */  0x33310000,
                           /* bit 9  */  0x66620000,
                           /* bit 10 */  0xCCC40000,
                           /* bit 11 */  0x89A90000,
                           /* bit 12 */  0x03730000,
                           /* bit 13 */  0x06E60000,
                           /* bit 14 */  0x0DCC0000,
                           /* bit 15 */  0x1B980000,
                           /* bit 16 */  0x00001021,
                           /* bit 17 */  0x00002042,
                           /* bit 18 */  0x00004084,
                           /* bit 19 */  0x00008108,
                           /* bit 20 */  0x00001231,
                           /* bit 21 */  0x00002462,
                           /* bit 22 */  0x000048C4,
                           /* bit 23 */  0x00009188,
                           /* bit 24 */  0x10210000,
                           /* bit 25 */  0x20420000,
                           /* bit 26 */  0x40840000,
                           /* bit 27 */  0x81080000,
                           /* bit 28 */  0x12310000,
                           /* bit 29 */  0x24620000,
                           /* bit 30 */  0x48C40000,
                           /* bit 31 */  0x91880000 };


psiconv_u32 psiconv_checkuid(psiconv_u32 id1,psiconv_u32 id2,psiconv_u32 id3) 
{
  psiconv_u32 i;
  psiconv_u32 res = 0;
  for (i = 0; i < 32; i++) {
    if (id1 & (1 << i)) 
      res = res ^ uid1[i];
    if (id2 & (1 << i)) 
      res = res ^ uid2[i];
    if (id3 & (1 << i)) 
      res = res ^ uid3[i];
  }
  return res;
}
