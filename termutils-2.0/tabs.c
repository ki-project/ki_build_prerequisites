/* tabs -- set terminal tabs
   Copyright (C) 1995 Free Software Foundation, Inc.

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

/* Usage: tabs [-T termtype] [-V] [-h] tabspec

   Options:

   -T termtype
   --terminal=termtype	Override $TERM.

   -V
   --version		Display version.

   -h
   --help		Help.

   GNU tabs accepts the following types of tab specification:

   n1,n2,...
           Set the TAB stops at positions n1, n2, and so on.

   -n      Set the TAB stops at intervals of n columns.
	   If n is zero, then all the TAB stops are cleared.

   -code
   -C code
   --code=code
           Set the TAB stops according to the canned TAB
           setting specified by code.  See fspec.def for
	   supported canned set of TAB stops.
	   The second form can be used if the code conflicts
	   with long options supported by GNU tabs.

   --filename
   -F filename
   --file=filename
	   Read the first line of the file specified by filename,
	   searching  for  a format specification.
	   Format specification is a blank separated sequence
	   of parameters and surrounded by <: and :>.
	   If this line contains a format specification,
	   set the TAB stops accordingly, otherwise
	   set them to every 8 columns.
	   The first form of this tabspec is a SYSVish way of
	   specifying a file.
	   The second form can be used if the filename conflicts
	   with long options supported by GNU tabs.

   GNU tabs uses the GNU termcap library.

   Junio Hamano <junio@twinsun.com> */

#include <config.h>

#if STDC_HEADERS
# include <stdlib.h>
#else
char *getenv ();
#endif

#include <stdio.h>
#include <termcap.h>
#include <errno.h>

#if STDC_HEADERS || HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
# define strchr index
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <tcutil.h>
#include <tabs.h>

#ifndef errno
extern int errno;
#endif

/* Number of elements in an array */
#define elemof(array) (sizeof (array) / sizeof (array[0]))

char *fgetline ();
void error ();

extern char *version_string;

/* Program name.  Used to report errors and usage */
char *program_name;

/* Width of the terminal */
static int term_width;

/* Convert a digit string to a positive integer.
   A character other than terminating NUL can be
   specified by stop.
   On return, next points to the next character
   to be scanned.
   Return negative if the string contains anything
   other than digits. */

static int
a_to_i (spec, next, stop)
     const char *spec;
     const char **next;
     int stop;
{
  int acc;

  acc = 0;
  while (*spec && *spec != stop)
    {
      if (*spec >= '0' && *spec <= '9')
	acc = acc * 10 + *spec++ - '0';
      else
	{
	  if (next)
	    *next = spec;
	  return (-1);
	}
    }
  if (next)
    if (*spec)
      *next = spec + 1;
    else
      *next = spec;
  return acc;
}

/* Parse an fspec line for a tabspec.  Return a pointer
   to the parameter character of spec if found and
   null terminate the spec.  Return 0 otherwise.
   Return the pointer used to scan the next item
   in *next, if returning non-null.

   A fspec line has a sequence of parameters separated
   by blanks and surrounded by <: and :>.  The parameters
   are
	tn1,n2,n3,...
	t-n
	t-code
	ssize
	mmargin
	d
	e
   though we are interested in only the first three cases.
 */

