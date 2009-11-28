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
 * $Id: clewn.c 230 2009-11-28 16:50:31Z xavier $
 */

#include <config.h>

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>
#include <readline/readline.h>
#include <readline/history.h>


#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#if defined(HAVE_SYS_SELECT_H) && \
	(!defined(HAVE_SYS_TIME_H) || defined(SYS_SELECT_WITH_SYS_TIME))
# include <sys/select.h>
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

#ifdef HAVE_TERMIOS_H
# include <termios.h>
#else
# include <termio.h>
#endif

/* sun's sys/ioctl.h redefines symbols from termio world */
#if defined(HAVE_SYS_IOCTL_H) && !defined(sun)
# include <sys/ioctl.h>
#endif

#include "obstack.h"
#include "clewn.h"
#include "gdb.h"
#include "misc.h"

/*
 * EMX doesn't have a global way of making open() use binary I/O.
 * Use O_BINARY for all open() calls.
 */
#if defined(__EMX__) || defined(__CYGWIN32__)
# define O_EXTRA    O_BINARY
#else
# define O_EXTRA    0
#endif

#define CLEWN_HELP "Clewn version %s using readline library revision %s.\n\
\n\
Usage: clewn [-vc gvim_cmd] [-va gvim_args]\n\
             [-gc gdb_cmd] [-ga gdb_args]\n\
	     [-p project] [-x pathnames_map]\n\
             [-a] [-nb[:<host>[:<port>[:<passwd>]]]] [-d] [-r]\n\
\n\
  -vc gvim_cmd                gvim shell command or gvim pathname (default 'gvim')\n\
  -va gvim_args               gvim command line arguments\n\
  -gc gdb_cmd                 gdb shell command or gdb pathname (default 'gdb')\n\
  -ga gdb_args                gdb command line arguments\n\
  -p  project                 project file name\n\
  -x pathnames_map            remote debugging\n\
  -a                          enable assembly support\n\
  -nb:<host>:<port>:<passwd>  NetBeans connection parameters\n\
                                <host>   default {$__NETBEANS_HOST|localhost}\n\
                                <port>   default {$__NETBEANS_SOCKET|3219}\n\
                                <passwd> default {$__NETBEANS_VIM_PASSWORD|changeme}\n\
  -d                          enable NetBeans debug mode\n\
  -r                          do not set SO_REUSEADDR socket option\n\
\n\
Clewn listens on host:port, with host being a name or the IP address of one\n\
of the local network interfaces in standard dot notation.\n\n"

#define INPUTRC_FNAME	    "inputrc"
#define CLEWN_HISTORY_FILE  "/.clewn_history"
#define CLEWN_KEYS_FILE	    "/.clewn_keys"
#define HISTORY_DFLT_SIZE   50
#define GVIM_DEFAULT_ARG    " -c \"run clewn.vim\" -nb -g "

int p_asm = 0;			/* assembly support when non zero */

static int module_state = -1;		    /* initial state (not OK, nor FAIL) */
static char clewn_buf[MAX_BUFFSIZE];	    /* general purpose buffer */

static char * p_vim = NULL;
static char * p_vc = "gvim";/* gvim shell command or gvim pathname */
static char * p_va = "";    /* gvim command line arguments */
static char * p_gdb = NULL;
static char * p_gc = "gdb"; /* gdb shell command or gdb pathname */
static char * p_ga = "";    /* gdb command line arguments */
static char * p_project = NULL;/* project file name */
static char * p_pmap = NULL;/* remote debugging pathnames map */

static int clewn_debug = FALSE;				/* debug flag */
static int showBalloon = FALSE;				/* enable tooltips */
static char * p_gvar = VARIABLES_FNAME;			/* variables buffer extension */
static char *clewn_tempdir = NULL;			/* Clewn temporary directory */
static char *clewn_dir;					/* Clewn directory */

static gdb_T *gdb = NULL;	/* the gdb_T instance */
static int got_sigint = FALSE;	/* TRUE when got SIGINT */
static int got_sigchld = FALSE;	/* TRUE when got SIGCHLD */
static int got_tab = FALSE;	/* TRUE when got a <TAB> */
static char *preprompt;		/* previous prompt line when prompt is multi line */
static char *cplt_prmpt;	/* current prompt for completion */
static int nl_output;		/* when TRUE, no need to output a newline */
static char *key_command;	/* command mapped from NetBeans key */
static int inside_readline = FALSE; /* TRUE when cli_getc() is called by readline() */

#define FUNMAP_SIZE 20
#define CLEWN_KEYMAP_SIZE (256 + FUNMAP_SIZE)
#define INTBITS (sizeof(int) * 8)

#define FUNCTION_INDEX(n) (((n) > 0 && (n) <= FUNMAP_SIZE) \
	? (256 + (n) - 1) : -1)

#define IS_KEY_FLAG(t,i) (((i) >= 0 && (i) < CLEWN_KEYMAP_SIZE) \
	? ((t)[(i) / INTBITS] & (1 << ((i) % INTBITS))) : FALSE)

#define SET_KEY_LINE(t,i) (((i) >= 0 && (i) < CLEWN_KEYMAP_SIZE) \
	? ((t)[(i) / INTBITS] |= (1 << ((i) % INTBITS))) : FALSE)

static char * keymap[CLEWN_KEYMAP_SIZE];  /* GDB commands array indexed by key */
static int keyline[CLEWN_KEYMAP_SIZE / INTBITS + 1];	/* %line flag bit map */
static int keytext[CLEWN_KEYMAP_SIZE / INTBITS + 1];	/* %text flag bit map */

/* The gdb keyword */
typedef struct
{
    int type;
    char *keyword;	/* keyword */
    char *tail;		/* optional tail */
} token_T;

static token_T tokens[] =
{
    {CMD_DETACH,    "det",	"ach"	    },
    {CMD_SHELL,	    "she",	"ll"	    },
    {CMD_STEPI,	    "si",	""	    },
    {CMD_STEPI,	    "stepi",	""	    },
    {CMD_STEPI,	    "ni",	""	    },
    {CMD_STEPI,	    "nexti",	""	    },
    {CMD_EXECF,	    "fil",	"e"	    },
    {CMD_EXECF,	    "ex",	"ec-file"   },
    {CMD_EXECF,	    "cor",	"e-file"    },
    {CMD_BREAK,	    "b",	"reak"	    },
    {CMD_BREAK,	    "tb",	"reak"	    },
    {CMD_BREAK,	    "hb",	"reak"	    },
    {CMD_BREAK,	    "thb",	"reak"	    },
    {CMD_DISPLAY,   "disp",	"lay"	    },
    {CMD_CREATEVAR, "cr",	"eatevar"   },
    {CMD_UP_SILENT, "up-",	"silently"  },
    {CMD_UP,	    "up",	""	    },
    {CMD_DOWN_SILENT,"down-",	"silently"  },
    {CMD_DOWN,	    "do",	"wn"	    },
    {CMD_FRAME,	    "f",	"rame"	    },
    {CMD_DISABLE,   "disab",	"le"	    },
    {CMD_DELETE,    "del",	"ete"	    },
    {CMD_SLECT_FRAME,"sel",	"ect-frame" },
    {CMD_SYMF,	    "sy",	"mbol-file" },
    {CMD_SYMF,	    "add-sy",	"mbol-file" },
    {CMD_SYMF,	    "so",	"urce"	    },
    {CMD_RESTART,   "cl_",	"restart"   },
    {CMD_QUIT,	    "q",	"uit"	    },
    {CMD_ANY,	    NULL,	NULL	    }
};

/* The gdb pattern */
typedef struct
{
    int id;		/* pattern id */
    char *str;		/* string pattern */
    regex_t *regprog;	/* compiled regexp */
} pattern_T;

static pattern_T patterns[] =
{
    {PAT_DIR,		"^ *Source directories searched: *(.*)$", NULL},
    {PAT_CHG_ANNO,	"^ *set +a(n|nn|nno|nnot|nnota|nnotat|nnotate) +.*$", NULL},
    {PAT_ADD,		"^ *0x0*([0-9a-fA-F]+)", NULL},
    {PAT_PID,		"^ *a(t|tt|tta|ttac|ttach) +([0-9]+) *$", NULL},
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
    {PAT_SOURCE,	"^ *([^:]*):([^:]*):[^:]*:[^:]*:0x0*([0-9a-fA-F]+)$", NULL},
    {PAT_QUERY,		"^.*\\(y or n\\) *$", NULL},
    {PAT_YES,		"^ *(y|ye|yes) *$", NULL},
    {PAT_SFILE,		"^Symbols from *\"([^\"]+)", NULL},
    {PAT_BP_CONT,	"^ *(c) *|^ *(con)(t|ti|tin|tinu|tinue) *", NULL},
    {PAT_ASM_FUNC,	"^([^ ^(]+)(\\(.*\\))* .*in section .text$", NULL},
    {PAT_ASM_FUNC_P,	"^.*<([^ ^+]+)(\\+[0-9]+)*>$", NULL},
    {PAT_FRAME,		"^#[0-9]+ +0x0*([0-9a-fA-F]+)", NULL},
    {PAT_HEIGHT,	"^ *set +h(e|ei|eig|eigh|eight) +([0-9]+) *$", NULL},
#endif
#ifdef GDB_LVL2_SUPPORT
    /* MUST add '.*' at end of each info (possibly last) breakpoint field
     * pattern because GDB adds the hit count, in a new line, after
     * printing the last field and within its annotation context */
    {PAT_BP_ASM,	"^<([^ ]+)\\+[0-9]+>.*$|^<([^ ]+)>.*$", NULL},
    {PAT_BP_SOURCE,	"^.* ([^ ]+):([0-9]+).*$", NULL},
    {PAT_DISPLAY,	"^ *dis(p|pl|pla|play) *$", NULL},
    {PAT_DISPINFO,	"^([0-9]+):", NULL},
    {PAT_CREATEVAR,	"^ *c(r|re|rea|reat|reate|reatev|reateva|reatevar) +(.*)$", NULL},
#endif
#ifdef GDB_LVL3_SUPPORT
    {PAT_CRVAR_FMT,	"^ *c(r|re|rea|reat|reate|reatev|reateva|reatevar) *(/[tdxo])* +(.*)$", NULL},
    {PAT_INFO_FRAME,	"^Stack level ([0-9]+), frame at ", NULL},
#endif
    {0,			NULL, NULL}
};

#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
static char *inputrc;		/* readline inputrc file name */

/* gdb readline inputrc file content */
static char *inputrc_array[] = {
    "set show-all-if-ambiguous on\n",
    "Control-u: unix-line-discard\n",
    NULL
};
#endif

/* default key mappings with netbeans version < 2.3 (no specialKeys command) */
static char * old_keys_mapping[] =
{
    "z:\032:		    # interrupt",
    "B:info breakpoints:",
    "L:info locals:",
    "A:info args:",
    "S:step:",
    "I:stepi:",
    "n:next:",
    "N:nexti:		    # upper case",
    "F:finish:",
    "R:run:",
    "Q:quit:",
    "C:continue:",
    "W:where:",
    "u:up:",
    "d:down:",
    "b:break:%line	    # set breakpoint at current line",
    "e:clear:%line	    # clear breakpoint at current line",
    "k:break *:%text	    # set breakpoint at address at mouse position",
    "h:clear *:%text	    # clear breakpoint at address at mouse position",
    "p:print:%text	    # print value of word at mouse position",
    "a:print *:%text	    # print value referenced by text at mouse position",
    "j:createvar:%text	    # add variable (expression) at mouse position",
    NULL
};

