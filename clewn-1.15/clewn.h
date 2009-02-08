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
 * $Id: clewn.h 217 2008-10-11 14:29:18Z xavier $
 */

#ifndef CLEWN_H
# define CLEWN_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
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

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else
# define dirent direct
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#if !defined(HAVE_SYS_TIME_H) || defined(TIME_WITH_SYS_TIME)
# include <time.h>	    /* on some systems time.h should not be
			       included together with sys/time.h */
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

/* Note: Some systems need both string.h and strings.h (Savage).  However,
 * some systems can't handle both, only use string.h in that case. */
#if defined(HAVE_STRINGS_H) && !defined(NO_STRINGS_WITH_STRING_H)
# include <strings.h>
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

typedef char	char_u;
typedef unsigned long	long_u;
typedef long		linenr_T;	/* line number type */
typedef unsigned	colnr_T;	/* column number type */

/* return values for functions */
#define OK	1
#define FAIL	0

/* boolean constants */
#define FALSE	0
#define TRUE	1

/* character constants */
#define NUL	'\000'
#define BELL	'\007'
#define BS	'\010'
#ifndef TAB
#define TAB	'\011'
#endif
#define NL	'\012'
#define CR	'\015'
#ifndef ESC
#define ESC	'\033'
#endif

#define Ctrl_U	21
#define Ctrl_Z	26

#define NUMBUFLEN	30	    /* length of a buffer to store a number in ASCII */

#define VARIABLES_FNAME	".clewn-watched-vars.gdbvar"
#define TEMPDIRNAMES	"$TMPDIR", "/tmp", ".", "$HOME"
#define TEMPNAMELEN	256

/* The _() stuff is for using gettext().  It is a no-op with Clewn. */
# define _(x) ((char *)(x))

/* defines to avoid typecasts from (char_u *) to (char *) and back */
#define STRLEN(s)		strlen((char *)(s))
#define STRCPY(d, s)		strcpy((char *)(d), (char *)(s))
#define STRNCPY(d, s, n)	strncpy((char *)(d), (char *)(s), (size_t)(n))
#define STRCMP(d, s)		strcmp((char *)(d), (char *)(s))
#define STRNCMP(d, s, n)	strncmp((char *)(d), (char *)(s), (size_t)(n))
#define STRCAT(d, s)		strcat((char *)(d), (char *)(s))

# define MIN_SCREEN_HEIGHT 12

/* escape characters to send NetBeans commands or functions
 * directly from readline */
#define DEBUG_ESC_CMD	    '@'
#define DEBUG_ESC_FUN	    '#'

/* sign types */
#define SIGN_ANY	    0
#define SIGN_BP_ENABLED	    1
#define SIGN_BP_DISABLED    2

/* key mapping types */
#define KEYMAP_CONTROL	    0
#define KEYMAP_UPPERCASE    1
#define KEYMAP_FUNCTION	    2

/* the NetBeans event structure */
typedef struct
{
    char *key;	    /* allocated NetBeans key */
    char *lnum;	    /* allocated line number */
    char *pathname; /* allocated buffer name */
    char *text;	    /* allocated balloon text */
    int text_event; /* TRUE after receiving a balloonText netbeans event */
    int seqno;	    /* netbeans event or reply sequence number */
} nb_event_T;

extern int p_asm;		    /* assembly support when non zero */

/* pty.c declarations */
int OpenPTY __ARGS((char **ttyn));
int SetupSlavePTY __ARGS((int fd));

typedef struct
{
    int dummy;
} gdb_handle_T;

/* gdb interface */
void gdb_docmd __ARGS((gdb_handle_T *, char *));
void gdb_setwinput __ARGS((gdb_handle_T *, char *));
int gdb_iswinput __ARGS((gdb_handle_T *));
void gdb_free_records __ARGS((int));

/* NetBeans interface */
#define IS_NETBEANS_CONNECTED (cnb_get_connsock() >= 0 || cnb_get_datasock() >= 0)
int cnb_open __ARGS((char *, int *, int, int, char *));
void cnb_close __ARGS((void));
void cnb_saveAndExit __ARGS((void));
int cnb_state __ARGS((void));
int cnb_specialKeys __ARGS((void));
void cnb_keymap __ARGS((int, int));

int cnb_get_connsock __ARGS((void));
void cnb_create_varbuf __ARGS((char *));
void cnb_set_instance __ARGS((int));
int cnb_get_datasock __ARGS((void));
void cnb_conn_evt __ARGS((void));
nb_event_T * cnb_data_evt __ARGS((void));
void cnb_send_debug __ARGS((int, char *));
void cnb_showBalloon __ARGS((char *, int, struct obstack *));
void cnb_startAtomic __ARGS((int));
void cnb_endAtomic __ARGS((int));
int cnb_editFile __ARGS((char *, linenr_T, char *, char *, char *, int, struct obstack *));
int cnb_define_sign __ARGS((int, int, int, struct obstack *));
void cnb_buf_addsign __ARGS((int, int, int, linenr_T, struct obstack *));
int cnb_buf_getsign __ARGS((int, int));
void cnb_buf_delsign __ARGS((int, int));
int cnb_create_buf __ARGS((char *));
void cnb_kill __ARGS((int));
void cnb_set_asm __ARGS((int));
void cnb_unlink_asm __ARGS((void));
char * cnb_filename __ARGS((int));
int cnb_lookup_buf __ARGS((char *));
int cnb_isvalid_buffer __ARGS((int));
int cnb_outofbounds __ARGS((int));
void cnb_append __ARGS((int, char *, struct obstack *));
void cnb_clear __ARGS((int, struct obstack *));
void cnb_replace __ARGS((int, char *, int, struct obstack *));
char * cnb_search_obj __ARGS((char *, int *));

/* Utilities */
void EMSG __ARGS((char *));
linenr_T find_fr_sign __ARGS((int));
FILE * clewn_opentmpfile __ARGS((char *, char **, int));
char * clewn_stripwhite __ARGS((char *));
linenr_T searchfor __ARGS((char *, char *));

#endif	/* CLEWN_H */