static char *
parse_fspec_line (line, first, next)
     char *line;
     int first;
     char **next;
{
  char *cp;
  int terminated;

  if (first)
    {
      /* If this is the first time, we have to find
	 the beginning of the fspec bracketed by '<:' */

      while (*line)
	if (line[0] == '<' && line[1] == ':')
	  break;
	else
	  line++;
      if (*line == 0)
	return 0;
      line += 2;
    }
  do
    {
      /* Skip leading blanks */
      while (*line && (*line == ' ' || *line == '\t'))
	line++;
      /* End of line or ':>' means the end of an fspec */
      if (*line == 0 ||
	  (*line == ':' && (line[1] == '>' || line[1] == 0)))
	return 0;
      switch (*line)
	{
	case 'e':
	case 'd':
	case 'm':
	case 's':
	case 't':
	  /* We seem to have found what we are looking for.
	     Let's make sure we have closing bracket before
	     returning.  Also, null terminate what we have
	     just found. */
	  terminated = 0;
	  for (cp = line; *cp; cp++)
	    if (*cp == ':' && cp[1] == '>')
	      {
		/* Ok, indeed there is a closing bracket.
		   In case the spec was the last item in the list
		   and there wasn't any blanks left (i.e. "<:... tspec:>"),
		   let's null terminate it here, too. */
		if (!terminated)
		  {
		    *cp = 0;
		    *next = 0;
		  }
		return line;
	      }
	    else if (!terminated && (*cp == ' ' || *cp == '\t'))
	      {
		*cp = 0;
		*next = cp + 1;
		terminated = 1;
	      }
	  break;
	default:
	  /* Skip to next blank */
	  while (*line && *line != ' ' && *line != '\t' &&
		 !(*line == ':' && line[1] == '>'))
	    line++;
	  if (*line != ' ' && *line != '\t')
	    return 0;
	}
    }
  while (1);
}

static void
usage (stream, status)
     FILE *stream;
     int status;
{
  static char *specs[] =
  {
    "n1,n2,...   : Set TAB stops at positions n1,n2,...",
    "-number     : Set TAB stops every number columns",
    "-C code     : Set TAB stops using canned settings",
    "-code       : Set TAB stops using canned settings",
    "-F filename : Set TAB stops according to the first line of the file",
    "--filename  : Set TAB stops according to the first line of the file",
    0,
  };
  char **p;
  struct fspec_table *fstp;
  int ix;

  fprintf (stream, "Usage: %s [-T termtype] [--terminal=termtype] tabspec\n",
	   program_name);
  for (p = specs; *p; p++)
    fprintf (stream, "  %s\n", *p);
  fprintf (stream, "Canned settings are:\n");
  for (fstp = fspec_table; fstp->code; fstp++)
    {
      fprintf (stream, "-%s\t", fstp->code);
      for (ix = 0; fstp->tabs[ix] != 0; ix++)
	fprintf (stream, "%d ", fstp->tabs[ix]);
      fprintf (stream, "(%s)\n", fstp->description);
    }
  exit (status);
}

/* Parse a tabspec and fill them into tabs array.  Return
   number of tab stops filled in.  Return negative on
   error. */

static int
parse_tabspec (spec, tabs, tabs_length)
     const char *spec;
     int *tabs;
     int tabs_length;
{
  const char *ispec;
  int tab, ix, width;

  ispec = spec;

  if (spec[0] != '-')
    {
      /* Has to be list of numbers */
      for (ix = 0;
	   ix < tabs_length && spec[0] != 0;
	   ix++)
	{
	  if (spec[0] == '+')
	    {
	      /* An increment */
	      if (ix == 0)
		goto bogus;
	      tab = a_to_i (spec + 1, &spec, ',');
	      if (tab <= 0)
		goto bogus;
	      tab = tabs[ix - 1] + tab;
	    }
	  else
	    tab = a_to_i (spec, &spec, ',');
	  if (tab <= 0 || (ix > 0 && tab < tabs[ix - 1]))
	    goto bogus;
	  tabs[ix] = tab;
	}
      return ix;
    }
  else
    {
      if (spec[1] >= '0' && spec[1] <= '9')
	{
	  /* Every N column */
	  width = a_to_i (&spec[1], spec, 0);
	  if (width < 0)
	    goto bogus;
	  else if (width == 0)
	    return 0;
	  else
	    {
	      for (tab = 1, ix = 0;
		   ix < tabs_length && tab <= term_width;
		   tab += width, ix++)
		tabs[ix] = tab;
	      return ix;
	    }
	}
      else
	{
	  /* Use canned tabs list */
	  for (tab = 0; fspec_table[tab].code != 0; tab++)
	    if (strcmp (fspec_table[tab].code, &spec[1]) == 0)
	      break;
	  if (fspec_table[tab].code != 0)
	    {
	      for (ix = 0;
		   ix < tabs_length;
		   ix++)
		{
		  tabs[ix] = fspec_table[tab].tabs[ix];
		  if (tabs[ix] <= 0)
		    break;
		}
	      return ix;
	    }
	  else
	    goto bogus;
	}
    }
bogus:
  error (0, 0, "invalid tab specification `%s'", ispec);
  usage (stderr, 1);
}

