/* vi:set ts=8 sts=4 sw=4:
 *
 * Copyright (C) 2004 Xavier de Gaye.
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
 *
 * $Id: misc.c 230 2009-11-28 16:50:31Z xavier $
 */

#ifdef HAVE_CLEWN
# include <config.h>
#else
# include <auto/config.h>
void vim_beep ();
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef HAVE_READLINE_READLINE_H
# include <readline/readline.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if !defined(HAVE_SYS_TIME_H) || defined(TIME_WITH_SYS_TIME)
# include <time.h>	    /* on some systems time.h should not be
			       included together with sys/time.h */
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#ifndef HAVE_SELECT
# ifdef HAVE_SYS_POLL_H
#  include <sys/poll.h>
# else
#  ifdef HAVE_POLL_H
#   include <poll.h>
#  endif
# endif
#endif

/* Note: Some systems need both string.h and strings.h (Savage).  However,
 * some systems can't handle both, only use string.h in that case. */
#if defined(HAVE_STRINGS_H) && !defined(NO_STRINGS_WITH_STRING_H)
# include <strings.h>
#endif

#include "obstack.h"
#ifdef FEAT_GDB
# include "vim.h"
#else
# include "clewn.h"
#endif

/* do not define xmalloc and family */
#undef GDB_MTRACE
#include "gdb.h"

#include "misc.h"

#if defined(__CYGWIN__) || defined(__CYGWIN32__)
# define WIN32UNIX	/* Compiling for Win32 using Unix files. */
# define BINARY_FILE_IO
# undef HAVE_FCHDIR	/* doesn't work well in most versions */
#endif

#if defined(HAVE_GETCWD) && defined(HAVE_GETWD)
# define USE_GETCWD
#endif

/* boolean constants */
#define FALSE	0
#define TRUE	1

/*
 * EMX doesn't have a global way of making open() use binary I/O.
 * Use O_BINARY for all open() calls.
 */
#if defined(__EMX__) || defined(__CYGWIN32__)
# define O_EXTRA    O_BINARY
#else
# define O_EXTRA    0
#endif

static void (*misc_abort)(void) = NULL;	    /* registered abort function */

/* Register an abort function for when allocating memory fails. */
    void
xatabort(abort_func)
    void (*abort_func)(void);
{
    misc_abort = abort_func;
}

/* allocate memory */
    void *
xmalloc(size)
    size_t size;
{
    void *value = malloc(size);

    if (value == 0)
    {
#ifdef HAVE_CLEWN
	rl_cleanup_after_signal();	/* have readline reset terminal */
	fprintf(stderr, "\nvirtual memory exhausted\n");
#endif
	if (misc_abort != NULL)
	    misc_abort();
	else
	    exit(EXIT_FAILURE);
    }
    return value;
}

/* Allocate memory and set all bytes to zero */
    void *
xcalloc(size)
    size_t size;
{
    void *p = xmalloc(size);
    clewn_memset(p, 0, size);
    return p;
}

/* Changes the size of the memory block pointed to by ptr to size bytes. */
    void *
xrealloc(ptr, size)
    void *ptr;
    size_t size;
{
    void *value = realloc(ptr, size);

    if (value == 0)
    {
#ifdef HAVE_CLEWN
	rl_cleanup_after_signal();	/* have readline reset terminal */
	fprintf(stderr, "\nvirtual memory exhausted\n");
#endif
	if (misc_abort != NULL)
	    misc_abort();
	else
	    exit(EXIT_FAILURE);
    }
    return value;
}

/* Replacement for free() that ignores NULL pointers. */
    void
xfree(x)
    void *x;
{
    if (x != NULL)
	free(x);
}

#ifndef HAVE_MEMSET
    void *
clewn_memset(ptr, c, size)
    void *ptr;
    int c;
    size_t size;
{
    char *p = ptr;

    while (size-- > 0)
	*p++ = c;
    return ptr;
}
#endif

#ifdef CLEWN_MEMMOVE
/*
 * Version of memmove() that handles overlapping source and destination.
 * For systems that don't have a function that is guaranteed to do that (SYSV).
 */
    void
clewn_memmove(d, s, len)
    void *d;
    void *s;
    size_t len;
{
    /* a void doesn't have a size, we use char pointers */
    char *dst = d;
    char *src = s;

    /* overlap, copy backwards */
    if (dst > src && dst < src + len)
    {
	src += len;
	dst += len;
	while (len-- > 0)
	    *--dst = *--src;
    }
    else    /* copy forwards */
	while (len-- > 0)
	    *dst++ = *src++;
}
#endif

/* copy a string into newly allocated memory */
    char *
clewn_strsave(string)
    char *string;
{
    char *p;
    size_t len;

    if (string != NULL) {
	len = strlen(string) + 1;
	p = xmalloc(len);
	clewn_memmove(p, string, len);
    }
    else {
	p = xmalloc(1);
	*p = '\0';
    }
    return p;
}

    char *