/* default key mappings with netbeans version >= 2.3 (with specialKeys command) */
static char * keys_mapping[] =
{
    "C-Z:\032:		    # interrupt",
    "B:info breakpoints:",
    "L:info locals:",
    "A:info args:",
    "S:step:",
    "I:stepi:",
    "C-N:next:",
    "X:nexti:		    # upper case",
    "F:finish:",
    "R:run:",
    "Q:quit:",
    "C:continue:",
    "W:where:",
    "C-U:up:",
    "C-D:down:",
    "C-B:break:%line	    # set breakpoint at current line",
    "C-E:clear:%line	    # clear breakpoint at current line",
    "C-K:break *:%text	    # set breakpoint at address at mouse position",
    "C-H:clear *:%text	    # clear breakpoint at address at mouse position",
    "C-P:print:%text	    # print value of word at mouse position",
    "C-X:print *:%text	    # print value referenced by text at mouse position",
    "C-J:createvar:%text    # add variable (expression) at mouse position",
    NULL
};

/* storage for mtrace hooks */
#if defined(GDB_MTRACE) && defined(HAVE_MTRACE)
__ptr_t (*s_malloc) (size_t, const void *);
void (*s_free) (void *, const void *);
__ptr_t (*s_realloc) (void *, size_t, const void *);
#endif	/* GDB_MTRACE */

/* readline interface */
static int cli_init __ARGS((void));
#ifdef HAVE_RL_COMMAND_FUNC_T
static rl_command_func_t cli_complete;
#else
static int cli_complete __ARGS((void));
#endif
static int cli_getc __ARGS((FILE *));

/* Gdb process mgmt */
static RETSIGTYPE catch_sigint __ARGS((int));
static RETSIGTYPE catch_sigterm __ARGS((int));
static RETSIGTYPE catch_sigchld __ARGS((int));
static RETSIGTYPE catch_sigpipe __ARGS((int));
static void clewn_abort __ARGS((void));
static void clewn_getout __ARGS((void));
static void start_gdb_process __ARGS((void));
static int module_init __ARGS((void));
static void module_end __ARGS((void));
static void clear_gdb_T __ARGS((void));
static void exec_gdb __ARGS((void));
static int exec_gvim __ARGS((void));
static void clewn_keymap __ARGS((void));
static void write_project __ARGS((void));

/* Utilities */
static int clewn_mktmpdir __ARGS((void));
static void clewn_deltempdir __ARGS((void));
static void print_regerror __ARGS((int, regex_t *));
static void clear_readline __ARGS((void));
static void clewn_add_history __ARGS((char *));
static void parse_keyline(char *);
static void read_keysfile __ARGS((char *));
static void process_nb_event __ARGS((void));
static char * get_keymap __ARGS((nb_event_T *));

/*
 * Main
 */
    int
main(int argc, char ** argv)
{
    char * nbconn  = NULL;	    /* NetBeans connection parameters */
    char intr[2]   = {KEY_INTERUPT, NUL};
    int reuse_addr   = TRUE;
    char *line;

#if defined(GDB_MTRACE) && defined(HAVE_MTRACE)
    mtrace();
    mv_hooks();
#endif
    /* register an abort function for when allocating memory fails */
    xatabort(clewn_abort);
    obstack_alloc_failed_handler = clewn_abort;

    /* initialize readline */
    rl_readline_name = "Clewn";	    /* allow conditional parsing of ~/.inputrc */
    rl_startup_hook = cli_init;	    /* change the binding of the <TAB> key */
    rl_getc_function = cli_getc;    /* to get a character from the input stream */
    rl_initialize();
    if (rl_outstream == NULL)
    {
	fprintf(stderr, "cannot initialize readline\n");
	clewn_abort();
    }

    /*
     * Get program options
     */
    argv++;
    argc--;

    while (argc > 0)
    {
	char *opt = *argv;

	if (*opt == '-')
	{
	    switch (*(opt + 1))
	    {
		case 'v':
		    if (*(opt + 2) == 'c'|| *(opt + 2) == 'a') {
			argv++;
			argc--;
			if (argc <= 0 || ((*argv)[0] == '-' && *(opt + 2) == 'c')) {
			    fprintf(stderr, "Cannot parse mandatory parameter of \"%s\"\n", opt);
			    clewn_abort();
			}
			/* -vc gvim_cmd - gvim shell command or gvim pathname (default 'gvim') */
			if (*(opt + 2) == 'c')
			    p_vc = *argv;
			/* -va gvim_args - gvim command line arguments */
			else if (*(opt + 2) == 'a')
			    p_va = *argv;
		    }
		    else if (*(opt + 2) == NUL) { /* "-v" print help */
			fprintf(stderr, CLEWN_HELP, PACKAGE_VERSION, rl_library_version);
			clewn_abort();
		    }
		    else {
			fprintf(stderr, "Unknown command line argument \"%s\"\n", opt);
			clewn_abort();
		    }
		    break;

		case 'g':
		    if (*(opt + 2) == 'c'|| *(opt + 2) == 'a') {
			argv++;
			argc--;
			if (argc <= 0 || ((*argv)[0] == '-' && *(opt + 2) == 'c')) {
			    fprintf(stderr, "Cannot parse mandatory parameter of \"%s\"\n", opt);
			    clewn_abort();
			}
			/* -gc gdb_cmd - gdb shell command or gdb pathname (default 'gdb') */
			if (*(opt + 2) == 'c')
			    p_gc = *argv;
			/* -ga gdb_args - gdb command line arguments */
			else if (*(opt + 2) == 'a')
			    p_ga = *argv;
		    }
		    else {
			fprintf(stderr, "Unknown command line argument \"%s\"\n", opt);
			clewn_abort();
		    }
		    break;

		case 'p':   /* "-p" project file name */
		    argv++;
		    argc--;
		    if (argc <= 0 || (*argv)[0] == '-') {
			fprintf(stderr, "Cannot parse mandatory parameter of \"%s\"\n", opt);
			clewn_abort();
		    }
		    p_project = *argv;
		    break;

		case 'a':   /* "-a" set assembly support */
		    p_asm = 1;
		    break;

		case 't':   /* "-t" enable displaying GDB state in a tooltip */
		    showBalloon = TRUE;
		    break;

		case 'r':   /* "-r" do not set SO_REUSEADDR socket option */
		    reuse_addr = FALSE;
		    break;

		case 'x':   /* "-x" remote debugging */
		    argv++;
		    argc--;
		    if (argc <= 0 || (*argv)[0] == '-') {
			fprintf(stderr, "Cannot parse mandatory parameter of \"%s\"\n", opt);
			clewn_abort();
		    }
		    p_pmap = *argv;
		    break;

		case 'd':   /* "-d" enable NetBeans debug output */
		    clewn_debug = TRUE;
		    break;

		case 'n':   /* "-nb:host:port:passwd" NetBeans parameters */
		    if (*(opt + 2) == 'b' && *(opt + 3) == ':')
			nbconn = opt + 4;
		    break;

		case 'h':   /* "-h" print help */
		    fprintf(stderr, CLEWN_HELP, PACKAGE_VERSION, rl_library_version);
		    clewn_abort();

		default:
		    fprintf(stderr, "Unknown command line argument \"%s\"\n", opt);
		    clewn_abort();
	    }
	}
	else
	{
	    fprintf(stderr, "Unknown command line argument \"%s\"\n", opt);
	    clewn_abort();
	}

	argv++;
	argc--;
    }

    signal(SIGINT, catch_sigint);
    signal(SIGTERM, catch_sigterm);
    signal(SIGCHLD, catch_sigchld);
    signal(SIGPIPE, catch_sigpipe);

#if defined(GDB_MTRACE) && defined(HAVE_MTRACE)
    /* the mtrace log file may get corrupted when forking a child
     * because mtrace uses output buffering and flushes its buffer
     * in the child when closing on exec
     * do few dummies alloc to force the flush before the fork
     * and avoid that problem
     * */
    {
	#define DUMMY_ALLOC_COUNT 100	/* tune this value */
	int i;
	char * dummy;

	for (i = 0; i < DUMMY_ALLOC_COUNT; i++)
	    if ((dummy = (char *)xmalloc(1)) != NULL)
		xfree(dummy);
    }
#endif

    /* start GDB, do not output unneeded newlines */
    nl_output = TRUE;
    start_gdb_process();
    nl_output = TRUE;

    if (! GDB_STATE(gdb, GS_UP))
	clewn_getout();

    /* remote debugging and project file are not supported on level 2 */
    if (gdb->mode == GDB_MODE_LVL2) {
	p_pmap = NULL;
	p_project = NULL;
    }

    /* listen on a NetBeans socket */
    if (cnb_open(nbconn, &(gdb->var_buf), clewn_debug, reuse_addr, p_pmap) != OK)
	clewn_getout();

    /* the gdb instance number is used to define signs in vim
     * with a different name for each instance of gdb */
    cnb_set_instance(gdb->instance);

    /* create variables buffer */
    cnb_create_varbuf(gdb->var_name);

    /* fork gvim if not doing remote debugging */
    if (! p_pmap && exec_gvim() != OK)
	clewn_getout();

    while (IS_NETBEANS_CONNECTED && GDB_STATE(gdb, GS_UP))
    {
	/*
	 * 1 - Wait for GDB prompt
	 */
	(void)cli_getc(NULL);

	/* handle interrupt */
	if (got_sigint)
	{
	    got_sigint = FALSE;
            gdb_docmd((gdb_handle_T *)gdb, intr); /* send KEY_INTERUPT to GDB */
	    continue;
	}

	/* should never happen (but make sure we do have a prompt) */
	if (gdb->prompt == NULL)
	    gdb->prompt = clewn_strsave("(gdb)  ");

	/*
	 * 2 - Launch readline
	 */
	/* output readline on a new line so as not to overwrite last line */
	if (! nl_output)
	    fputs("\n", rl_outstream);
	nl_output = TRUE;

	inside_readline = TRUE;

	if ((line = readline(gdb->prompt)) == NULL)
	    break;

	inside_readline = FALSE;

	if (gdb->note == ANO_PMT_FORMORE)
	{
	    if (got_sigint || (strlen(line) == 1 && *line == 'q'))
		got_sigint = TRUE;	/* abort listing */
	    else
	    {
		free(line);
		gdb_docmd((gdb_handle_T *)gdb, "");   /* continue */
		continue;
	    }
	}

	if (got_tab)
	    got_tab = FALSE;
	else if (got_sigint)
	{
	    got_sigint = FALSE;
            gdb_docmd((gdb_handle_T *)gdb, intr); /* send KEY_INTERUPT to GDB */
	}
	else
        {
	    clewn_add_history(line);

	    /* when debug mode and first char is an escape character,
	     * handle the line as a NetBeans command or function */
	    if (clewn_debug && (*line == DEBUG_ESC_CMD || *line == DEBUG_ESC_FUN))
		cnb_send_debug((int)*line, line + 1);
	    else
		gdb_docmd((gdb_handle_T *)gdb, line);   /* send cmd to GDB */
        }

	free(line);

	/* warn the user that the data socket is not connected */
	if (cnb_get_datasock() < 0)
	{
	    fputs("************************************************\n", rl_outstream);
	    fputs("The netbeans socket to Vim is not connected yet.\n", rl_outstream);
	    fputs("Please check that the netbeans_intg feature is compiled in your Vim version\n", rl_outstream);
	    fputs("by running the Vim command \":version\", and checking that this command\n", rl_outstream);
	    fputs("displays \"+netbeans_intg\".\n", rl_outstream);
	    fputs("************************************************\n", rl_outstream);
	}

    }

    clewn_getout();
    return 0;
}

/* Change the binding of the <TAB> key */
    static int
cli_init()
{
    rl_bind_key((int)TAB, cli_complete);
    return 0;
}