static int
tcputchar (c)
     char c;
{
  putchar (c);
  return c;
}

/* Set TAB stops.
   numtabs == 0 means clear all tabs */

static void
set_tabstops (term, tabs, numtabs)
     char *term;
     int *tabs;
     int numtabs;
{
  char *cr, *ct, *st;
  int lines, tab, ix, col;
  char *noclear =
  "cannot clear tabs on terminal %s";
  char *noset =
  "cannot set nonstandard hardware tabs on terminal %s";

  ct = tgetstr ("ct", 0);
  lines = tgetnum ("li");
  if (lines < 0)
    lines = 25;			/* Should be a reasonable default */

  if (numtabs == 0)
    {
      if (ct == 0)
	error (1, 0, noclear, term);
    }
  else
    {
      /* If both ct and st are not present, we are in trouble */
      if ((ct == 0) ||
	  (st = tgetstr ("st", 0)) == 0)
	{
	  /* If we are setting tabs to every 8 columns, then
	     we might be ok. */
	  for (tab = 1, ix = 0; ix < numtabs; ix++, tab += 8)
	    {
	      if (tabs[ix] != tab)
		error (1, 0, noset, term);
	    }
	  /* Return without doing anything; hope it's ok */
	  return;
	}
    }

  cr = tgetstr ("cr", 0);
  if (cr == 0)
    cr = "\r";

  /* Disable OPOST */
  translations_off ();

  tputs (cr, lines, tcputchar);	/* Bring cursor to column 1 */
  tputs (ct, lines, tcputchar);	/* Clear tabs */
  tputs (cr, lines, tcputchar);	/* Bring cursor to column 1 */

  if (numtabs)
    {
      for (ix = 0, col = 1;
	   ix < numtabs && col <= term_width;
	   ix++)
	{
	  while (tabs[ix] > col && col <= term_width)
	    {
	      putchar (' ');
	      col++;
	    }
	  tputs (st, 1, tcputchar);
	}
      tputs (cr, lines, tcputchar);
    }
  fflush (stdout);

  /* Enable OPOST */
  restore_translations ();

  /* Disable XTABS */
  disable_xtabs ();
}

/* Read the termcap entry for term and get the width
   of terminal. */

static void
prepare_termcap (term)
     char *term;
{
  char *tc_pc;

  if (term == NULL)
    error (1, 0, "No value for $TERM and no -T specified");
  switch (tgetent (0, term))
    {
    case 0:
      error (1, 0, "Unknown terminal type `%s'", term);
    case (-1):
      error (1, 0, "No termcap database");
    }
  tc_pc = tgetstr ("pc", 0);
  PC = tc_pc ? *tc_pc : 0;
  /* It leaks, but who cares */
  term_width = tgetnum ("co");
  if (term_width <= 0)
    term_width = 80;		/* Should be a reasonable default */
}

void
version ()
{
  printf ("GNU tabs version %s\n", version_string);
  exit (0);
}

static char *optval;

enum tabs_args
  {
    arg_termtype, arg_help, arg_version,
    arg_canned_tabs, arg_file,
    arg_every_n_column,
    arg_tablist, arg_bad
  };

