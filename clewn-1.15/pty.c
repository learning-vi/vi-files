/* vi:set ts=8 sts=4 sw=4:
 * pty.c,v 1.3 2004/06/07 09:20:40 xavier Exp
 */

/*
 * The stuff in this file mostly comes from the "screen" program.
 *
 * Copyright (c) 1993
 *	Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *	Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#include <config.h>

#include <signal.h>
#include <string.h>

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_TERMIOS_H
# include <termios.h>
#else
# include <termio.h>
#endif

/* sun's sys/ioctl.h redefines symbols from termio world */
#if defined(HAVE_SYS_IOCTL_H) && !defined(sun)
# include <sys/ioctl.h>
#endif

#if HAVE_STROPTS_H
#include <sys/types.h>
#ifdef sinix
#define buf_T __system_buf_t__
#endif
#include <stropts.h>
#ifdef sinix
#undef buf_T
#endif
# ifdef sun
#  include <sys/conf.h>
# endif
#endif

#if HAVE_SYS_STREAM_H
# include <sys/stream.h>
#endif

#if HAVE_SYS_PTEM_H
# include <sys/ptem.h>
#endif

#if defined(sun) && defined(LOCKPTY) && !defined(TIOCEXCL)
# include <sys/ttold.h>
#endif

#ifdef ISC
# include <sys/tty.h>
# include <sys/sioctl.h>
# include <sys/pty.h>
#endif

#ifdef sgi
# include <sys/sysmacros.h>
#endif

#if defined(_INCLUDE_HPUX_SOURCE) && !defined(hpux)
# define hpux
#endif

#ifndef __ARGS
    /* The AIX VisualAge cc compiler defines __EXTENDED__ instead of __STDC__
     * because it includes pre-ansi features. */
# if defined(__STDC__) || defined(__GNUC__) || defined(__EXTENDED__)
#  define __ARGS(x) x
# else
#  define __ARGS(x) ()
# endif
#endif

/*
 * EMX doesn't have a global way of making open() use binary I/O.
 * Use O_BINARY for all open() calls.
 */
#if defined(__EMX__) || defined(__CYGWIN32__)
# define O_EXTRA    O_BINARY
#else
# define O_EXTRA    0
#endif

/*
 * if no PTYRANGE[01] is in the config file, we pick a default
 */
#ifndef PTYRANGE0
# define PTYRANGE0 "qprs"
#endif
#ifndef PTYRANGE1
# define PTYRANGE1 "0123456789abcdef"
#endif

/* SVR4 pseudo ttys don't seem to work with SCO-5 */
#ifdef M_UNIX
# undef HAVE_SVR4_PTYS
#endif

static void initmaster __ARGS((int));

/*
 *  Open all ptys with O_NOCTTY, just to be on the safe side
 *  (RISCos mips breaks otherwise)
 */
#ifndef O_NOCTTY
# define O_NOCTTY 0
#endif

    static void
initmaster(f)
    int f;
{
#ifndef VMS
# ifdef POSIX
    tcflush(f, TCIOFLUSH);
# else
#  ifdef TIOCFLUSH
    (void)ioctl(f, TIOCFLUSH, (char *) 0);
#  endif
# endif
# ifdef LOCKPTY
    (void)ioctl(f, TIOCEXCL, (char *) 0);
# endif
#endif
    f = 0;  /* keep gcc happy */
}

/*
 * This causes a hang on some systems, but is required for a properly working
 * pty on others.  Needs to be tuned...
 */
    int
SetupSlavePTY(int fd)
{
    if (fd < 0)
	return 0;
#if defined(I_PUSH) && defined(HAVE_SVR4_PTYS) && !defined(sgi) && !defined(linux) && !defined(__osf__) && !defined(M_UNIX)
# if defined(HAVE_SYS_PTEM_H) || defined(hpux)
    if (ioctl(fd, I_PUSH, "ptem") != 0)
	return -1;
# endif
    if (ioctl(fd, I_PUSH, "ldterm") != 0)
	return -1;
# ifdef sun
    if (ioctl(fd, I_PUSH, "ttcompat") != 0)
	return -1;
# endif
#endif
    return 0;
}


#if defined(OSX) && !defined(PTY_DONE)
#define PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    int		f;
    static char TtyName[32];

    if ((f = open_controlling_pty(TtyName)) < 0)
	return -1;
    initmaster(f);
    *ttyn = TtyName;
    return f;
}
#endif

#if (defined(sequent) || defined(_SEQUENT_)) && defined(HAVE_GETPSEUDOTTY) \
	&& !defined(PTY_DONE)
#define PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    char	*m, *s;
    int		f;
    /* used for opening a new pty-pair: */
    static char PtyName[32];
    static char TtyName[32];

    if ((f = getpseudotty(&s, &m)) < 0)
	return -1;
#ifdef _SEQUENT_
    fvhangup(s);
#endif
    strncpy(PtyName, m, sizeof(PtyName));
    strncpy(TtyName, s, sizeof(TtyName));
    initmaster(f);
    *ttyn = TtyName;
    return f;
}
#endif

#if defined(__sgi) && !defined(PTY_DONE)
#define PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    int f;
    char *name;
    void  (*sigcld)(int);

    /*
     * SIGCHLD set to SIG_DFL for _getpty() because it may fork() and
     * exec() /usr/adm/mkpts
     */
    sigcld = signal(SIGCHLD, SIG_DFL);
    name = _getpty(&f, O_RDWR | O_NONBLOCK | O_EXTRA, 0600, 0);
    signal(SIGCHLD, sigcld);

    if (name == 0)
	return -1;
    initmaster(f);
    *ttyn = name;
    return f;
}
#endif