/* Handle completion */
    static int
#ifdef HAVE_RL_COMMAND_FUNC_T
cli_complete(whatisthis, key)
    int whatisthis;
    int key;
#else
cli_complete()
#endif
{
    char * cmd;

#ifdef HAVE_RL_COMMAND_FUNC_T
    whatisthis = key = 0;   /* keep compiler happy */
#endif

    /* store the prompt for the completion result */
    xfree(cplt_prmpt);
    cplt_prmpt = clewn_strsave(gdb->prompt);

    /* send readline content up to cursor and <TAB> to GDB */
    cmd = clewn_strnsave(rl_line_buffer, rl_point + 1);
    *(cmd + rl_point) = NUL;    /* need rl_point first characters, not more */
    strcat(cmd, "\t");
    gdb_docmd((gdb_handle_T *)gdb, cmd);   /* send it to GDB */
    xfree(cmd);

    got_tab = TRUE;
    return 0;
}

/*
 * Blocking poll/select on readline instream, GDB pseudo tty and NetBeans sockets.
 * Return one character from instream.
 * Return NL when got SIGINT or a GDB command mapped from a received NetBeans key.
 * Return <SPACE> when got <TAB>.
 * Return EOF when instream is NULL and got a GDB prompt.
 * Return EOF when IO error or end of file.
 */
    static int
cli_getc(instream)
    FILE *instream;	/* not NULL when called by readline */
{
    int fd = -1;	/* instream file descriptor */
    int fdcon;		/* connection socket file descriptor */
    int fdata;		/* data socket file descriptor */
    pid_t wait_pid;
    int ret;
#ifndef HAVE_SELECT
    struct pollfd fds[4];
    int nfd;
    int gdb_idx;
    int fdcon_idx;
    int fdata_idx;
#else
    int maxfd;
    fd_set rfds;
#endif

    if (instream != NULL && (fd = fileno(instream)) == -1)
	return EOF;

    do
    {
	ret = -1;

	/* Source the project file when the netbeans session is up,
	 * so that we can set the breakpoints in vim */
	if (gdb->project_file != NULL
		&& gdb->project_state == PROJ_INIT
		&& ! IS_OOBACTIVE(gdb)
		&& inside_readline
		&& cnb_state())
	{
	    gdb->project_state = PROJ_SOURCEIT;
	    gdb_docmd((gdb_handle_T *)gdb, "");
	}

	/* GDB still running */
	if (got_sigchld || gdb->pid == -1)
	{
	    got_sigchld = FALSE;

	    if (gdb->pid != -1)	    /* got SIGCHLD */
	    {
		wait_pid = waitpid(gdb->pid, NULL, WNOHANG);

		if ((wait_pid == (pid_t)-1 && errno == ECHILD)
			|| wait_pid == gdb->pid)
		{
		    gdb->pid = -1;  /* got SIGCHLD from GDB */
		    fprintf(stderr, "GDB terminated       \n");
		}
	    }

	    if (gdb->pid == -1)	    /* shutdown and exit */
	    {
		/* restart gdb if not quitting */
		if (! GDB_STATE(gdb, GS_QUITTING)) {
		    gdb_close(gdb);
		    start_gdb_process();
		}

		if (! GDB_STATE(gdb, GS_UP))
		    clewn_getout();

	    }
	}

	/* got a prompt from GDB */
	if (instream == NULL && gdb->prompt != NULL)
	{
	    return EOF;
	}

	/* got SIGINT or an interrupt in the form of a NetBeans key command */
	if (got_sigint || (key_command != NULL && *key_command == KEY_INTERUPT))
	{
	    /* being called by readline */
	    if (instream != NULL)
	    {
		rl_point = rl_end;	/* at the end of the line */
		rl_insert_text("Quit");
		rl_redisplay();
	    }

	    /* consume the key command and emulate an interrupt */
	    if (key_command != NULL && *key_command == KEY_INTERUPT)
	    {
		got_sigint = TRUE;
		FREE(key_command);
	    }
	    return (int)NL;
	}

	/* got <TAB>:
	 * tell readline we are done
	 * stay on same line, but set nl_output so that further lines if
	 * any are printed below this line */
	if (instream != NULL && got_tab)
	{
	    rl_done = 1;
	    nl_output = FALSE;
	    return (int)' ';
	}

	/* insert winput_cmd in readline */
	if (gdb->winput_cmd != NULL)
	{
	    /* being called by readline */
	    /* we get a prompt from GDB when the completion
	     * result is a list (no need to provide one then) */
	    if (instream != NULL)
	    {
#ifdef HAVE_RL_REPLACE_LINE
		rl_replace_line(gdb->winput_cmd, 0);
		rl_point = rl_end;	/* at the end of the line */
#else
		rl_insert_text(gdb->winput_cmd);
#endif
		rl_redisplay();
		FREE(gdb->winput_cmd);
	    }
	    else
	    {
		/* provide the prompt to enable readline in main */
		/* assert: gdb->prompt == NULL */
		if (gdb->cli_cmd.state == CS_QUERY)
		{
		    /* a completion query: provide last line as prompt
		     * overwrite (identical) last line with readline prompt */
		    nl_output = TRUE;
		    gdb->prompt = clewn_strsave(gdb->line);
		}
		else
		{
		    clear_readline();

		    /* a single completion result: provide last prompt
		     * as prompt and stay on same line */
		    nl_output = TRUE;
		    gdb->prompt = cplt_prmpt;
		    cplt_prmpt = NULL;
		}
		return EOF;
	    }
	}

	/* a GDB command mapped from a received NetBeans key */
	if (instream != NULL && key_command != NULL)
	{
	    rl_insert_text(key_command);
	    rl_redisplay();
	    FREE(key_command);
	    return (int)NL;
	}

#ifndef HAVE_SELECT
	nfd = 0;

	if (fd >= 0)
	{
	    fds[nfd].fd = fd;
	    fds[nfd].events = POLLIN;
	    nfd++;
	}

	if (GDB_STATE(gdb, GS_UP))
	{
	    fds[nfd].fd = gdb->fd;
	    fds[nfd].events = POLLIN;
	    gdb_idx = nfd;
	    nfd++;
	}

	if ((fdcon = cnb_get_connsock()) >= 0)
	{
	    fds[nfd].fd = fdcon;
	    fds[nfd].events = POLLIN;
	    fdcon_idx = nfd;
	    nfd++;
	}

	if ((fdata = cnb_get_datasock()) >= 0)
	{
	    fds[nfd].fd = fdata;
	    fds[nfd].events = POLLIN;
	    fdata_idx = nfd;
	    nfd++;
	}

	if (nfd > 0)
	    ret = poll(fds, nfd, -1);

	if (GDB_STATE(gdb, GS_UP) && ret > 0 && fds[gdb_idx].revents & POLLIN)
	{
	    ret--;
	    if (gdb->parse_output != NULL)
		(void)gdb->parse_output(gdb);
	}

	if (fdcon >= 0 && ret > 0 && fds[fdcon_idx].revents & POLLIN)
	{
	    if (instream != NULL)
		clear_readline();

	    ret--;
	    cnb_conn_evt();

	    if (instream != NULL)   /* update readline display */
		rl_forced_update_display();
	}

	if (fdata >= 0 && ret > 0 && fds[fdata_idx].revents & POLLIN)
	{
	    if (instream != NULL)
		clear_readline();

	    ret--;
	    process_nb_event();

	    clewn_keymap();	/* initialize keys mappings (only once) */

	    if (instream != NULL)   /* update readline display */
		rl_forced_update_display();
	}

#else /* HAVE_SELECT */
	maxfd = -1;
	FD_ZERO(&rfds);

	if (fd >= 0)
	{
	    FD_SET(fd, &rfds);
	    if (maxfd < fd)
		maxfd = fd;
	}

	if (GDB_STATE(gdb, GS_UP))
	{
	    FD_SET(gdb->fd, &rfds);
	    if (maxfd < gdb->fd)
		maxfd = gdb->fd;
	}

	if ((fdcon = cnb_get_connsock()) >= 0)
	{
	    FD_SET(fdcon, &rfds);
	    if (maxfd < fdcon)
		maxfd = fdcon;
	}

	if ((fdata = cnb_get_datasock()) >= 0)
	{
	    FD_SET(fdata, &rfds);
	    if (maxfd < fdata)
		maxfd = fdata;
	}

	if (maxfd >= 0)
	    ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);

	if (GDB_STATE(gdb, GS_UP) && ret > 0 && FD_ISSET(gdb->fd, &rfds))
	{
	    ret--;
	    if (gdb->parse_output != NULL)
		(void)gdb->parse_output(gdb);
	}

	if (fdcon >= 0 && ret > 0 && FD_ISSET(fdcon, &rfds))
	{
	    if (instream != NULL)
		clear_readline();

	    ret--;
	    cnb_conn_evt();

	    if (instream != NULL)   /* update readline display */
		rl_forced_update_display();
	}

	if (fdata >= 0 && ret > 0 && FD_ISSET(fdata, &rfds))
	{
	    if (instream != NULL)
		clear_readline();

	    ret--;
	    process_nb_event();

	    clewn_keymap();	/* initialize keys mappings (only once) */

	    if (instream != NULL)   /* update readline display */
		rl_forced_update_display();
	}
#endif
    } while ((ret == 0 || (ret < 0 && errno == EINTR)) && IS_NETBEANS_CONNECTED);

    if (ret > 0)
    {
	if (instream != NULL)
	    return rl_getc(instream);
    }
    else if (IS_NETBEANS_CONNECTED)
	perror("select() failed in cli_getc()");

    return EOF;
}

/* Handle SIGINT signal */
    static RETSIGTYPE
catch_sigint(sig)
    int sig;
{
    if (sig == SIGINT) {}   /* keep compiler happy */

    got_sigint = TRUE;
    signal(SIGINT, catch_sigint);
}

/* Handle SIGTERM signal */
    static RETSIGTYPE
catch_sigterm(sig)
    int sig;
{
    if (sig == SIGTERM) {}   /* keep compiler happy */

    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    clewn_getout();             /* terminate GDB */
}

/* Handle SIGCHLD signal */
    static RETSIGTYPE
catch_sigchld(sig)
    int sig;
{
    if (sig == SIGCHLD) {}   /* keep compiler happy */

    got_sigchld = TRUE;
    signal(SIGCHLD, catch_sigchld);
}

/* Handle SIGPIPE signal */
    static RETSIGTYPE
catch_sigpipe(sig)
    int sig;
{
    if (sig == SIGPIPE) {}   /* keep compiler happy */

    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    clear_readline();
    if (clewn_debug)
	fprintf(stderr, "disconnected by peer in write()\n");
    clewn_getout();             /* terminate GDB */
}

/** Abort clewn */
    static void
clewn_abort()
{
    /* we MUST NOT call any function that allocates memory */
    if (gdb != NULL)
	gdb_close(gdb);		/* terminate GDB */

#if defined(GDB_MTRACE) && defined(HAVE_MTRACE)
    muntrace();
#endif
    exit(EXIT_FAILURE);
}

#define EXIT_TIMEOUT 10000
/** Terminate clewn */
    static void
