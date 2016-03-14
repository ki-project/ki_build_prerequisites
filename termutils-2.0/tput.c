/* tput -- shell-level interface to terminfo, emulated by termcap.
   Copyright (C) 1991, 1995 Free Software Foundation, Inc.

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

/* Usage: tput [-T termtype] [--terminal=termtype] capability [parameter...]

   Options:
   -T termtype
   --terminal=termtype	Override $TERM.

   Requires the GNU termcap library.
   Requires the bsearch library function and GNU getopt.

   Written by David MacKenzie <djm@gnu.ai.mit.edu> and
   Junio Hamano <junio@twinsun.com>.  */

#include <config.h>
#include <stdio.h>
#include <termcap.h>
#include <getopt.h>
#include <errno.h>

#if STDC_HEADERS || HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
# define strrchr rindex
#endif

#if STDC_HEADERS
# include <stdlib.h>
#else
char *getenv ();
#endif

#include <tcutil.h>
#include <tput.h>

#ifndef errno
extern int errno;
#endif

/* Exit codes. */
#define CAP_PRESENT 0		/* Normal operation. */
#define BOOLEAN_FALSE 1		/* Boolean capability not present. */
#define USAGE_ERROR 2		/* Invalid arguments given. */
#define UNKNOWN_TERM 3		/* $TERM not found in termcap file. */
#define MISSING_CAP 4		/* Termcap entry lacks this capability. */
#define ERROR_EXIT 5		/* Real error or signal. */

void error ();
int tcputchar ();
void tput_one_item ();
void put_bool ();
void put_num ();
void put_str ();
void put_longname ();
void put_reset ();
void put_init ();
void put_init_internal ();
void setup_termcap ();
void split_args ();
void usage ();
void version ();

char *fgetline ();

extern char *version_string;

/* The name this program was run with, for error messages. */
char *program_name;

/* Take capability from stdin */
static int use_standard_input;

/* Use termcap name only */
static int use_termcap_only;

static struct option long_options[] =
{
  {"help", no_argument, 0, 'h'},
  {"standard-input", no_argument, 0, 'S'},
  {"terminal", required_argument, 0, 'T'},
  {"termcap", no_argument, 0, 't'},
  {"version", no_argument, 0, 'V'},
  {0, 0, 0, 0}
};