clewn_strnsave(string, len)
    char *string;
    size_t len;
{
    char *p = xmalloc(len + 1);

    *p = '\0';
    if (string != NULL)
	strncpy(p, string, len);
    *(p + len) = '\0';
    return p;
}

    void
clewn_sleep(msec)
    int msec;
{
    /*
     * Everybody sleeps in a different way...
     * Prefer nanosleep(), some versions of usleep() can only sleep up to
     * one second.
     */
#ifdef HAVE_NANOSLEEP
    {
	struct timespec ts;

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	(void)nanosleep(&ts, NULL);
    }
#else
# ifdef HAVE_USLEEP
    while (msec >= 1000)
    {
	usleep((999 * 1000));
	msec -= 999;
    }
    usleep((msec * 1000));
# else
#  ifndef HAVE_SELECT
    poll(NULL, 0, msec);
#  else
#   ifdef __EMX__
    _sleep2(msec);
#   else
    {
	struct timeval tv;

	tv.tv_sec = msec / 1000;
	tv.tv_usec = (msec % 1000) * 1000;
	/*
	 * NOTE: Solaris 2.6 has a bug that makes select() hang here.  Get
	 * a patch from Sun to fix this.  Reported by Gunnar Pedersen.
	 */
	select(0, NULL, NULL, NULL, &tv);
    }
#   endif /* __EMX__ */
#  endif /* HAVE_SELECT */
# endif /* HAVE_NANOSLEEP */
#endif /* HAVE_USLEEP */
}

/* Give a warning for an error. */
    void
clewn_beep()
{
#ifdef HAVE_CLEWN
    /* use readline utility */
# ifdef HAVE_RL_DING
    rl_ding();
# else
    ding();	/* deprecated */
# endif
#else
    vim_beep();
#endif
}

/*
 * Get name of current directory into buffer 'buf' of length 'len' bytes.
 * Return 1 for success, 0 for failure.
 */
    int
clewn_getwd(buf, len)
    char *buf;
    int len;
{
#if defined(USE_GETCWD)
    if (getcwd(buf, len) == NULL)
    {
	strcpy(buf, strerror(errno));
	return 0;
    }
    return 1;
#else
    return (getwd(buf) != NULL ? 1 : 0);
#endif
}

/*
 * Get absolute file name into buffer 'buf' of length 'len' bytes.
 * return 0 for failure, 1 for success
 */
    int
clewn_fullpath(fname, buf, len, force)
    char *fname;
    char *buf;
    int len;
    int force;		/* also expand when already absolute path */
{
    int		l;
#ifdef HAVE_FCHDIR
    int		fd = -1;
    static int	dont_fchdir = FALSE;	/* TRUE when fchdir() doesn't work */
#endif
    char	olddir[MAXPATHL];
    char	*p;
    int		retval = 1;

    *buf = '\0';
    if (fname == NULL)
	return 0;

    /* expand it if forced or not an absolute path */
    if (force || *fname != '/')
    {
	/*
	 * If the file name has a path, change to that directory for a moment,
	 * and then do the getwd() (and get back to where we were).
	 * This will get the correct path name with "../" things.
	 */
	if ((p = strrchr(fname, '/')) != NULL)
	{
#ifdef HAVE_FCHDIR
	    /*
	     * Use fchdir() if possible, it's said to be faster and more
	     * reliable.  But on SunOS 4 it might not work.  Check this by
	     * doing a fchdir() right now.
	     */
	    if (!dont_fchdir)
	    {
		fd = open(".", O_RDONLY | O_EXTRA, 0);
		if (fd >= 0 && fchdir(fd) < 0)
		{
		    close(fd);
		    fd = -1;
		    dont_fchdir = TRUE;	    /* don't try again */
		}
	    }
#endif

	    /* Only change directory when we are sure we can return to where
	     * we are now.  After doing "su" chdir(".") might not work. */
	    if (
#ifdef HAVE_FCHDIR
		fd < 0 &&
#endif
			(clewn_getwd(olddir, MAXPATHL) == 0
					   || chdir((char *)olddir) != 0))
	    {
		p = NULL;	/* can't get current dir: don't chdir */
		retval = 0;
	    }
	    else
	    {
		/* The directory is copied into buf[], to be able to remove
		 * the file name without changing it (could be a string in
		 * read-only memory) */
		if (p - fname >= len)
		    retval = 0;
		else
		{
		    strncpy(buf, fname, p - fname);
		    buf[p - fname] = '\0';
		    if (chdir((char *)buf))
			retval = 0;
		    else
			fname = p + 1;
		    *buf = '\0';
		}
	    }
	}

	if (clewn_getwd(buf, len) == 0)
	{
	    retval = 0;
	    *buf = '\0';
	}

	if (p != NULL)
	{
#ifdef HAVE_FCHDIR
	    if (fd >= 0)
	    {
		(void)fchdir(fd);
		close(fd);
	    }
	    else
#endif
		(void)chdir((char *)olddir);
	}

	l = strlen(buf);
	if (l >= len)
	    retval = 0;
	else
	{
	    if (l > 0 && buf[l - 1] != '/' && *fname != '\0')
		strcat(buf, "/");
	}
    }

    /* catch file names which are too long */
    if (retval == 0 || ((int)(strlen(buf) + strlen(fname)) >= len))
	return 0;

    strcat(buf, fname);

    return 1;
}