#if defined(MIPS) && defined(HAVE_DEV_PTC) && !defined(PTY_DONE)
#define PTY_DONE
    int
OpenPTY(ttyn)
    char **ttyn;
{
    int		f;
    struct stat buf;
    /* used for opening a new pty-pair: */
    static char TtyName[32];

    if ((f = open("/dev/ptc", O_RDWR | O_NOCTTY | O_NONBLOCK | O_EXTRA, 0)) < 0)
	return -1;
    if (mch_fstat(f, &buf) < 0)
    {
	close(f);
	return -1;
    }
    sprintf(TtyName, "/dev/ttyq%d", minor(buf.st_rdev));
    initmaster(f);
    *ttyn = TtyName;
    return f;
}
#endif

#if defined(HAVE_SVR4_PTYS) && !defined(PTY_DONE) && !defined(hpux)

/* NOTE: Even though HPUX can have /dev/ptmx, the code below doesn't work! */
#define PTY_DONE
    int
OpenPTY(char ** ttyn)
{
    int		f;
    char	*m, *ptsname(int fd);
    int unlockpt __ARGS((int)), grantpt __ARGS((int));
    void  (*sigcld)(int);
    /* used for opening a new pty-pair: */
    static char TtyName[32];

    if ((f = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_EXTRA, 0)) == -1)
	return -1;

    /*
     * SIGCHLD set to SIG_DFL for grantpt() because it fork()s and
     * exec()s pt_chmod
     */
    sigcld = signal(SIGCHLD, SIG_DFL);
    if ((m = ptsname(f)) == NULL || grantpt(f) || unlockpt(f))
    {
	signal(SIGCHLD, sigcld);
	close(f);
	return -1;
    }
    signal(SIGCHLD, sigcld);
    strncpy(TtyName, m, sizeof(TtyName));
    initmaster(f);
    *ttyn = TtyName;
    return f;
}
#endif

#if defined(_AIX) && defined(HAVE_DEV_PTC) && !defined(PTY_DONE)
#define PTY_DONE

#ifdef _IBMR2
int aixhack = -1;
#endif

    int
OpenPTY(ttyn)
    char **ttyn;
{
    int		f;
    /* used for opening a new pty-pair: */
    static char TtyName[32];

    /* a dumb looking loop replaced by mycrofts code: */
    if ((f = open("/dev/ptc", O_RDWR | O_NOCTTY | O_EXTRA)) < 0)
	return -1;
    strncpy(TtyName, ttyname(f), sizeof(TtyName));
    if (geteuid() && access(TtyName, R_OK | W_OK))
    {
	close(f);
	return -1;
    }
    initmaster(f);
# ifdef _IBMR2
    if (aixhack >= 0)
	close(aixhack);
    if ((aixhack = open(TtyName, O_RDWR | O_NOCTTY | O_EXTRA, 0)) < 0)
    {
	close(f);
	return -1;
    }
# endif
    *ttyn = TtyName;
    return f;
}
#endif

#ifndef PTY_DONE

# ifdef hpux
static char PtyProto[] = "/dev/ptym/ptyXY";
static char TtyProto[] = "/dev/pty/ttyXY";
# else
#  ifdef __BEOS__
static char PtyProto[] = "/dev/pt/XY";
static char TtyProto[] = "/dev/tt/XY";
#  else
static char PtyProto[] = "/dev/ptyXY";
static char TtyProto[] = "/dev/ttyXY";
#  endif
# endif

    int
OpenPTY(ttyn)
    char **ttyn;
{
    char	*p, *q, *l, *d;
    int		f;
    /* used for opening a new pty-pair: */
    static char PtyName[32];
    static char TtyName[32];

    strcpy(PtyName, PtyProto);
    strcpy(TtyName, TtyProto);
    for (p = PtyName; *p != 'X'; p++)
	;
    for (q = TtyName; *q != 'X'; q++)
	;
    for (l = PTYRANGE0; (*p = *l) != '\0'; l++)
    {
	for (d = PTYRANGE1; (p[1] = *d) != '\0'; d++)
	{
#if !defined(MACOS) || defined(USE_CARBONIZED)
	    if ((f = open(PtyName, O_RDWR | O_NOCTTY | O_EXTRA, 0)) == -1)
#else
	    if ((f = open(PtyName, O_RDWR | O_NOCTTY | O_EXTRA)) == -1)
#endif
		continue;
	    q[0] = *l;
	    q[1] = *d;
#ifndef MACOS
	    if (geteuid() && access(TtyName, R_OK | W_OK))
	    {
		close(f);
		continue;
	    }
#endif
#if defined(sun) && defined(TIOCGPGRP) && !defined(SUNOS3)
	    /* Hack to ensure that the slave side of the pty is
	     * unused. May not work in anything other than SunOS4.1
	     */
	    {
		int pgrp;

		/* tcgetpgrp does not work (uses TIOCGETPGRP)! */
		if (ioctl(f, TIOCGPGRP, (char *)&pgrp) != -1 || errno != EIO)
		{
		    close(f);
		    continue;
		}
	    }
#endif
	    initmaster(f);
	    *ttyn = TtyName;
	    return f;
	}
    }
    return -1;
}
#endif
