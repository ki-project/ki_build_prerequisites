/* Utility functions to work with termutils.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tcutil.h>
#include <termcap.h>
#define ERROR_EXIT 5		/* Real error or signal. */

/* We would appreciate it if somebody using a system with
   termio but no termios nor sgtty could add termio support
   for the following macros - junio@twinsun.com */

#if HAVE_TERMIOS_H
#include <termios.h>
typedef struct termios tty_mode;
#ifdef HAVE_TCGETATTR
#define GET_TTY_MODE(fd,buf) tcgetattr ((fd), (buf))
#define SET_TTY_MODE(fd,buf) tcsetattr ((fd), TCSANOW, (buf))
#define GET_OSPEED(buf) cfgetospeed ((buf))
#else /* not HAVE_TCGETATTR */
#define GET_TTY_MODE(fd,buf) ioctl ((fd), TCGETS, (buf))
#define SET_TTY_MODE(fd,buf) ioctl ((fd), TCSETS, (buf))
#define GET_OSPEED(buf) ((buf)->c_cflag & CBAUD)
#endif /* not HAVE_TCGETATTR */
#define DISABLE_OPOST(buf) ((buf)->c_oflag &= ~OPOST)
#if !defined(XTABS) && defined(TAB3)
#define XTABS TAB3
#endif /* !defined(XTABS) && defined(TAB3) */
#if defined(XTABS)
#define ENABLE_XTABS(buf) ((buf)->c_oflag |= XTABS)
#define DISABLE_XTABS(buf) ((buf)->c_oflag &= ~XTABS)
#else /* not defined(XTABS) */
#define ENABLE_XTABS(buf)	/* empty */
#define DISABLE_XTABS(buf)	/* empty */
#endif /* not defined(XTABS) */
#else /* not HAVE_TERMIOS_H */
#if HAVE_SGTTY_H
#include <sgtty.h>
typedef struct sgttyb tty_mode;
#define GET_TTY_MODE(fd,buf) gtty ((fd), (buf))
#define SET_TTY_MODE(fd,buf) stty ((fd), (buf))
#define GET_OSPEED(buf) (buf)->sg_ospeed
#define DISABLE_OPOST(buf) ((buf)->sg_flags &= ~(XTABS|LCASE|CRMOD))
#define ENABLE_XTABS(buf) ((buf)->sg_flags |= XTABS)
#define DISABLE_XTABS(buf) ((buf)->sg_flags &= ~XTABS)
#else /* not HAVE_SGTTY_H */
#define NO_TTY_CONTROL
#endif /* not HAVE_SGTTY_H */
#endif /* not HAVE_TERMIOS_H */

#include <signal.h>

#ifdef NO_TTY_CONTROL
void
translations_off ()
{
  ;				/* empty */
}

void
restore_translations ()
{
  ;				/* empty */
}

void
enable_xtabs ()
{
  ;				/* empty */
}

void
disable_xtabs ()
{
  ;				/* empty */
}
#else
static tty_mode old_modes, new_modes;

static RETSIGTYPE
signal_handler ()
{
  restore_translations ();
  exit (ERROR_EXIT);
}
/* Turn off expansion of tabs into spaces, saving the old
   terminal settings first.
   Also set OSPEED.  */

void
translations_off ()
{
  if (isatty (1))
    {
      GET_TTY_MODE (1, &old_modes);

      if (signal (SIGINT, SIG_IGN) != SIG_IGN)
	signal (SIGINT, signal_handler);
      if (signal (SIGHUP, SIG_IGN) != SIG_IGN)
	signal (SIGHUP, signal_handler);
      if (signal (SIGQUIT, SIG_IGN) != SIG_IGN)
	signal (SIGQUIT, signal_handler);
      signal (SIGTERM, signal_handler);

      new_modes = old_modes;

      DISABLE_OPOST (&new_modes);
      SET_TTY_MODE (1, &new_modes);
      ospeed = GET_OSPEED (&old_modes);
    }
  else
    ospeed = 0;
}

/* Restore the old terminal settings.  */

void
restore_translations ()
{
  SET_TTY_MODE (1, &old_modes);
}

void
enable_xtabs ()
{
  new_modes = old_modes;
  ENABLE_XTABS (&new_modes);
  SET_TTY_MODE (1, &new_modes);
}

void
disable_xtabs ()
{
  new_modes = old_modes;
  DISABLE_XTABS (&new_modes);
  SET_TTY_MODE (1, &new_modes);
}
#endif
