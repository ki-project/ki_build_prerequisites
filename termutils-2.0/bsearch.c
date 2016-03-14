/* Copyright (C) 1991, 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef STDC_HEADERS
#include <stdlib.h>
#else
#define NULL 0
#include <sys/types.h>
#endif

/* Perform a binary search for KEY in BASE which has NMEMB elements
   of SIZE bytes each.  The comparisons are done by (*COMPAR)().  */
char *
bsearch (key, base, nmemb, size, compar)
     register char *key;
     register char *base;
     size_t nmemb;
     register size_t size;
     register int (*compar) ();
{
  register size_t l, u, idx;
  register char *p;
  register int comparison;

  l = 0;
  u = nmemb;
  while (l < u)
    {
      idx = (l + u) / 2;
      p = (char *) (((char *) base) + (idx * size));
      comparison = (*compar) (key, p);
      if (comparison < 0)
	u = idx;
      else if (comparison > 0)
	l = idx + 1;
      else
	return (char *) p;
    }

  return NULL;
}