/* Some of the SYSVish tabspecs look like short or long
   options, and even GNU getopt doesn't help us very much.
   This function parses each argument and returns what
   kind of option it is.  If the option takes an argument,
   optval either points to the argument, or NULL, in
   which case the argument is the next element in av[] */

static enum tabs_args
parse_an_arg (arg)
     char *arg;
{
  int arglen;

  switch (arg[0])
    {
    case '-':
      switch (arg[1])
	{
	case 'T':		/* short opt 'T' -- terminal */
	  if (arg[2])
	    optval = &arg[2];
	  else
	    optval = 0;
	  return arg_termtype;
	case '-':
	  /* longopt --terminal, --help, --version, --file,
	     or SYSVish --filename. */
	  switch (arg[2])
	    {
	    case 't':		/* --terminal? */
	    case 'h':		/* --help? */
	    case 'v':		/* --version? */
	    case 'f':		/* --file? */
	    case 'c':		/* --code? */
	      optval = strchr (&arg[3], '=');
	      if (optval)
		{
		  *optval++ = 0;
		  if (strncmp (&arg[2], "terminal", optval - &arg[3]) == 0)
		    return arg_termtype;
		  else if (strncmp (&arg[2], "file", optval - &arg[3]) == 0)
		    return arg_file;
		  else if (strncmp (&arg[2], "code", optval - &arg[3]) == 0)
		    return arg_canned_tabs;
		  /* --version and --help doesn't take any argument.
		     This must be a SYSVish --filename with a funny
		     filename that has an equal sign in it. */
		  optval[-1] = '=';
		}
	      else
		{
		  /* Since there is no equal sign, option argument (if any)
		     is the next argument */
		  optval = 0;
		  arglen = strlen (&arg[2]);

		  if (strncmp (&arg[2], "terminal", arglen) == 0)
		    return arg_termtype;
		  else if (strncmp (&arg[2], "file", arglen) == 0)
		    return arg_file;
		  else if (strncmp (&arg[2], "code", arglen) == 0)
		    return arg_canned_tabs;
		  else if (strncmp (&arg[2], "version", arglen) == 0)
		    return arg_version;
		  else if (strncmp (&arg[2], "help", arglen) == 0)
		    return arg_help;
		  /* Otherwise, this is SYSVish --filename */
		}
	      /* fall through */
	    default:		/* SYSVish --filename */
	      optval = &arg[2];
	      return arg_file;
	    }
	  break;
	case 'F':		/* short opt 'F' - file */
	  if (arg[2])
	    optval = &arg[2];
	  else
	    optval = 0;
	  return arg_file;
	case 'C':		/* short opt 'C' - code */
	  if (arg[2])
	    optval = &arg[2];
	  else
	    optval = 0;
	  return arg_canned_tabs;
	case 'h':		/* short opt 'h' - help */
	  return arg_help;
	case 'V':		/* short opt 'V' - version */
	  return arg_version;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  {
	    char *cp;
	    int bad;

	    for (bad = 0, cp = &arg[1]; *cp; cp++)
	      if (!(*cp >= '0' && *cp <= '9'))
		{
		  bad++;
		  break;
		}
	    if (bad == 0)
	      {
		optval = &arg[1];
		return arg_every_n_column;
	      }
	  }
	  /* fall through - this is SYSVish canned code
	     that begins with a digit (GNU tabs
	     currently doesn't define any canned code
	     that begins with a digit, though.) */
	default:		/* canned code */
	  optval = &arg[1];
	  return arg_canned_tabs;
	}
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      optval = arg;
      return arg_tablist;
    default:
      return arg_bad;
    }
}