clewn_getout()
{
    int wtime = EXIT_TIMEOUT;	/* msecs */
    int fdata = cnb_get_datasock();
    int rc;
# ifndef HAVE_SELECT
    struct pollfd fds;
# else
    struct timeval tv;
    struct timeval start_tv;
    fd_set rfds;

    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    /* prevent recursion */
    if (gdb == NULL)
	return;

    rl_cleanup_after_signal();  /* reset terminal so we can printf */
    cnb_saveAndExit();

#  ifdef HAVE_GETTIMEOFDAY
	gettimeofday(&start_tv, NULL);
#  endif
# endif

    /* Give vim a chance to close the netbeans socket before EXIT_TIMEOUT,
     * in conformance with the netbeans protocol  */
    while (fdata >=0 && wtime > 0) {
# ifndef HAVE_SELECT
	fds.fd = fdata;
	fds.events = POLLIN;

	rc = poll(&fds, 1, 200);
# else
	FD_ZERO(&rfds);
	FD_SET(fdata, &rfds);

	tv.tv_sec = 0;		/* do a select every 200 ms, to update the printf */
	tv.tv_usec = 200000;

	rc = select(fdata + 1, &rfds, NULL, NULL, &tv);
# endif

	if ((rc == -1 && errno == EINTR) || rc == 0) {
	    /* compute remaining wait time */
# if ! defined(HAVE_SELECT) || ! defined(HAVE_GETTIMEOFDAY)
	    /* guess: interrupted halfway, gdb processing 10 msecs */
	    wtime = wtime / 2 - 10L;
# else
	    gettimeofday(&tv, NULL);
	    wtime -= (tv.tv_sec - start_tv.tv_sec) * 1000L
			    + (tv.tv_usec - start_tv.tv_usec) / 1000L;
	    start_tv.tv_sec = tv.tv_sec;
	    start_tv.tv_usec = tv.tv_usec;
# endif
	}
	else if (rc < 0)
	    break;
	else if (read(fdata, clewn_buf, MAX_BUFFSIZE) <= 0)
	    break;
	else
	    wtime = EXIT_TIMEOUT;

	printf("\rWaiting for gvim to close netbeans socket, exit in %02d s", wtime / 1000);
	fflush(stdout);
    }

    printf("\r                                                         \n");
    if (wtime <= 0)
	printf("Vim is still up and running\n");

    gdb_close(gdb);		/* terminate GDB */
    module_end();		/* release module resources */
    clear_gdb_T();
    FREE(gdb);
    cnb_close();		/* close NetBeans sockets */

#if defined(GDB_MTRACE) && defined(HAVE_MTRACE)
    muntrace();
#endif
    exit(EXIT_SUCCESS);
}

/** Send a cmd to gdb */
    void
gdb_docmd(gdb_inst, cmd)
    gdb_handle_T *gdb_inst;
    char  *cmd;	/* gdb cmd */
{
    gdb_T *this = (gdb_T *)gdb_inst;

    if (this == NULL || ! GDB_STATE(this, GS_UP))
	return;

    /* accept one cmd at a time, allow intr */
    if (cmd != NULL && *cmd != NUL && *(cmd + strlen(cmd) - 1) == KEY_INTERUPT)
	this->oob.state |= OS_INTR;
    else if (this->oob.state & OS_CMD)
    {
	if (cmd != NULL && *cmd != NUL)
	    fprintf(stderr, "GDB busy: command discarded, please retry\n");
	return;
    }
    else
	this->oob.idx = -1;	/* needed when last oob was aborted with OS_QUITs */
    this->oob.state |= OS_CMD;

    /* reset both prompts */
    FREE(this->prompt);
    FREE(preprompt);

    /* call mode specific docmd */
    if (this->gdb_docmd != NULL)
	this->gdb_docmd(this, cmd);
}

/** Set the cmd to be inserted at start of readline */
    void
gdb_setwinput(gdb_inst, cmd)
    gdb_handle_T *gdb_inst;
    char *cmd;	/* cmd to insert */
{
    gdb_T *this = (gdb_T *)gdb_inst;

    if (this == NULL)
	return;

    if (cmd == NULL)
	cmd = "";

    if (strchr(cmd, (int)NL) != NULL)	/* assert no NL in cmd */
	return;

    xfree(this->winput_cmd);
    this->winput_cmd = clewn_strsave(cmd);
}

/** Return TRUE if we are opening the gdb input-line window */
    int
gdb_iswinput(gdb_inst)
    gdb_handle_T *gdb_inst;
{
    return (gdb_inst != NULL ? ((gdb_T *)gdb_inst)->winput_cmd != NULL : FALSE);
}

/* Start a gdb process */
    static void
start_gdb_process()
{
    if (module_init() != OK)
	return;

    gdb->project_file = p_project;
    gdb->project_state = PROJ_INIT;

    exec_gdb();

    /* gdb instance count */
    ++gdb->instance;

    /* done later, after netbeans init, for the first gdb instance */
    if (gdb->instance != 1) {
	/* the gdb instance number is used to define signs in vim
	 * with a different name for each instance of gdb */
	cnb_set_instance(gdb->instance);

	/* empty the variables buffer */
	cnb_create_varbuf(gdb->var_name);

	/* free mode specific data within gdb_T */
	if (gdb->clear_gdb_T != NULL)
	    gdb->clear_gdb_T(gdb);

	/* unlink all asm buffers */
	cnb_unlink_asm();
    }
}

/*
 * Initialize this module: set inputrc file, define signs, compile regexp.
 * Return OK when succcess, FAIL otherwise.
 */
    static int
module_init()
{
    FILE *fd;
    char *fname = NULL;
    char *histsize;
    pattern_T *pat;
    int errcode;
    int len;
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
    char **p;
    FILE * stream;
#endif

    if (module_state == -1)
    {
	module_state = FAIL;		/* do it only once */

	/* build gvim and gdb exec strings */
	gdb_cat(&p_vim, p_vc);
	gdb_cat(&p_vim, GVIM_DEFAULT_ARG);
	if (p_project != NULL)
	    gdb_cat(&p_vim, "-c \"let clewn_project=1\" ");
	gdb_cat(&p_vim, p_va);

	gdb_cat(&p_gdb, p_gc);
	gdb_cat(&p_gdb, " ");
	gdb_cat(&p_gdb, p_ga);

	/* create a gdb_T instance */
	gdb = (gdb_T *)xcalloc(sizeof(gdb_T));
	gdb->version = PACKAGE_VERSION;

	clear_gdb_T();

	if (clewn_mktmpdir() != OK)	/* create the tmp directory */
	    goto fin;

	/* get clewn_dir */
	if ((clewn_dir = getenv("CLEWNDIR")) == NULL)
	    clewn_dir = getenv("HOME");

	/* load readline history */
	if (clewn_dir != NULL && *clewn_dir != NUL)
	{
	    gdb_cat(&fname, clewn_dir);
	    gdb_cat(&fname, CLEWN_HISTORY_FILE);
	    (void)read_history(fname);
	    FREE(fname);
	}

	/* set history max size */
	if ((histsize = getenv("HISTSIZE")) != NULL && (len = atoi(histsize)) > 0)
	    stifle_history(len);
	else
	    stifle_history(HISTORY_DFLT_SIZE);

#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
	/* Set gdb readline inputrc file contents
	 * We don't know yet if we are going to use GDB/MI, so we need to
	 * setup inputrc just in case, even though it might never be used */
	if ((stream = clewn_opentmpfile(INPUTRC_FNAME, &inputrc, 1)) != NULL)
	{
	    for (p = inputrc_array; *p; p++)
		fputs(*p, stream);
	    fclose(stream);
	    setenv("INPUTRC", inputrc, TRUE);
	}
#endif

	/* Compile patterns */
	for (pat = patterns; pat->str != NULL; pat++)
	{
	    pat->regprog = (regex_t *)xcalloc(sizeof(regex_t));

	    if ((errcode = regcomp(pat->regprog, pat->str, REG_EXTENDED)) != 0)
	    {
		print_regerror(errcode, pat->regprog);
		regfree(pat->regprog);
		FREE(pat->regprog);
		goto fin;
	    }
	}

	/* create variables file */
	if (p_gvar != NULL && *p_gvar != NUL)
	{
	    if ((fd = clewn_opentmpfile(p_gvar, &(gdb->var_name), 1)) != NULL)
		fclose(fd);
	}

	module_state = OK;
    }
fin:
    return module_state;
}

/* Release module resources */
    static void
module_end()
{
    char *fname = NULL;
    pattern_T *pat;
    int i;

    module_state = -1;

    xfree(p_vim);
    xfree(p_gdb);

# if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
    if (inputrc != NULL)
	FREE(inputrc);
# endif

    clewn_deltempdir();	    /* remove temporary directory and its content */

    xfree(preprompt);
    xfree(cplt_prmpt);
    xfree(key_command);

    /* patterns an tokens */
    for (pat = patterns; pat->str != NULL; pat++)
    {
	if (pat->regprog != NULL)
	{
	    regfree(pat->regprog);
	    FREE(pat->regprog);
	}
    }

    for (i = 0; i < CLEWN_KEYMAP_SIZE; i++)
	xfree(keymap[i]);

    /* write readline history */
    if (clewn_dir != NULL && *clewn_dir != NUL)
    {
	gdb_cat(&fname, clewn_dir);
	gdb_cat(&fname, CLEWN_HISTORY_FILE);
	(void)write_history(fname);
	xfree(fname);
    }
}

/* Initialize a gdb_T structure */
    static void
clear_gdb_T()
{
    if (gdb != NULL)
    {
	gdb->instance = 0;
	gdb->fd = -1;
	gdb->pid = (pid_t)-1;
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
	gdb->height = 0;
#endif
	gdb->state = GS_INIT;
	FREE(gdb->status);
	gdb->recurse = 0;

	gdb->cmd_type = CMD_ANY;
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
	gdb->cli_cmd.state = CS_START;
	gdb->cli_cmd.cnt = 0;
	FREE(gdb->cli_cmd.gdb);
	FREE(gdb->cli_cmd.readline);
	FREE(gdb->cli_cmd.echoed);
#endif

	FREE(gdb->firstcmd);	/* not used by clewn */
	FREE(gdb->prompt);
	FREE(gdb->pwd);
	FREE(gdb->args);
	FREE(gdb->winput_cmd);
	FREE(gdb->directories);
	FREE(gdb->sfile);

#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
	gdb->note = ANO_NONE;
	gdb->annoted = FALSE;
	gdb->newline = FALSE;
	FREE(gdb->annotation);
#endif
	FREE(gdb->line);
	FREE(gdb->pc);
	FREE(gdb->frame_pc);
	FREE(gdb->oob_result);
	FREE(gdb->asm_add);
	FREE(gdb->asm_func);

#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
	gdb->bp_state = 0;
	gdb_free_bplist(&(gdb->tmplist));
#endif
	FREE(gdb->record);
	gdb->cont = FALSE;
	gdb_free_bplist(&(gdb->bpinfo));
	gdb->frame_curlvl = -1;
	gdb->frame_lnum = (linenr_T) -1;
	FREE(gdb->frame_fname);

	gdb->fr_buf = -1;
	gdb->var_buf = -1;
	FREE(gdb->var_name);
	FREE(gdb->balloon_txt);

	gdb->oob.state = 0;
	gdb->oob.idx = -1;

	gdb->pool.buf = -1;
	FREE(gdb->pool.name);
	gdb->pool.hilite = FALSE;

	/* free mode specific data within gdb_T */
	if (gdb->clear_gdb_T != NULL)
	    gdb->clear_gdb_T(gdb);

	gdb->oobfunc = NULL;
	gdb->parse_output = NULL;
	gdb->gdb_docmd = NULL;
	gdb->var_delete = NULL;
	gdb->clear_gdb_T = NULL;
    }
}

/* Spawn a gdb process */
    static void