/*
 * Get a full path name for the file named 'name'.
 * If name is an absolute path, just stat it. Otherwise, add name to
 * each directory in GDB source directories and stat the result.
 */
    char *
get_fullpath(name, sourcedir, source_cur, source_list, obs)
    char *name;	        /* file name */
    char *sourcedir;	/* GDB source directories */
    char *source_cur;	/* GDB current source */
    char *source_list;  /* GDB source list */
    struct obstack *obs;
{
    char *pathname;
    char pathbuf[MAXPATHL];
    struct stat st;
    char *dir;
    char *ptr;
    char *last;
    char *hay;
    char *found;
    char *end;

    if (name == NULL || *name == NUL)
	return NULL;

    /* an absolute path name */
    if (*name == '/')
    {
	if (stat((char *)name, &st) == 0)
	    return name;
	else
	    /* strip off the directory part and continue */
	    name = strrchr(name, '/') + 1;
    }

    if (sourcedir == NULL)		    /* use current working directory */
	sourcedir = GDB_CWD;

    /* proceed with each directory in GDB source directories */
    ptr = sourcedir;
    do
    {
	if ((last = strchr(ptr, ':')) != NULL)
	    *last++ = NUL;

	if (strcmp(ptr, GDB_CDIR) == 0)	    /* compilation directory */
	{
	    /* hay: file="NAME",fullname=" */
	    obstack_strcat(obs, "file=\"");
	    OBSTACK_STRCAT(obs, name);
	    obstack_strcat0(obs, "\",fullname=\"");
	    hay = (char *)obstack_finish(obs);

	    /* name is the current sourcefile: use gdb compilation directory */
	    if (source_cur != NULL && (found=strstr(source_cur, hay)) != NULL ){
		found += strlen(hay);
		if ((end=strstr(found, "\"")) != NULL)
		    return (char *)obstack_copy0(obs, found, end - found);
	    }

	    /* lookup for first occurence of name in source list */
	    if (source_list != NULL && (found=strstr(source_list, hay)) != NULL ){
		found += strlen(hay);
		if ((end=strstr(found, "\"")) != NULL)
		    return (char *)obstack_copy0(obs, found, end - found);
	    }

	    dir = NULL;
	}
	else if (strcmp(ptr, GDB_CWD) == 0) /* current working directory */
	{
	    if (clewn_getwd(pathbuf, MAXPATHL))
		dir = pathbuf;
	    else
		dir = NULL;
	}
	else
	    dir = ptr;

	if (dir != NULL)
	{
	    OBSTACK_STRCAT(obs, dir);
	    obstack_strcat(obs, "/");
	    OBSTACK_STRCAT0(obs, name);
	    pathname = (char *)obstack_finish(obs);

	    if (stat((char *)pathname, &st) == 0) {
		/* need to handle the case where the path includes '..'
		 * for example "/home/xavier/tmp/.." must be converted
		 * to "/home/xavier" */
		if (clewn_fullpath(pathname, pathbuf, MAXPATHL, TRUE))
		    return (char *)obstack_strsave(obs, pathbuf);
	    }
	}

	ptr = last;
    } while (ptr != NULL && *ptr != NUL);

    return NULL;
}

/*
 * Execute a program with (optionnaly quoted) arguments
 */
    void
clewn_exec(cmd)
    char *cmd;
{
    char ** argv = NULL;	/* keep compiler happy */
    char * newcmd;
    int argc;
    char * ptr;
    int inquote;
    int i;

    if ((newcmd = clewn_strsave(cmd)) == NULL)
	return;

    /*
     * step 1: find number of arguments
     * step 2: separate them and built argv[]
     */
    for (i = 0; i < 2; i++)
    {
	ptr = newcmd;
	argc = 0;

	for (;;)
	{
	    inquote = FALSE;

	    while (*ptr == ' ' || *ptr == '\t')	/* skip to next non-white */
		ptr++;

	    if (*ptr == '\0')
		break;

	    if (*ptr == '"' || *ptr == '\'')
	    {
		ptr++;
		inquote = TRUE;
	    }

	    if (i == 1)
		argv[argc] = ptr;

	    argc++;
	    while (*ptr && (inquote || (*ptr != ' ' && *ptr != '\t')))
	    {
		if (*ptr == '"' || *ptr == '\'')
		    break;

		ptr++;
	    }

	    if (*ptr == '\0')
		break;

	    if (i == 1)
		*ptr = '\0';

	    ptr++;
	}

	/* got the number of arguments */
	if (i == 0 && (argv = (char **) xcalloc((size_t)((argc + 1) * sizeof(char *)))) == NULL)
	    return;
    }

    execvp(argv[0], argv);
}