int
main (argc, argv)
     int argc;
     char **argv;
{
  int *tabs;
  int every;
  int numtabs;
  int opt;
  char *tabspec;
  enum
    {
      spec_normal, spec_every, spec_file
    } tabspec_type;
  char *nonunique = "More than one tabspec specified";
  char *term;
  /* 1003.2 specifies the format of this message.  */
  char *required = "option requires an argument -- %c";

  program_name = argv[0];
  term = getenv ("TERM");
  tabs = 0;
  tabspec = 0;

  for (opt = 1; opt < argc; opt++)
    {
      switch (parse_an_arg (argv[opt]))
	{
	case arg_termtype:
	  if (optval)
	    term = optval;
	  else if (opt == argc - 1)
	    error (1, 0, required, 'T');
	  else
	    term = argv[++opt];
	  break;
	case arg_help:
	  usage (stdout, 0);
	case arg_version:
	  version ();
	case arg_every_n_column:
	  if (tabspec)
	    error (1, 0, nonunique);
	  tabspec = optval;
	  tabspec_type = spec_every;
	  break;
	case arg_canned_tabs:
	case arg_tablist:
	  if (tabspec)
	    error (1, 0, nonunique);
	  if (optval)
	    tabspec = optval;
	  else if (opt == argc - 1)
	    error (1, 0, required, 'C');
	  else
	    tabspec = argv[++opt];
	  tabspec_type = spec_normal;
	  break;
	case arg_file:
	  if (tabspec)
	    error (1, 0, nonunique);
	  if (optval)
	    tabspec = optval;
	  else if (opt == argc - 1)
	    error (1, 0, required, 'f');
	  else
	    tabspec = argv[++opt];
	  tabspec_type = spec_file;
	  break;
	case arg_bad:
	default:
	  usage (stderr, 1);
	}
    }

  if (tabspec == 0)
    /* No tabspec specified */
    usage (stderr, 1);

  if ((term == 0) || (term[0] == 0))
    /* POSIX tells us to output TAB setting sequence for
       ``unspecified default terminal type'' in this case
       instead of reporting an error.
       We decided that TAB means very very little on our
       default terminal. ;-) */
    exit (0);

  prepare_termcap (term);

  /* At most term_width TAB stops can be set for
     a terminal of width term_width */
  tabs = (int *) xmalloc (sizeof (int) * term_width);

  switch (tabspec_type)
    {
    case spec_every:
      /* parse_an_arg has already verified that
	 tabspec is a digits-only string */
      every = a_to_i (tabspec, 0, 0);
    every_n_column:
      if (every == 0)
	/* Clearing tabs */
	numtabs = 0;
      else
	{
	  int col, ix;
	  numtabs = term_width / every;
	  for (ix = 0, col = 1;
	       col <= term_width;
	       ix++, col += every)
	    tabs[ix] = col;
	}
      break;
    case spec_file:
      {
	/* Read the first line of the file */
	FILE *fp;
	char *line;
	fp = fopen (tabspec, "r");
	if (!fp)
	  {
	    error (0, errno, "cannot open `%s'", tabspec);
	    return (-1);
	  }

	line = fgetline (fp);
	fclose (fp);

	/* SYSVish --filename resets TAB stops to every 8
	   columns, if an fspec line is not found */
	if (line == 0)
	  {
	    every = 8;
	    goto every_n_column;
	  }
	else
	  {
	    int first;
	    char *next;

	    first = 1;
	    tabspec = line;
	    while ((tabspec = parse_fspec_line (tabspec, first, &next)) != 0)
	      {
		first = 0;
		if (*tabspec == 't')
		  {
		    numtabs = parse_tabspec (tabspec + 1, tabs, term_width);
		    free (line);
		    goto set_tabs;
		  }
		else
		  tabspec = next;
	      }
	    free (line);
	    /* There was no specification.  Set it to default. */
	    every = 8;
	    goto every_n_column;
	  }
      }
      break;
    default:
      numtabs = parse_tabspec (tabspec, tabs, term_width);
      if (numtabs < 0)
	usage (stderr, 1);
      break;
    }
set_tabs:
  set_tabstops (term, tabs, numtabs);
  exit (0);
}