exec_gdb()
{
    char *err = NULL;
    int fd = -1;	/* slave pty file descriptor */
    char *tty;		/* pty name */
# ifdef HAVE_TERMIOS_H
    struct termios tio;
# else
    struct termio tio;
# endif

    /* process already running */
    if (gdb->pid != (pid_t)-1 && waitpid(gdb->pid, NULL, WNOHANG) == 0)
	return;

    /* Open pty */
    if ((gdb->fd = OpenPTY(&tty)) < 0
	    || (fd = open(tty, O_RDWR|O_NOCTTY|O_EXTRA, 0)) < 0
	    || SetupSlavePTY(fd) == -1)
    {
	err = "Cannot open gdb pty\n";
	goto err;
    }

    /* Set terminal attributes */
# ifdef HAVE_TERMIOS_H
    if (tcgetattr(fd, &tio) == 0)
# else
    if (ioctl(fd, TCGETA, &tio) >= 0)
# endif
    {
	tio.c_oflag &= ~ONLCR;		/* don't map NL to CR-NL on output */
	tio.c_cc[VINTR] = KEY_INTERUPT;
# ifdef HAVE_TERMIOS_H
	if (tcsetattr(fd, TCSAFLUSH, &tio) != 0)
# else
	if (ioctl(fd, TCSETA, &tio) < 0)
# endif
	{
	    err = "Cannot set gdb pty\n";
	    goto err;
	}
    }
    else
    {
	err = "Cannot get gdb pty\n";
	goto err;
    }

# if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
#  if defined(TIOCGWINSZ) && defined(TIOCSWINSZ)
	{
	    struct winsize win;

	    /* set tty height */
	    if (ioctl(0, TIOCGWINSZ, &win) >= 0)
	    {
		/* Prevent use of a low height size because of "prompt-for-continue" messages.
		 * The annotations lines printed by GDB are part of the lines count that
		 * is used by GDB for "prompt-for-continue" messages, however we do not
		 * display them. */
		if (win.ws_row < MIN_SCREEN_HEIGHT)
		{
		    fprintf(stderr, "Please retry with a larger screen,\n");
		    fprintf(stderr, "the screen height must be at least %d lines.\n", MIN_SCREEN_HEIGHT);
		    fprintf(stderr, "On a console, try the unix command: \"stty rows 16\" for example.\n");
		    goto err;
		}

		/* see the comment in gdb_parse_output_cli() about
		 * the "--More--" prompt */
		win.ws_row = LPP_LINES;
		if (ioctl(fd, TIOCSWINSZ, &win) >= 0)
		    gdb->height = win.ws_row;
	    }
	}
#  endif
# endif

    /* Fork */
    if ((gdb->pid = fork()) == (pid_t)-1)
    {
	err = "Cannot fork gdb\n";
	goto err;
    }
/* The child */
    else if (gdb->pid == (pid_t)0)
    {
	/* Grab control of terminal (from `The GNU C Library' (glibc-2.3.1)) */
	setsid();
# ifdef TIOCSCTTY
	if (ioctl(fd, TIOCSCTTY, (char *)NULL) == -1)
	    _exit(1);
# else
	{ int newfd;
	char *fdname = ttyname(fd);

	/* This might work (it does on Linux) */
	if (fdname)
	{
	    if (fd != 0)
		close (0);
	    if (fd != 1)
		close (1);
	    if (fd != 2)
		close (2);
	    newfd = open(fdname, O_RDWR);
	    close(newfd);
	}
	}
# endif

	close(0); dup(fd);
	close(1); dup(fd);
	close(2); dup(fd);

	if (fd > 2)
	    close(fd);

	close(gdb->fd);	    /* close master pty */

# ifdef GDB_MI_SUPPORT
	if (p_gdbmi)
	{
	    /* MI mi2 is available starting with GDB 6.0 */
	    execlp(p_gc, p_gc, "--interpreter=mi2", NULL);
	    _exit(EXIT_FAILURE);
	}
# endif
# if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
	clewn_exec(p_gdb);
# endif /* defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT) */

	_exit(EXIT_FAILURE);
    }
/* The parent */
    else
    {
	close(fd);

# ifdef GDB_MI_SUPPORT
	if (p_gdbmi)
	{
	    gdb->state |= GS_UP;
	    if (gdb_setup_mi(gdb) != OK)
	    {
		gdb_cat(&err, "Cannot start GDB program \"");
		gdb_cat(&err, p_gc);
		gdb_cat(&err, "\" (MI)\n");
		if (err != NULL)
		{
		    fprintf(stderr, err);
		    xfree(err);
		}
		gdb->state = GS_INIT;
	    }
	}
# else
#  if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
	fprintf(stderr, "guessing level, please wait...\r");
	
	gdb->state |= GS_UP;
	if (gdb_setup_cli(gdb) != OK)
	    gdb->state = GS_INIT;

#  endif
# endif

	return;
    }
err:
    if (gdb->fd >= 0) {
	close(gdb->fd);
	gdb->fd = -1;
    }
    if (fd >= 0)
	close(fd);
    if (err != NULL)
	fprintf(stderr, err);
    return;
}

/* Spawn a gdb process; return OK when sucess, FAIL otherwise */
    static int
exec_gvim()
{
    char dummy;
    int pipefd[2];	    /* pipe between parent and child */
    int pipe_error;
    pid_t pid;

    /* setup a pipe between the child and the parent, so that the parent
     * knows when the child has done the setsid() call and is allowed to exit */
    pipe_error = (pipe(pipefd) < 0);

    /* Fork */
    if ((pid = fork()) == (pid_t)-1)
    {
	fprintf(stderr, "Cannot fork gvim\n");
	return FAIL;
    }
    /* The child */
    else if (pid == (pid_t)0)
    {
	setsid();

	if (! pipe_error)
	{
	    close(pipefd[0]);
	    close(pipefd[1]);
	}

	close(0);
	close(1);
	close(2);
	cnb_close();	    /* close sockets */

	clewn_exec(p_vim);

	_exit(EXIT_FAILURE);
    }
    /* The parent */
    else
    {
	if (! pipe_error)
	{
	    /* the read returns when the child closes the pipe (or when
	     * the child dies for some reason) */
	    close(pipefd[1]);
	    (void)read(pipefd[0], &dummy, (size_t)1);
	    close(pipefd[0]);
	}

	return OK;
    }
}

/* Initialize key mappings */
    static void
clewn_keymap()
{
    static int keymap_done  = 0;
    char *fname = NULL;
    int key;
    char ** line;
    char * ptr;
    char ** default_keymap;

    /* map the keys in vim - do it only once and when the data socket is opened */
    if (! keymap_done && cnb_state()) {
	keymap_done = 1;

	/* use the default table, depending on netbeans version */
	if (cnb_specialKeys())
	    default_keymap = keys_mapping;
	else
	    default_keymap = old_keys_mapping;

	/* read the default keys mapping */
	for (line = default_keymap; *line != NULL; line++)
	{
	    /* make a copy so that it's writeable */
	    if ((ptr = clewn_strsave(*line)) != NULL)
	    {
		parse_keyline(ptr);
		xfree(ptr);
	    }
	}

	/* read the key mappings user configuration file*/
	if (clewn_dir != NULL && *clewn_dir != NUL)
	{
	    gdb_cat(&fname, clewn_dir);
	    gdb_cat(&fname, CLEWN_KEYS_FILE);
	    read_keysfile(fname);
	    xfree(fname);
	}

	/* if the netbeans version supports specialKeys */
	if (cnb_specialKeys()) {
	    /* control chars */
	    for (key='A'; key<='_'; key++)
		if (keymap[key - '@'] != NULL)
		    cnb_keymap(KEYMAP_CONTROL, key);

	    /* upper case chars */
	    for (key='A'; key<='Z'; key++)
		if (keymap[key] != NULL)
		    cnb_keymap(KEYMAP_UPPERCASE, key);

	    /* function keys */
	    for (key=1; key<=FUNMAP_SIZE; key++)
		if (keymap[FUNCTION_INDEX(key)] != NULL)
		    cnb_keymap(KEYMAP_FUNCTION, key);
	}
    }
}

/* Write project file */
    static void
write_project()
{
    char * res = NULL;
    int bp_count = 0;
    FILE *stream;
    bpinfo_T *p;
    char * source_file;
    int lnum;

    if (gdb->project_file == NULL)	/* no project file */
	return;

    if (gdb->sfile == NULL) {		/* no debuggee */
	fprintf(stderr, "Error: no target file, the project is not written to: %s\n", gdb->project_file);
	goto end;
    }

    if ((stream = fopen(gdb->project_file, "w+")) != NULL) {
	fputs("# Project file automatically generated by clewn.\n", stream);

	if (gdb->pwd != NULL)		/* current working directory */
	    fputs(gdb->pwd, stream);

	gdb_cat(&res, "file ");
	gdb_cat(&res, gdb->sfile);
	gdb_cat(&res, "\n");
	fputs(res, stream);		/* debuggee */
	FREE(res);

	if (gdb->args != NULL)		/* debuggee command line args */
	    fputs(gdb->args, stream);

	for (p = gdb->bpinfo; p != NULL; p = p->next) {
	    if ((source_file=cnb_filename(p->buf)) != NULL) {
		bp_count++;

		/* get the new sign position in the vim buffer */
		lnum = cnb_buf_getsign(p->buf, BP_SIGN_ID(p->id));
		if (lnum == 0)
		    lnum = (int)p->lnum;    /* use the original value */

		gdb_cat(&res, "break ");
		gdb_cat(&res, source_file);
		gdb_cat(&res, ":");
		gdb_cat(&res, gdb_itoa(lnum));
		gdb_cat(&res, "\n");
		fputs(res, stream);	/* breakpoint */
		FREE(res);

		/* handle disabled breakpoints */
		if (! p->enabled) {
		    gdb_cat(&res, "disable ");
		    gdb_cat(&res, gdb_itoa(bp_count));
		    gdb_cat(&res, "\n");
		    fputs(res, stream);	/* disabled breakpoint */
		    FREE(res);
		}
	    }
	}

	fclose(stream);
    }
    else
	fprintf(stderr, "Error: cannot write project in %s\n", gdb->project_file);

end:
    /* gdb_close may be called more than one time on exit,
     * on the second occurence, the breakpoint list has been freed,
     * prevent this to happen by setting the project_file to NULL */
    gdb->project_file = NULL;
}

#define CG_QUIT "quit\n"
#define CG_YES  "yes\n"
#define CG_POLL 100
/* Close gdb process */
    void
gdb_close(this)
    gdb_T *this;
{
    bpinfo_T *p;
    pid_t pid;
    int rc;

    if (this->state & GS_CLOSING)	/* prevent recursive calls */
	return;
    this->state |= GS_CLOSING;

    /* write the project file */
    write_project();

    /* remove all signs */
    gdb_fr_unlite(this);
    for (p = gdb->bpinfo; p != NULL; p = p->next)
	cnb_buf_delsign(p->buf, BP_SIGN_ID(p->id));

    /* free breakpoints table */
    gdb_free_bplist(&(this->bpinfo));

    /*  a) attempt to gracefully terminate gdb process
     *  b) if this fails, SIGTERM it
     *  c) if this fails, too bad, just return */
    if (this->pid != (pid_t)-1)
    {
	pid = waitpid(this->pid, NULL, WNOHANG);

	if ((pid == (pid_t)-1 && errno == ECHILD) || pid == this->pid)
	    ;	/* nop */
	else	/* still running */
	{
	    char c     = KEY_INTERUPT;
	    int killed = FALSE;
	    int t;

	    /* a) write an interrupt followed by a 'quit' cmd */
	    write(this->fd, &c, 1);
	    if (gdb_read(this, clewn_buf, MAX_BUFFSIZE, 1000) >= 0)
	    {
		write(this->fd, CG_QUIT, strlen(CG_QUIT));
		while ((rc = gdb_read(this, clewn_buf, MAX_BUFFSIZE, 100)) > 0)
		    ;

		if (rc != -1)
		    write(this->fd, CG_YES, strlen(CG_YES));
	    }

	    /* make sure gdb is terminated: poll for waitpid() */
	    for (t = 0; !killed; t += CG_POLL)
	    {
		/* 1 second elapsed since start of polling for waitpid */
		if (t >= 1000 )
		{
# ifdef SIGTERM
		    /* b) kill it now */
		    kill(this->pid, SIGTERM);
# endif
		    killed = TRUE;
		}

		clewn_sleep(CG_POLL);
		pid = waitpid(this->pid, NULL, WNOHANG);
		if ((pid == (pid_t)-1 && errno == ECHILD) || pid == this->pid)
		    break;
	    }
	}
	clear_readline();
	fprintf(stderr, "GDB terminated\n");
    }

    if (this->fd >= 0) {
	close(this->fd);
	this->fd = -1;
    }

    /* keep the GS_QUITTING state of gdb; this state sets the difference
     * between a 'quit' command, and a 'restart' command */
    this->state = GS_INIT | (this->state & GS_QUITTING);
    this->pid = (pid_t)-1;
}