void
main (argc, argv)
     int argc;
     char **argv;
{
  char *term;			/* Terminal type. */
  int c;			/* Option character. */

  program_name = argv[0];
  term = getenv ("TERM");
  use_standard_input = 0;
  use_termcap_only = 0;

  while ((c = getopt_long (argc, argv, "T:SV",
			   long_options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case 'h':
	  usage (stdout, 0);
	  break;
	case 't':
	  use_termcap_only = 1;
	  break;
	case 'T':
	  term = optarg;
	  break;
	case 'S':
	  use_standard_input = 1;
	  break;
	case 'V':
	  version ();
	  break;
	default:
	  usage (stderr, 1);
	}
    }

  if ((use_standard_input && (optind != argc)) ||
      ((use_standard_input == 0) && (optind == argc)))
    usage (stderr, 1);

  setup_termcap (term);

  if (use_standard_input)
    {
      /* max number of args is 9 (sgr), even
	 though we use only 4 of them for now. */
      char *line;
      char *argbuf[10];
      int argcount;
      while ((line = fgetline (stdin)) != NULL)
	{
	  argcount = sizeof (argbuf) / sizeof (argbuf[0]);
	  split_args (line, argbuf, &argcount);
	  tput_one_item (argbuf, argcount);
	  free (line);
	}
    }
  else
    tput_one_item (argv + optind, argc - optind);
  exit (CAP_PRESENT);
}

void
tput_one_item (argv, argc)
     char **argv;
     int argc;
{
  struct conversion *conv;

  conv = find_info (argv[0], use_termcap_only);
  if (conv == NULL)
    {
      if (strcmp (argv[0], "longname") == 0)
	put_longname ();
      else if (strcmp (argv[0], "reset") == 0)
	put_reset ();
      else if (strcmp (argv[0], "init") == 0)
	put_init ();
      else
	{
	  error (MISSING_CAP, 0, "Unknown term%s capability `%s'",
		 use_termcap_only ? "cap" : "info", argv[0]);
	}
      if (use_standard_input)
	return;
      else
	exit (CAP_PRESENT);
    }

  if (conv->type & NUM)
    put_num (conv, use_standard_input);
  else if (conv->type & BOOL)
    put_bool (conv, use_standard_input);
  else
    put_str (conv, argv + 1, argc - 1);
}

void
put_num (conv)
     struct conversion *conv;
{
  if (use_standard_input)
    {
      error (USAGE_ERROR, 0,
	     "Number capability `%s' cannot be used with -S", conv->info);
    }
  printf ("%d\n", tgetnum (conv->cap));
}

void
put_bool (conv)
     struct conversion *conv;
{
  if (use_standard_input)
    {
      error(USAGE_ERROR, 0,
	    "Boolean capability `%s' cannot be used with -S", conv->info);
    }
  if (!tgetflag (conv->cap))
    exit (BOOLEAN_FALSE);
}

void
put_str (conv, argv, argc)
     struct conversion *conv;
     char **argv;
     int argc;
{
  char *string_value;		/* String capability. */
  int lines_affected;		/* Number of lines affected by capability. */

  string_value = tgetstr (conv->cap, (char **) NULL);
  if (string_value == NULL)
    if (use_standard_input)
      return;
    else
      exit (MISSING_CAP);

  if (!strcmp (conv->cap, "cm"))
    {
      if (BC == 0)
	BC = tgetstr ("le", (char **) NULL);
      if (UP == 0)
	UP = tgetstr ("up", (char **) NULL);

      /* The order of command-line arguments is `vertical horizontal'.
	 If horizontal is not given, it defaults to 0. */
      switch (argc)
	{
	case 0:
	  break;
	case 1:
	  string_value = tgoto (string_value, 0, atoi (argv[0]));
	  break;
	default:
	  string_value = tgoto (string_value,
				atoi (argv[1]), atoi (argv[0]));
	  break;
	}
    }
  else
    /* Although the terminfo `sgr' capability can take 9 parameters,
       the GNU tparam function only accepts up to 4.
       I don't know whether tparam could interpret an `sgr'
       capability reasonably even if it could accept that many
       parameters.  For now, we'll live with the 4-parameter limit. */
    switch (argc)
      {
      case 0:
	break;
      case 1:
	string_value = tparam (string_value, (char *) NULL, 0,
			       atoi (argv[0]));
	break;
      case 2:
	string_value = tparam (string_value, (char *) NULL, 0,
			       atoi (argv[0]),
			       atoi (argv[1]));
	break;
      case 3:
	string_value = tparam (string_value, (char *) NULL, 0,
			       atoi (argv[0]),
			       atoi (argv[1]),
			       atoi (argv[2]));
	break;
      default:
	string_value = tparam (string_value, (char *) NULL, 0,
			       atoi (argv[0]),
			       atoi (argv[1]),
			       atoi (argv[2]),
			       atoi (argv[3]));
	break;
      }

  /* Since we don't know where the cursor is, we have to be
     pessimistic for capabilities that need padding proportional to
     the number of lines affected, and tell them that the whole
     screen is affected.  */
  if (conv->type & PAD)
    lines_affected = tgetnum ("li");
  else
    lines_affected = 1;

  if (lines_affected < 1)
    lines_affected = 1;

  translations_off ();
  tputs (string_value, lines_affected, tcputchar);
  fflush (stdout);
  restore_translations ();
}

/* Output function for tputs.  */

int
tcputchar (c)
     char c;
{
  putchar (c & 0x7f); /* do we still need masking? */
  return c;
}

/* Although GNU termcap can dynamically allocate the buffer,
   we need to use static allocation to extract the longname
   ourselves. */
static char term_buffer[2048];

void
put_longname ()
{
  char *ep, *cp;
  for (ep = term_buffer; *ep && *ep != ':'; ep++)
    ;
  cp = (char *) xmalloc (ep - term_buffer + 1);
  strncpy (cp, term_buffer, ep - term_buffer);
  cp[ep - term_buffer] = 0;
  for (ep = &cp[ep - term_buffer] - 1; ep >= cp && *ep != '|'; ep--)
    ;
  if (ep >= cp && *ep == '|')
    puts (ep + 1);
  else
    puts (cp);
}

void
put_init_internal (reset)
     int reset;
{
  int need_xtabs;
  int lines, it;
  char *cap;

  need_xtabs = 0;

  lines = tgetnum ("li");
  if (lines <= 0)
    lines = 25;

  /* Disable OPOST */
  translations_off ();

  /* Output "rs" if we are resetting */
  cap = tgetstr ("rs", (char **) NULL);
  if (cap)
    tputs (cap, lines, tcputchar);

  /* Output "i1" */
  cap = tgetstr ("i1", (char **) NULL);
  if (cap)
    tputs (cap, lines, tcputchar);

  /* Output "is" */
  cap = tgetstr ("is", (char **) NULL);
  if (cap)
    tputs (cap, lines, tcputchar);

  /* Output "if" */
  cap = tgetstr ("if", (char **) NULL);
  if (cap)
    {
      char buf[1024];
      int fd;
      int nbytes;
      int errors;

      errors = 0;
      fd = open (cap, 0);
      if (fd >= 0)
	{
	  while ((nbytes = read (fd, buf, sizeof (buf))) > 0)
	    {
	      if (write (1, buf, nbytes) < 0)
		{
		  error (0, errno, "cannot write initialization string");
		  errors++;
		  break;
		}
	    }
	  close (fd);
	}
      else
	{
	  error (0, errno, "cannot read initialization file `%s'", cap);
	  errors++;
	}
      if (errors)
	{
	  restore_translations ();
	  exit (ERROR_EXIT);
	}
    }

  /* Output "i3" */
  cap = tgetstr ("i3", (char **) NULL);
  if (cap)
    tputs (cap, lines, tcputchar);

  /* Check "it".
       If it is not 8, then
         check "ct" and "st".
	 If both present, then
	   Output "ct", followed by `\r' (or `cr' if set),
	   followed by eight (8) spaces, followed by `st',...
	   until we reach `co' (or winsize if exist, or 80)
	 else
	   Remember that we should enable XTABS
       fi */

  it = tgetnum ("it");
  if (it < 0 || it == 8)
    ;				/* Assume standard setting is 8 */
  else
    {
      char *ct, *st;
      if ((ct = tgetstr ("ct", (char **) NULL)) != 0 &&
	  (st = tgetstr ("st", (char **) NULL)) != 0)
	{
	  int width, col;
	  char *cr;

	  width = tgetnum ("co");
	  if (width <= 0)
	    {
	      /* NEEDSWORK: we may want to find out how
		   wide the window is using newer tty structures. */
	      width = 80;
	    }
	  if ((cr = tgetstr ("cr", (char **) NULL)) == 0)
	    cr = "\r";
	  tputs (cr, lines, tcputchar);
	  for (col = 1; col < width; col += 8)
	    {
	      tputs ("        ", 1, tcputchar);
	      tputs (st, 1, tcputchar);
	    }
	}
      else
	need_xtabs = 1;
    }

  fflush (stdout);
  /* Enable OPOST. */
  restore_translations ();

  /* If we should enable XTABS, do it. */
  if (need_xtabs)
    enable_xtabs ();
}

void
put_reset ()
{
  put_init_internal (1);
}

void
put_init ()
{
  put_init_internal (0);
}

/* Read in the needed termcap strings for terminal type TERM. */

void
setup_termcap (term)
     char *term;
{
  char *tc_pc;			/* "pc" termcap string. */

  if (term == NULL)
    error (UNKNOWN_TERM, 0, "No value for $TERM and no -T specified");
  switch (tgetent (term_buffer, term))
    {
    case 0:
      error (UNKNOWN_TERM, 0, "Unknown terminal type `%s'", term);
    case -1:
      error (UNKNOWN_TERM, 0, "No termcap database");
    }

  tc_pc = tgetstr ("pc", (char **) NULL);
  PC = tc_pc ? *tc_pc : 0;
}

void
split_args (buf, argv, argc)
     char *buf;
     char **argv;
     int *argc;
{
  int maxargs = *argc - 1;
  int argindex = 0;

  while (*buf && *buf != '\n' && argindex < maxargs)
    {
      /* Skip leading blanks */
      while (*buf == ' ' || *buf == '\t')
	buf++;
      if (*buf == 0 || *buf == '\n')
	{
	  *argc = argindex;
	  return;
	}
      argv[argindex] = buf;
      /* Skip to next blank */
      while (*buf && *buf != '\n' && *buf != ' ' && *buf != '\t')
	buf++;
      if (*buf == 0)
	{
	  *argc = argindex;
	  return;
	}
      else
	{
	  *buf++ = 0;
	}
      argindex++;
    }
  *argc = argindex;
}

void
usage (stream, status)
     FILE *stream;
     int status;
{
  static char *how[] =
  {
    "Usage: [options] capability [parameters...]",
    "       [options] longname",
    "       [options] init",
    "       [options] reset",
    0,
  };
  static char *opts[] =
  {
    "       [-T termtype] [--terminal=termtype] : specify terminal type",
    "       [-t] [--termcap] : search only by termcap name",
    "       [-V] [--version] : print version information",
    "       [-S] [--standard-input] : read capabilities from standard input",
    0,
  };
  char **p;

  for (p = how; *p; p++)
    fprintf (stderr, "%s\n", *p);
  fprintf (stream, "Options:\n");
  for (p = opts; *p; p++)
    fprintf (stream, "%s\n", *p);
  if (status)
    exit (USAGE_ERROR);
  else
    exit (0);
}

void
version ()
{
  printf ("GNU tput version %s\n", version_string);
  exit (0);
}
