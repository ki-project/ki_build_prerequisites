/* fgetline -- read a line of an unlimited length.
   Copyright (C) 1993 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#if STDC_HEADERS || HAVE_STRING_H
#include <string.h>
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
#define strchr index
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef STDC_HEADERS
#include <stdlib.h>
#else
char *getenv ();
char *malloc ();
char *realloc ();
#endif

/* Up to this many bytes are read with a call to
   fgets.  Should be a bit larger than the expected
   average line length */
#define LINE_UNIT 64

/* Read a line of an unlimited length from fp
   and return the pointer to it.  The caller is
   responsible for freeing the buffer */

char *
fgetline (fp)
     FILE *fp;
{
  /* Buffer used to store the bytes we read */
  char *readbuf;
  /* Number of bytes we've already read so far */
  int readcount;

  /* If we have already reached the end of the file,
     do not bother allocating a buffer to call fgets */
  if (feof (fp))
    return 0;

  readcount = 0;
  readbuf = malloc (LINE_UNIT);
  if (readbuf == 0)
    return 0;			/* What else can we do? */

  while (1)
    {
      if (fgets (readbuf + readcount, LINE_UNIT, fp) == 0)
	{
	  if (readcount)
	    return readbuf;	/* We've read something */
	  else
	    return 0;		/* got EOF before reading anything */
	}
      if (strchr (readbuf + readcount, '\n') == 0)
	{
	  /* fgets returned because the line was too long */
	  readcount += LINE_UNIT - 1;	/* fgets null-terminates the buffer */
	  readbuf = realloc (readbuf, readcount + LINE_UNIT);
	  if (readbuf == 0)
	    /* We could save the old readbuf and return it to the
	       caller, but some implementations of realloc
	       trashes the old buffer when it fails. */
	    return 0;
	}
      else
	return readbuf;
    }
}