/* Highlite asm_add line; return TRUE when asm_add found in asm buffer */
    int
gdb_as_frset(this, obs)
    gdb_T *this;
    struct obstack *obs;
{
    int bufno;
    char *fname;
    linenr_T lnum;

    if (this->asm_func == NULL || this->asm_add == NULL)
	return FALSE;

    /* Search all buffers whose name start with this->asm_func for ptrn */
    for (bufno = 1; ! cnb_outofbounds(bufno); bufno++)
    {
	if ((fname = cnb_filename(bufno)) != NULL
		&& strstr(fname, this->asm_func) != NULL
		&& (lnum = searchfor(fname, this->asm_add)) > 0)
	{
	    this->pool.buf = bufno;
	    this->pool.lnum = lnum;

	    if (this->pool.hilite)
		gdb_fr_set(this, NULL, NULL, obs);

	    FREE(this->asm_add);
	    return TRUE;
	}
    }
    return FALSE;
}

/*
 * Highlight line within frame
 * Return -1 when failing to load the buffer, 0 otherwise
 */
    int
gdb_fr_set(this, file, line, obs)
    gdb_T *this;
    char *file;
    linenr_T *line;
    struct obstack *obs;
{
    int buf = -1;
    linenr_T lnum;

    /* Do not set frame hilite when this breakpoint has a 'commands'
     * with a 'continue' statement */
    if (this->cont)
    {
	this->cont = FALSE;
	return 0;
    }

    if (line == NULL)		/* in asm window */
    {
	buf = this->pool.buf;
	lnum = this->pool.lnum;
    }
    else			/* in source file */
	lnum = *line;

    if (cnb_isvalid_buffer(buf) || file != NULL)
    {
	if ((buf = gdb_edit_file(buf, file, lnum, 0, obs)) > 0)
	    gdb_fr_lite(this, buf, lnum, obs);
	else
	    return -1;
    }
    return 0;
}

/* Highlite frame */
    void
gdb_fr_lite(this, buf, lnum, obs)
    gdb_T *this;
    int buf;		/* buffer number where to highlite */
    linenr_T lnum;	/* line number */
    struct obstack *obs;
{
    if (! cnb_isvalid_buffer(buf) || lnum <= 0)
	return;

    /*
     * Remove previous frame sign:
     * GDB sends ANO_FRAME_INVALID annotations whenever stepping, running, etc...
     * and these annotations invoke gdb_fr_unlite() that turn off the previous frame sign.
     * But when moving along the stack frame with GDB 'up', 'down', 'frame' commands,
     * we don't get annotations and must turn off the previous frame sign.
     */
    if (this->fr_buf > 0)
	gdb_unlite(FRAME_SIGN);

    this->fr_buf = buf;
    this->lnum = lnum;

    /* add new frame highlite */
    cnb_buf_addsign(buf, FRAME_SIGN, FRAME_SIGN, lnum, obs);
}

/* Unlite frame */
    void
gdb_fr_unlite(this)
    gdb_T *this;
{
    if (! cnb_isvalid_buffer(this->fr_buf))
	return;

    this->fr_buf = -1;
    this->lnum = 0;
    cnb_buf_delsign(0, FRAME_SIGN);
}

/* Print an error message */
    void
EMSG (m)
    char *m;
{
    fputs(m, rl_outstream);
    fputs("\n", rl_outstream);
}

/*
 * Find the line number of the FRAME_SIGN in this buffer.
 * Return 0 as the line number when error.
 */
    linenr_T
find_fr_sign(buf)
    int buf;
{
    if (gdb == NULL || ! cnb_isvalid_buffer(gdb->fr_buf) || buf != gdb->fr_buf)
	return 0;

    return gdb->lnum;
}

/* Unlite a sign. */
    void
gdb_unlite(id)
    int id;		/* sign id */
{
    bpinfo_T *p;

    if (gdb == NULL || id <= 0)
	return;

    if (id == FRAME_SIGN)
	cnb_buf_delsign(0, id);
    else
    {
	/* look for buf number in bp table */
	for (p = gdb->bpinfo; p != NULL; p = p->next)
	    if (BP_SIGN_ID(p->id) == id)
	    {
		cnb_buf_delsign(p->buf, id);
		break;
	    }
    }
    return;
}

/*
 * Define a breakpoint sign. There is one sign type per breakpoint
 * sign in order to have breakpoints numbers as the sign text.
 * Returns sign type number or -1 if error.
 */
    int
gdb_define_bpsign(bp, obs)
    bpinfo_T *bp;	/* the breakpoint */
    struct obstack *obs;
{
    int typenr;

    if (bp == NULL)
	return -1;

    /* do not define it twice, use the previous sequence number if any */
    if (bp->enabled && bp->typenr_en > 0)
	return bp->typenr_en;
    else if (! bp->enabled && bp->typenr_dis > 0)
	return bp->typenr_dis;

    if ((typenr = cnb_define_sign(bp->buf, bp->id,
		    (bp->enabled ? SIGN_BP_ENABLED : SIGN_BP_DISABLED), obs)) > 0)
    {
	if (bp->enabled)
	    bp->typenr_en = typenr;
	else
	    bp->typenr_dis = typenr;
    }

    return typenr;
}

/* Append to or replace last line on rl_outstream */
    void
gdb_write_buf(this, chunk, add)
    gdb_T *this;
    char *chunk;	/* a chunk may contain one, many or no NL */
    int add;		/* TRUE when chunk is added */
{
    if (chunk == NULL || this == NULL)
	return;

    /* do not echo the prompt as it is already done by readline */
    if (this->note == ANO_PREPROMPT
	    || this->note == ANO_PRECMDS
	    || this->note == ANO_PREQUERY
	    || this->note == ANO_PREOVERLOAD)
    {
	/* stores the prompt in case it is a multi line prompt */
	xfree(preprompt);
	preprompt = clewn_strsave(chunk);
	return;
    }

    /* hack to handle ANO_PRECMDS missing from GDB
     * avoid printing a superfluous ">" line after completion
     * in a "commands" or "define" */
    if (this->note == ANO_NONE
	    && this->cli_cmd.state == CS_CHOICE
	    && strcmp(chunk, ">") == 0)
	return;

    /* do not echo user input as it is already done by readline */
    if (this->note == ANO_PROMPT
	    || this->note == ANO_CMDS
	    || this->note == ANO_QUERY
	    || this->note == ANO_OVERLOAD)
	return;

    if (add)
    {
	if (nl_output)		/* newline already output by readline */
	    nl_output = FALSE;
	else
	    fputs("\n", rl_outstream);
    }
    else
	fputs("\r", rl_outstream);	/* overwrite last line */

    if (preprompt == NULL)
    {
	/* write chunk */
	fputs(chunk, rl_outstream);
    }
    else
    {
	/* a multi line prompt: print the previous line from the multi
	 * line prompt instead of this last prompt line
	 * a multi line prompt occurs after GDB command 'run' when
	 * debuggee is already started */
	if (gdb->prompt == NULL)
	{
	    fputs(preprompt, rl_outstream);

	    /* now store chunk as the previous line
	     * from the multi line prompt */
	    xfree(preprompt);
	    preprompt = clewn_strsave(chunk);
	}
	else
	{
	    fputs(chunk, rl_outstream);
	    FREE(preprompt);
	}
    }
}

/*
 * Fill up buff with a NUL terminated string of max size - 1 bytes from gdb.
 * Return bytes read count, -1 if error or zero for nothing to read.
 */
    int
gdb_read(this, buff, size, wtime)
    gdb_T *this;
    char *buff;	/* where to write */
    int size;		/* buff size */
    int wtime;		/* msecs time out, -1 wait forever */
{
    int len;
    int rc;
# ifndef HAVE_SELECT
    struct pollfd fds;

    fds.fd = this->fd;
    fds.events = POLLIN;
# else
    struct timeval tv;
    struct timeval start_tv;
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(this->fd, &rfds);

#  ifdef HAVE_GETTIMEOFDAY
    if (wtime >= 0)
	gettimeofday(&start_tv, NULL);
#  endif
# endif

    if (size <= 0 || buff == NULL || ! GDB_STATE(this, GS_UP))
	return -1;

    /* make sure there is some data to read */
    while (1)
    {
	if (this->state & GS_SIGCHLD)
	    goto close;

# ifndef HAVE_SELECT
	if ((rc = poll(&fds, 1, wtime)) > 0)
# else
	if (wtime >= 0)
	{
	    tv.tv_sec = wtime / 1000;
	    tv.tv_usec = (wtime % 1000) * (1000000/1000);
	}

	if ((rc = select(this->fd + 1, &rfds, NULL, NULL, (wtime >= 0) ? &tv : NULL)) > 0)
# endif
	    break;

	if (rc == -1 && errno == EINTR)
	{
	    if (wtime >= 0)
	    {
		/* compute remaining wait time */
# if ! defined(HAVE_SELECT) || ! defined(HAVE_GETTIMEOFDAY)
		/* guess: interrupted halfway, gdb processing 10 msecs */
		wtime = wtime / 2 - 10L;
# else
		gettimeofday(&tv, NULL);
		wtime -= (tv.tv_sec - start_tv.tv_sec) * 1000L
				+ (tv.tv_usec - start_tv.tv_usec) / 1000L;
# endif
		if (wtime < 0)
		    return 0;
	    }
	}
	else if (rc == 0)
	    return 0;
	else
	    goto close;
    }

    /* read the data */
    if ((len = read(this->fd, (char *)buff, size - 1)) < 0)
	goto close;

    buff[len] = NUL;
    return len;
close:
    gdb_close(this);
    return -1;
}

/*
 * Edit a file.
 * Use buf number if positive, otherwise fname.
 * Return the buffer number or -1 when error.
 */
    int
gdb_edit_file(buf, fname, lnum, silent, obs)
    int buf;		/* asm buffer number to edit */
    char *fname;	/* file name */
    linenr_T lnum;	/* line number */
    int silent;
    struct obstack *obs;
{
    if (gdb == NULL)
	return -1;

    if (! cnb_isvalid_buffer(buf) && (fname == NULL || *fname == NUL))
	return -1;

    if (cnb_isvalid_buffer(buf) && (fname = cnb_filename(buf)) == NULL)
	return -1;

#ifdef GDB_LVL3_SUPPORT
    return cnb_editFile(fname, lnum, gdb->directories, gdb->lvl3.source_cur,
					    gdb->lvl3.source_list, silent, obs);
#else
    return cnb_editFile(fname, lnum, gdb->directories, NULL, NULL, silent, obs);
#endif
}

