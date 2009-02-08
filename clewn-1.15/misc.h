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
 * $Id: misc.h 148 2007-07-21 16:35:40Z xavier $
 */

#ifndef MISC_H
# define MISC_H

#ifndef __ARGS
    /* The AIX VisualAge cc compiler defines __EXTENDED__ instead of __STDC__
     * because it includes pre-ansi features. */
# if defined(__STDC__) || defined(__GNUC__) || defined(__EXTENDED__)
#  define __ARGS(x) x
# else
#  define __ARGS(x) ()
# endif
#endif

/* memmove is not present on all systems, use memmove, bcopy, memcpy or our
 * own version */
/* Some systems have (void *) arguments, some (char *). If we use (char *) it
 * works for all */
#ifdef USEMEMMOVE
# define clewn_memmove(to, from, len) memmove((char *)(to), (char *)(from), len)
#else
# ifdef USEBCOPY
#  define clewn_memmove(to, from, len) bcopy((char *)(from), (char *)(to), len)
# else
#  ifdef USEMEMCPY
#   define clewn_memmove(to, from, len) memcpy((char *)(to), (char *)(from), len)
#  else
#   define CLEWN_MEMMOVE
void clewn_memmove __ARGS((void *, void *, size_t));
#  endif
# endif
#endif

#ifdef HAVE_MEMSET
# define clewn_memset(ptr, c, size)   memset((ptr), (c), (size))
#else
void *clewn_memset __ARGS((void *, int, size_t));
#endif

/*
 * Maximum length of a path (for non-unix systems) Make it a bit long, to stay
 * on the safe side.  But not too long to put on the stack.
 */
#ifndef MAXPATHL
# ifdef MAXPATHLEN
#  define MAXPATHL	MAXPATHLEN
# else
#  define MAXPATHL	256
# endif
#endif

/* gdb directory constants */
#define GDB_CDIR    "$cdir"
#define GDB_CWD	    "$cwd"

void xatabort __ARGS((void (*)(void)));
void * xmalloc __ARGS((size_t));
void * xcalloc __ARGS((size_t));
void * xrealloc __ARGS((void *, size_t));
void xfree __ARGS((void *));
char * clewn_strsave __ARGS((char *));
char * clewn_strnsave __ARGS((char *, size_t));
void clewn_sleep __ARGS((int));
void clewn_beep __ARGS((void));
int clewn_getwd __ARGS((char *, int));
int clewn_fullpath __ARGS((char *, char *, int, int));
char * get_fullpath __ARGS((char *, char *, char *, char *, struct obstack *));
void clewn_exec(char *);

#endif	/* MISC_H */