/* Do the OOB_COMPLETE part of an oob cmd and send the next one */
    void
gdb_oob_send(this, obs)
    gdb_T *this;
    struct obstack *obs;
{
    int keep    = FALSE;    /* when TRUE, do not switch to next oob function */
    char *res   = NULL;
    int *pi     = &(this->oob.idx);
    int s_a     = (this->state & GS_ALLOWED);

    /* prevent recursive calls to parse_output() since breakpoint
     * or frame highlighting may cause Vim to query the user when
     * changes have been made in the previous buffer */
    this->state &= ~GS_ALLOWED;

    if (this->oobfunc == NULL)
	return;

    if (*pi == -1)
    {
	this->oob.state &= ~OS_INTR;
	if (this->oob.state & OS_QUIT)
	    goto quit;
    }

    if (*pi >= 0 && (this->oobfunc)[*pi].oob != NULL) /* assert != NULL */
    {
	if ((this->oobfunc)[*pi].oob(this, OOB_COMPLETE, NULL, obs) != NULL)
	    keep = TRUE;

	if (this->oob.state & OS_QUIT)
	    goto quit;
    }

    if (! keep)
	++(*pi);

    while ((this->oobfunc)[*pi].oob != NULL && !(this->oob.state & OS_INTR))
    {
	if ((res = (this->oobfunc)[*pi].oob(this, OOB_CMD, NULL, obs)) != NULL)
	{
	    this->oob.cnt = 0;

	    /* send the command to GDB */
	    write(this->fd, (char *)res, strlen(res));

	    this->state &= ~GS_ALLOWED;
	    if (s_a)
		this->state |= GS_ALLOWED;
	    return;
	}

	++(*pi);
    }

    *pi = -1;
quit:
    this->oob.state &= ~OS_CMD;
    this->oob.state &= ~OS_QUIT;

    this->state &= ~GS_ALLOWED;
    if (s_a)
	this->state |= GS_ALLOWED;
}

/* Receive out of band response to idx cmd */
    void
gdb_oob_receive(this, chunk, obs)
    gdb_T *this;
    char *chunk;	/* response (possibly incomplete) */
    struct obstack *obs;
{
    char *res = NULL;
    int s_a = (this->state & GS_ALLOWED);

    /* prevent recursive calls to parse_output() since breakpoint
     * or frame highlighting may cause Vim to query the user when
     * changes have been made in the previous buffer */
    this->state &= ~GS_ALLOWED;

    if (this->oobfunc == NULL)
	return;

    if(IS_OOBACTIVE(this))
    {
	/* silently discard when interrupted */
	if (!(this->oob.state & OS_INTR) && chunk != NULL)
	{
	    if (this->parser != PS_PREPROMPT && this->parser != PS_PROMPT
		&& (this->oobfunc)[this->oob.idx].oob != NULL) /* assert != NULL */
	    {
		this->oob.cnt++;
		(void)(this->oobfunc)[this->oob.idx].oob(this, OOB_COLLECT, chunk, obs);

		this->state &= ~GS_ALLOWED;
		if (s_a)
		    this->state |= GS_ALLOWED;
		return;
	    }
	}

	/* keep the last prompt */
	if (this->parser == PS_PREPROMPT)
	{
	    gdb_cat(&res, this->line);
	    gdb_cat(&res, chunk);
	    xfree(this->line);
	    this->line = res;
	}
    }

    this->state &= ~GS_ALLOWED;
    if (s_a)
	this->state |= GS_ALLOWED;
}

/*
 * Remove in the bpinfo list the records corresponding to the signs in bufno,
 * the signs themselves are removed by Vim in free_buffer()
 */
    void
gdb_free_records(bufno)
    int bufno;
{
    bpinfo_T *p, **pt;

    if (gdb == NULL)
	return;

    for (pt = &(gdb->bpinfo); *pt != NULL; )
    {
	p = *pt;
	if (p->buf == bufno)
	{
	    *pt = p->next;	/* unlink record */
	    xfree(p);
	}
	else
	    pt = &(p->next);
    }
}

/* Free a bpinfo_T list and set address referenced by plist to NULL */
    void
gdb_free_bplist (plist)
    bpinfo_T ** plist;
{
    bpinfo_T *p, *next;

    if (plist == NULL)
	return;

    for (p = *plist; p != NULL; p = next)
    {
	next = p->next;
	xfree(p);
    }

    *plist = NULL;
}

/* Get the GDB command type */
    void
gdb_cmd_type(this, cmd)
    gdb_T *this;
    char *cmd;
{
    token_T *tok;
    char *ptr;
    char *last;
    char *tail;

    this->cmd_type = CMD_ANY;

    if (cmd == NULL)
	return;

    while (isspace(*cmd))
	cmd++;

    if (*cmd == NUL)
	return;

    /* find the token 'keyword:tail' that matches cmd */
    for (tok = tokens; tok->keyword != NULL; tok++)
    {
	if (strstr(cmd, tok->keyword) == cmd)
	{
	    last = ptr = cmd + strlen(tok->keyword);

	    /* get first space after token */
	    while (*last && ! isspace(*last))
		last++;

	    /* get a copy of last part */
	    if (last != ptr)
	    {
		tail = clewn_strnsave(ptr, last - ptr);

		if (strstr(tok->tail, tail) != tok->tail)
		{
		    xfree(tail);
		    continue;	/* tail do not match */
		}
		xfree(tail);
	    }

	    /* match */
	    this->cmd_type = tok->type;
	    break;
	}
    }
}

/* Create a private temporary directory */
    static int
clewn_mktmpdir()
{
    char *(tempdirs[]) = {TEMPDIRNAMES};
    char itmp[TEMPNAMELEN];
    char *tmpdir;
    char *buf;
    mode_t umask_save;
    DIR *dir;
    int ret;
    unsigned i;
    long nr;
    long off;

    if (clewn_tempdir == NULL)
    {
	/* try the entries in TEMPDIRNAMES to create the temp directory */
	for (i = 0; i < sizeof(tempdirs) / sizeof(char *); i++)
	{
	    if ((*(tempdirs[i]) == '$' && (tmpdir = getenv(tempdirs[i] + 1)) != NULL)
		    || (*(tempdirs[i]) != '$' && (tmpdir = tempdirs[i])))
	    {
		/* leave room for "/c1dddddd" and check directory exists*/
		if (strlen(tmpdir) < TEMPNAMELEN - 10
			&& (dir = opendir(tmpdir)) != NULL)
		{
		    closedir(dir);
		    nr = (long)getpid() % 1000000L;

		    /* try up to 10000 different values */
		    for (off = nr; off < nr + 10000L; off++)
		    {
			sprintf(itmp, "%s/c%ld", tmpdir, off);

			umask_save = umask(077);
			ret = mkdir(itmp, 0700);
			(void)umask(umask_save);

			if (ret == 0)
			{
			    /* expand to full path */
			    buf = (char *)xmalloc(MAXPATHL + 1);

			    if (! clewn_fullpath(itmp, buf, MAXPATHL, FALSE))
				strcpy(buf, itmp);

			    if (*buf != NUL && *(buf + strlen(buf) - 1) != '/')
				strcat(buf, "/");

			    clewn_tempdir = clewn_strsave(buf);
			    xfree(buf);

			    return clewn_tempdir != NULL ? OK : FAIL;
			}
			else if (errno != EEXIST)
			    break; /* probably can't create any dir here, try another place */
		    }
		}
	    }
	}
    }
    else
	return OK;

    return FAIL;
}

/*
 * Create and open a file in the temporary directory whose name
 * is prefixed by 'filename'. The new name is 'filename{nb}' where
 * 'nb' are digits appended to 'filename' so that it's a new file.
 * When 'fullpath' is not NULL and operation was successful, the
 * address pointed to by 'fullpath' is set to an allocated full
 * pathname of this file.
 * Return a pointer to the stream, NULL when failed.
 */
    FILE *
clewn_opentmpfile(filename, fullpath, instance)
    char *filename;
    char **fullpath;
    int instance;
{
    FILE *stream = NULL;
    char *res    = NULL;
    int i        = instance;
    struct stat st;
    char *buf;

    if (clewn_tempdir != NULL && filename != NULL && *filename != NUL)
    {
	/* find a non existing file name in this directory */
	do
	{
	    buf = gdb_itoa(i);
	    if (i == 1)
		*buf = NUL;
	    i++;

	    FREE(res);
	    gdb_cat(&res, clewn_tempdir);
	    gdb_cat(&res, buf);
	    gdb_cat(&res, filename);
	} while (stat((char *)res, &st) == 0);

	/* open the file */
	if ((stream = fopen(res, "w+")) != NULL && fullpath != NULL)
	    *fullpath = res;
	else
	    xfree(res);
    }
    return stream;
}

/* Remove Clewn temporary directory and its content */
    static void
clewn_deltempdir()
{
    char itmp[TEMPNAMELEN];
    DIR *dp;
    struct dirent *ep;

    if (clewn_tempdir != NULL)
    {
	if ((dp = opendir(clewn_tempdir)) != NULL)
	{
	    while ((ep = readdir(dp)) != NULL)
	    {
		sprintf(itmp, "%s%s", clewn_tempdir, ep->d_name);
		(void)unlink(itmp);
	    }

	    (void)closedir(dp);
	    (void)rmdir(clewn_tempdir);
	}

	FREE(clewn_tempdir);
    }
}

/*
 * Append src to string pointed to by pdest or copy src to a new allocated
 * string when *pdest is NULL.
 * *pdest is reallocated to make room for src.
 * Append an empty string when src is NULL.
 */
    void
gdb_cat(pdest, src)
    char **pdest;	/* string address to append to */
    char *src;		/* string to append */
{
    int ldest = (*pdest != NULL ? strlen(*pdest) : 0);
    int lsrc  = (src != NULL ? strlen(src) : 0);
    char *res;

    if (lsrc != 0 || *pdest == NULL)
    {
	res = (char *)xmalloc(ldest + lsrc + 1);

	if (ldest == 0)
	{
	    if (lsrc != 0)
		strcpy(res, src);
	    else
		strcpy(res, "");
	}
	else
	{
	    strcpy(res, *pdest);
	    strcat(res, src);	/* assert src != NULL */
	}

	xfree(*pdest);
	*pdest = res;
    }
}

/* Print the regexp error message string to stderr. */
    static void
print_regerror(errcode, compiled)
    int errcode;
    regex_t *compiled;
{
    size_t length = regerror(errcode, compiled, NULL, 0);
    char *buffer;

    buffer = (char *)xmalloc(length);
    (void)regerror(errcode, compiled, buffer, length);
    fprintf(stderr, buffer);
    xfree(buffer);
}

/*
 * Return an allocated string that is the sub-match indexed by subid ([0-9])
 * using compiled pattern id. The string is allocated in an obstack when
 * obs is not NULL.
 * Return NULL if str does not match (or no such sub-match in pattern).
 */
    char *
gdb_regexec(str, id, subid, obs)
    char *str;		    /* string to match against */
    int id;		    /* pattern id */
    int subid;		    /* sub-match index */
    struct obstack *obs;    /* obstack to use for allocating memory */
{
    regmatch_t match[10];
    pattern_T *pat;

    if (str == NULL || *str == NUL || subid < 0 || subid > 9)
	return NULL;

    for (pat = patterns; pat->str != NULL; pat++)
    {
	if (pat->id == id)
	{
	    if (pat->regprog != NULL && regexec(pat->regprog, str, 10, match, 0) == 0
		    && match[subid].rm_eo != -1
		    && match[subid].rm_so != -1)
	    {
		if (obs != NULL)
		    return (char *)obstack_copy0(obs, str + match[subid].rm_so,
			    (int)(match[subid].rm_eo - match[subid].rm_so));
		else
		    return clewn_strnsave(str + match[subid].rm_so,
			    (int)(match[subid].rm_eo - match[subid].rm_so));
	    }
	    break;
	}
    }
    return NULL;
}

/* Return an integer as a string */
    char *
gdb_itoa(i)
    int i;		/* integer to stringify */
{
    static char buf[NUMBUFLEN];

    sprintf(buf, "%ld", (long)i);
    return buf;
}

    char *
clewn_stripwhite(string)
    char *string;
{
    char *s;
    char *t;

    for (s = string; isspace(*s); s++)
	;

    if (*s == NUL)
	return s;

    t = s + strlen(s) - 1;
    while (t > s && isspace(*t))
	t--;
    *++t = NUL;

    return s;
}

/* Clear realine and set cursor at line start */
    static void
clear_readline()
{
    int len = 0;

    if (gdb->prompt != NULL)	    /* find out how many spaces are needed */
	len = strlen(gdb->prompt);
    if (rl_line_buffer != NULL)
	len += strlen(rl_line_buffer);

    len = (len < MAX_BUFFSIZE ? len: MAX_BUFFSIZE -1);
    memset((void *)clewn_buf, (int)' ', (size_t)len);
    clewn_buf[len] = NUL;

    fprintf(rl_outstream, "\r%s\r", clewn_buf);    /* clear the line */
    fflush(rl_outstream);
}

/* Add line to readline history after having stripped whites and
 * removed a previous identical entry in the history */
    static void
clewn_add_history(line)
    char *line;
{
    HIST_ENTRY * entry;
    char *str;
    int offset;

    /* make a copy */
    str = clewn_strsave(line);

    line = clewn_stripwhite(str);
    if (*line == NUL)
    {
	xfree(str);
	return;
    }

    for (offset = 0; offset + history_base < history_length
	    && (offset = history_search_pos(line, 1, offset)) >= 0; )
    {
	if((entry = history_get(offset + history_base)) != NULL
		&& entry->line != NULL
		&& strcmp(line, entry->line) == 0
		&& (entry = remove_history(offset)) != NULL)
	{
	    if (entry->line != NULL)
		free(entry->line);
	    free(entry);
	}
	else
	    offset++;
    }

    add_history(line);
    xfree(str);
}

/*
 * Parse a line of key mapping
 * Key definitions are of the form `KEY:GDB COMMAND:{%line|%text}'
 * where:
 *   %text: the text below the mouse is added to the GDB command
 *   %line: 'fname:lnum' are added to the GDB command, where fname is the
 *          current buffer full pathname, and lnum the line number at
 *          cursor position
 *
 * All characters following `#' up to the next new line are ignored.
 * Leading blanks on each line are ignored. Empty lines are ignored.
 *
 * Supported key names:
 *  key functions: F1 to F20, example: `F11:continue'
 *  all ASCII printable characters except '#'
 *  control characters written as C-...
 */
    static void
parse_keyline(line)
    char * line;
{
    char *end;
    int key;
    int fnum;

    while (isspace(*line))			/* skip blanks */
	line++;

    key = *line++;
    if (key > 0 && key < 256 && isalnum(key))
    {
	if (key == '#')				/* a comment */
	    return;

	else if (key == 'F' && *line != ':')	/* a function key */
	{
	    fnum = (int)strtol((char *)line, &end, 10);

	    /* advance ptr to next separator */
	    if (*end != ':')
		return;
	    line = end;

	    /* set key to fnum function key index in table */
	    if ((key = FUNCTION_INDEX(fnum)) == -1)
		return;
	}

	else if (key == 'C' && *line == '-') {	/* a control character */
	    line++;
	    key = *line++;
	    if (key < 'A' || key > '_')
		return;
	    key = key - '@';
	}

	/* parse GDB command, it must be surrounded by ':' and not empty */
	if (*line++ != ':')
	    return;
	/* remove a key mapping when the GDB command is empty */
	else if (*line == ':') {
	    FREE(keymap[key]);
	    return;
	}
	else if ((end = strchr(line, ':')) == NULL)
	    return;

	*end++ = NUL;		    /* terminate command string */

	/* set GDB command in table */
	xfree(keymap[key]);
	keymap[key] = clewn_strsave(line);

	/* parse %line or %text */
	if (strstr(end, "%line") == end)
	    SET_KEY_LINE(keyline, key);
	else if (strstr(end, "%text") == end)
	    SET_KEY_LINE(keytext, key);
    }
}

/*
 * Read the key mappings file and build the keymap array and
 * bit map flags.
 */
    static void
read_keysfile(fname)
    char *fname;
{
    FILE *fd;
    char *ptr;

    if ((fd = fopen(fname, "r")) == NULL)
	return;

    /* read the key mappings file */
    while ((ptr = fgets(clewn_buf, MAX_BUFFSIZE, fd)) != NULL)
	parse_keyline(ptr);

    fclose(fd);
}

/* Process a netbeans event */
    static void
process_nb_event()
{
    nb_event_T * event;

    if ((event = cnb_data_evt()) != NULL)
    {
	/* process a keyAtPos event */
	if (event->key != NULL) {
	    xfree(key_command);
	    key_command = get_keymap(event);
	}

	/* process a balloonText event */
	else if (event->text_event && gdb != NULL && ! IS_OOBACTIVE(gdb)) {

	    xfree(gdb->balloon_txt);
	    if ((gdb->balloon_txt = clewn_strsave(event->text)) != NULL) {

		/* need a static array since gdb->oobfunc is referenced later
		 * by gdb_oob_receive */
		static oobfunc_T print_value_oobfunc[] = {
		    {gdb_print_value},
		    {NULL}
		};
		struct obstack obs;

		(void)obstack_init(&obs);

		/* the standard oobfunc table will be reset
		 * after the prompt processing in process_annotation() */
		gdb->oobfunc = print_value_oobfunc;
		gdb_oob_send(gdb, &obs);

		obstack_free(&obs, NULL);
	    }
	}

	event->text_event = FALSE;	/* maybe lost when IS_OOBACTIVE */
    }
}

/*
 * Use keyinfo to build the corresponding GDB command that is
 * found in keymap[].
 * If this key is flagged as `%line': build `command pathname:lnum'
 * If this key is flagged as `%text': build `command text`
 * Return an allocated GDB command.
 */
    static char *
get_keymap(keyinfo)
    nb_event_T *keyinfo;
{
    char *res = NULL;
    char *ptr;
    char *end;
    int fnum;
    int key;

    if (keyinfo == NULL || (ptr = keyinfo->key) == NULL || *ptr == NUL)
	return NULL;

    key = *ptr++;

    if (key == 'F' && *ptr != NUL)	/* a function key */
    {
	fnum = (int)strtol((char *)ptr, &end, 10);

	if (*end != NUL)
	    return NULL;

	/* set key to fnum function key index in table */
	if ((key = FUNCTION_INDEX(fnum)) == -1)
	    return NULL;
    }

    else if (key == 'S' && *ptr == '-') {/* an uppercase key */
	key = *++ptr;
	if (key < 'A' || key > 'Z')
	    return NULL;
    }

    else if (key == 'C' && *ptr == '-') {/* a control character */
	key = *++ptr;
	if (key < 'A' || key > '_')
	    return NULL;
	key = key - '@';
    }

    else if (*ptr != NUL)		/* only one character keys */
	return NULL;

    if (keymap[key] == NULL)		/* key is not mapped */
	return NULL;

    /* build `command pathname:lnum'*/
    if (IS_KEY_FLAG(keyline, key))
    {
	gdb_cat(&res, keymap[key]);
	gdb_cat(&res, " \"");
	gdb_cat(&res, keyinfo->pathname);
	gdb_cat(&res, ":");
	gdb_cat(&res, keyinfo->lnum);
	gdb_cat(&res, "\"");
	return res;
    }
    /* build `command text'*/
    else if (IS_KEY_FLAG(keytext, key))
    {
	if (keyinfo->text != NULL)
	{
	    gdb_cat(&res, keymap[key]);
	    gdb_cat(&res, " ");
	    gdb_cat(&res, keyinfo->text);
	    return res;
	}
    }
    else
	return clewn_strsave(keymap[key]);

    return NULL;
}

/*
 * Search for a line in file fname that matches regexp PAT_ADD
 * and contains needle. Abort when addresses are greater than needle.
 * Return the line number or -1 when error.
 */
    linenr_T
searchfor(fname, needle)
    char *fname;
    char *needle;
{
    struct obstack obs;	    /* use an obstack for speed */
    char *add;
    FILE *fd;
    linenr_T lnum;
    int compare;


    if (needle != NULL && fname != NULL && (fd = fopen(fname, "r")) != NULL)
    {
	lnum = 0;
	(void)obstack_init(&obs);

	while (fgets(clewn_buf, MAX_BUFFSIZE, fd) != NULL)
	{
	    lnum++;
	    if ((add = gdb_regexec(clewn_buf, PAT_ADD, 1, &obs)) != NULL)
	    {
		if ((compare = strcmp(add, needle)) == 0)
		{
		    obstack_free(&obs, NULL);
		    fclose(fd);
		    return lnum;
		}
		else if (compare > 0)	/* addresses are greater, abort */
		{
		    obstack_free(&obs, NULL);
		    fclose(fd);
		    return -1;
		}
	    }
	}
	obstack_free(&obs, NULL);
	fclose(fd);
    }
    return -1;
}

/* Show the text in a balloon. */
    void
gdb_showBalloon(text, obs)
    char * text;
    struct obstack *obs;
{
    cnb_showBalloon(text, FALSE, obs);
}

/* Show the status in a ballon. */
    void
gdb_status(this, status, obs)
    gdb_T *this;
    char *status;	/* gdb status */
    struct obstack *obs;
{
    char *p, *q;

    if (showBalloon && this != NULL && GDB_STATE(this, GS_UP))
    {
	obstack_strcat(obs, "gdb ");
	if (this->sfile != NULL)
	{
	    obstack_strcat(obs, "- ");
	    /* get the tail of this pathname */
	    for (p = q = this->sfile; *q; q++)
		if (*q == '/')
		    p = q + 1;
	    OBSTACK_STRCAT(obs, p);
	}

	obstack_strcat(obs, " [");
	OBSTACK_STRCAT(obs, status);
	obstack_strcat0(obs, "]");
	p = (char *)obstack_finish(obs);
	cnb_showBalloon(p, TRUE, obs);
    }
}

/* Display a cmd line busy msg */
    void
gdb_msg_busy(str)
    char *str;
{
#define BUSY_LINE_SIZE	82
    static char *prop[] = { "/", "-", "\\", "|" };
    static char busy[BUSY_LINE_SIZE];
    static int cnt;
    char *p;

    /* set busy string */
    if (str != NULL && strcmp(str, "FIN") == 0)
    {
	for (p = busy; *p && p < busy + BUSY_LINE_SIZE - 6; p++)
	    *p = ' ';
	*p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = ' '; *p++ = '\r';
	*p = NUL;
	busy[0] = '\r';
	fprintf(stderr, busy);
	fflush(stderr);
	rl_forced_update_display();
    }
    else if (str != NULL)
    {
	busy[0] = '\r';
	strncpy(busy + 1, str, BUSY_LINE_SIZE - 2);
	busy[BUSY_LINE_SIZE - 1] = NUL;
    }
    else
    {
	fprintf(stderr, busy);
	fprintf(stderr, " [");
	fprintf(stderr, prop[(++cnt % 4)]);
	fprintf(stderr, "]");
	fflush(stderr);
    }
}

