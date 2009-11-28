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
 * $Id: gdb_lvl3.c 230 2009-11-28 16:50:31Z xavier $
 */

# ifdef HAVE_CLEWN
#  include <config.h>
#  include "obstack.h"
#  include "clewn.h"
static int got_int;	/* not used with Clewn */
# else
#  include "vim.h"
#  include "clewn/obstack.h"
# endif

#if defined(FEAT_GDB) || defined(HAVE_CLEWN)

# include "gdb.h"
# include "misc.h"

# if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)

static char gdb_buf[MAX_BUFFSIZE];  /* general purpose buffer */

typedef struct
{
    int id;	    /* annoted identifier */
    char_u *str;    /* GDB annotation */
} annotation_T;

static annotation_T annotations[] = {
    {ANO_PREPROMPT,		(char_u *)"pre-prompt"},
    {ANO_PROMPT,		(char_u *)"prompt"},
    {ANO_POSTPROMPT,		(char_u *)"post-prompt"},
    {ANO_PRECMDS,		(char_u *)"pre-commands"},
    {ANO_CMDS,			(char_u *)"commands"},
    {ANO_PREOVERLOAD,		(char_u *)"pre-overload-choice"},
    {ANO_OVERLOAD,		(char_u *)"overload-choice"},
    {ANO_PREQUERY,		(char_u *)"pre-query"},
    {ANO_QUERY,			(char_u *)"query"},
    {ANO_PREPMT_FORMORE,	(char_u *)"pre-prompt-for-continue"},
    {ANO_PMT_FORMORE,		(char_u *)"prompt-for-continue"},
    {ANO_POSTPMT_FORMORE,	(char_u *)"post-prompt-for-continue"},
    {ANO_QUIT,			(char_u *)"quit"},
    {ANO_ERROR_BEG,		(char_u *)"error-begin"},
    {ANO_FRAME_INVALID,		(char_u *)"frames-invalid"},
    {ANO_BP_INVALID,		(char_u *)"breakpoints-invalid"},
    {ANO_STARTING,		(char_u *)"starting"},
    {ANO_STOPPED,		(char_u *)"stopped"},
    {ANO_EXITED,		(char_u *)"exited"},
    {ANO_SIGNALLED,		(char_u *)"signalled"},
    {ANO_BREAKPOINT,		(char_u *)"breakpoint"},
#  ifdef GDB_LVL2_SUPPORT
    {ANO_SOURCE,		(char_u *)"source"},
    {ANO_FRAME_BEGIN,		(char_u *)"frame-begin"},
    {ANO_FRAME_END,		(char_u *)"frame-end"},
    {ANO_BP_HEADER,		(char_u *)"breakpoints-headers"},
    {ANO_BP_TABLE,		(char_u *)"breakpoints-table"},
    {ANO_BP_RECORD,		(char_u *)"record"},
    {ANO_BP_FIELD0,		(char_u *)"field 0"},
    {ANO_BP_FIELD1,		(char_u *)"field 1"},
    {ANO_BP_FIELD2,		(char_u *)"field 2"},
    {ANO_BP_FIELD3,		(char_u *)"field 3"},
    {ANO_BP_FIELD4,		(char_u *)"field 4"},
    {ANO_BP_FIELD5,		(char_u *)"field 5"},
    {ANO_BP_FIELD6,		(char_u *)"field 6"},
    {ANO_BP_FIELD7,		(char_u *)"field 7"},
    {ANO_BP_FIELD8,		(char_u *)"field 8"},
    {ANO_BP_FIELD9,		(char_u *)"field 9"},
    {ANO_BP_END,		(char_u *)"breakpoints-table-end"},
    {ANO_DISP_BEG,		(char_u *)"display-begin"},
    {ANO_DISP_NUMEND,		(char_u *)"display-number-end"},
    {ANO_DISP_FMT,		(char_u *)"display-format"},
    {ANO_DISP_EXP,		(char_u *)"display-expression"},
    {ANO_DISP_EXPEND,		(char_u *)"display-expression-end"},
    {ANO_DISP_VALUE,		(char_u *)"display-value"},
    {ANO_DISP_END,		(char_u *)"display-end"},
    {ANO_FIELD_BEG,		(char_u *)"field-begin"},
    {ANO_FIELD_NAMEND,		(char_u *)"field-name-end"},
    {ANO_FIELD_VALUE,		(char_u *)"field-value"},
    {ANO_FIELD_END,		(char_u *)"field-end"},
    {ANO_ARRAY_BEG,		(char_u *)"array-section-begin"},
    {ANO_ARRAY_ELT,		(char_u *)"elt"},
    {ANO_ARRAY_ELTREP,		(char_u *)"elt-rep"},
    {ANO_ARRAY_ELTEND,		(char_u *)"elt-rep-end"},
    {ANO_ARRAY_END,		(char_u *)"array-section-end"},
#  endif
    {0,				NULL}
};

/* User interface */
static char_u *process_cmd __ARGS((gdb_T *, char_u *));

/* Vim low level hook */
/* Not allowed after ^O in INS REP mode or from the input-line window */
static char_u *process_annotation __ARGS((gdb_T *, char_u *, struct obstack *));
static char_u * process_completion __ARGS((cli_cmd_T *, char_u *, struct obstack *));
static char_u * eol_choices __ARGS((cli_cmd_T *, struct obstack *));

#  ifdef GDB_LVL3_SUPPORT
/* Gdb process mgmt */
static void clear_gdb_T __ARGS((gdb_T *));

/* Out Of Band */
static char *get_lastbp __ARGS((gdb_T *, int, char_u *, struct obstack *));
static char *get_bp __ARGS((gdb_T *, int, char_u *, struct obstack *));
static void process_record __ARGS((gdb_T *, char_u *, struct obstack *));
static char *varobj_update __ARGS((gdb_T *, int, char_u *, struct obstack *));
static char *varobj_complete __ARGS((gdb_T *, struct obstack *));
static void varobj_hilite __ARGS((gdb_T *, varobj_T *, int, struct obstack *));
static void varobj_replace __ARGS((gdb_T *, varobj_T *, char_u *, struct obstack *));
static void remove_object __ARGS((gdb_T *, varobj_T *));
#  endif

/* Utilities */
static char_u * parse_note __ARGS((gdb_T *, char_u *));
static void print_version __ARGS((gdb_T *, char_u *));
#  ifndef FEAT_GDB
static int get_note __ARGS((gdb_T *, char_u *));
#  endif

#  ifdef GDB_LVL3_SUPPORT
/* The function ordering in this array is important as some of
 * these functions must be invoked in the right order */
static oobfunc_T oobfunc[] = {
    {gdb_get_pc},
    {gdb_get_frame},
    {gdb_get_sourcedir},
#ifndef FEAT_GDB
    {gdb_source_project},
    {gdb_get_pwd},
    {gdb_get_args},
#endif
    {gdb_source_cur},
    {gdb_source_list},
    {gdb_get_sfile},
    {gdb_info_frame},
    {gdb_stack_frame},	    /* after gdb_info_frame */
    {get_lastbp},	    /* after gdb_get_frame */
    {gdb_get_asmfunc},	    /* after get_lastbp */
    {gdb_get_asmfunc_hack}, /* after gdb_get_asmfunc */
    {gdb_get_asm},	    /* after gdb_get_asmfunc */
    {get_bp},		    /* after gdb_get_asm and get_lastbp */
    {varobj_update},
    {NULL}
};
#  endif

/** Send a cmd to gdb */
    void
gdb_docmd_cli(this, cmd)
    gdb_T *this;
    char_u  *cmd;	/* gdb cmd */
{
#  ifdef GDB_LVL3_SUPPORT
    char_u *expression = NULL;
    varobj_T *varobj;
#  endif
    char_u *res;
    char_u *ptr;
    char_u *last;
    int len;

    /* make a copy so we can mess with it */
    if (cmd == NULL)
    {
	this->cmd_type = CMD_ANY;
	goto empty_cmd;
    }
    cmd = (char_u *)clewn_strsave((char *)cmd);

    /* remove illegal characters */
    for (ptr = last = cmd; *last != NUL; last++)
	if (*last == NL
		|| *last == TAB
		|| *last == KEY_INTERUPT 
		|| !iscntrl((int)(*last)))
	    *ptr++ = *last;
    *ptr = NUL;

    /* remove backslash at last position */
    if (ptr != cmd && *(ptr - 1) == '\\')
	*(ptr - 1) = NUL;

    if (this->cli_cmd.state == CS_QUERY || this->note == ANO_QUERY)
    {
	if (*cmd == NUL)
	    goto empty_cmd;
	goto send;
    }

    /* now we can forget last cmd */
    this->cmd_type = CMD_ANY;

    /* an interrupt */
    if (STRCHR(cmd, KEY_INTERUPT) != NULL)
	goto send;

    /* a TAB */
    if (STRCHR(cmd, TAB) != NULL)
    {
	gdb_cmd_type(this, cmd);	/* get cmd type */

	/* handle completion for all non-gdb commands */
	if (this->cmd_type == CMD_RESTART) {
	    gdb_setwinput((gdb_handle_T *)this, (char_u *)"cl_restart ");
	    xfree(cmd);
	    this->oob.state &= ~OS_CMD;
	    return;
	}
	else if (this->cmd_type == CMD_CREATEVAR)
	{
	    gdb_setwinput((gdb_handle_T *)this, (char_u *)"createvar ");
	    xfree(cmd);
	    this->oob.state &= ~OS_CMD;
	    return;
	}
	goto send;
    }

    if ((res = gdb_regexec(cmd, PAT_CHG_ANNO, 0, NULL)) != NULL)
    {
	EMSG(_("Sorry, cannot change annotation level"));
	xfree(res);
	goto empty_cmd;
    }

#  ifndef FEAT_GDB  /* Clewn follows GDB behavior with empty commands */
    if (*cmd == NUL)
    {
	gdb_send_cmd(this, (char_u *)"\n");
	xfree(cmd);
	return;
    }
#  endif

    /* process the cmd */
    if ((res = process_cmd(this, cmd)) != NULL)
    {
	xfree(cmd);
	cmd = res;

#  ifdef GDB_LVL3_SUPPORT
	/* a |+gdb| createvar command is processed later in oob varobj_update() */
	if (this->mode == GDB_MODE_LVL3 && this->cmd_type == CMD_CREATEVAR)
	{
#   ifdef FEAT_GDB
	    if (this->var_buf == NULL)
#   else
	    if (this->var_buf <= 0)
#   endif
	    {
		EMSG(_("Variables buffer does not exist anymore: unable to create variable"));
		goto empty_cmd;
	    }

	    if (
#   ifdef FEAT_GDB
		    (expression = gdb_regexec(cmd, PAT_CRVAR_FMT, 2, NULL)) != NULL
#   else
		    (expression = gdb_regexec(cmd, PAT_CRVAR_FMT, 3, NULL)) != NULL
#   endif
		    && *expression != NUL)
	    {
		varobj = (varobj_T *)xcalloc(sizeof(varobj_T));

		/* create a new varobj_T element and link it to the list */
		varobj->state      = VS_INIT;
		varobj->children   = FALSE;
#   ifdef FEAT_GDB
		varobj->format     = gdb_regexec(cmd, PAT_CRVAR_FMT, 1, NULL);
#   else
		varobj->format     = gdb_regexec(cmd, PAT_CRVAR_FMT, 2, NULL);
#   endif
		varobj->expression = expression;
		varobj->next = this->lvl3.varlist;
		this->lvl3.varlist = varobj;

		goto empty_cmd;
	    }

	    xfree(expression);
	    EMSG(_("Unvalid arguments to \"createvar\" command"));
	    goto empty_cmd;
	}
#  endif
    }
    else
	goto empty_cmd;

    /* add a newline to cmd if needed */
    len = STRLEN(cmd);
    if (len == 0 || *(cmd + len - 1) != NL)
    {
	res = NULL;
	gdb_cat(&res, cmd);
	gdb_cat(&res, (char_u *)"\n");
	xfree(cmd);
	cmd = res;
    }
send:
    gdb_send_cmd(this, cmd);
    xfree(cmd);
    return;
empty_cmd:
    gdb_send_cmd(this, (char_u *)" \n");
    xfree(cmd);
    return;
}

/*
 * Process a gdb cmd according to its type.
 * Return an allocated sanitized cmd or NULL if error.
 */
    static char_u *
process_cmd(this, cmd)
    gdb_T *this;
    char_u  *cmd;	/* user's gdb cmd */
{
    char_u *delete  = NULL;
    char_u *range   = NULL;
    char_u *res;
#  ifdef FEAT_GDB
    int i;
    buf_T *buf	    = NULL;
    win_T *oldwin   = curwin;
#  endif

    /* make a copy so we can mess with it */
    if (cmd == NULL || *cmd == NUL)
	return NULL;
    cmd = (char_u *)clewn_strsave((char *)cmd);

    gdb_cmd_type(this, cmd);	/* get cmd type */

#  ifdef GDB_LVL2_SUPPORT
    /* Replace "createvar" command with GDB "display" command */
    if (this->mode == GDB_MODE_LVL2 && this->cmd_type == CMD_CREATEVAR)
    {
#   ifdef FEAT_GDB
	if ((res = gdb_regexec(cmd, PAT_CREATEVAR, 1, NULL)) != NULL)
#   else
	if ((res = gdb_regexec(cmd, PAT_CREATEVAR, 2, NULL)) != NULL)
#   endif
	{
	    /* replace */
	    this->cmd_type = CMD_DISPLAY;
	    FREE(cmd);
	    gdb_cat(&cmd, (char_u *)"display ");
	    gdb_cat(&cmd, res);
	    xfree(res);

	    if (cmd == NULL)
	    {
		this->cmd_type = CMD_ANY;
		return NULL;
	    }
	}
    }
#  endif

#  ifndef FEAT_GDB
    /* Prevent use of a low height size because of "prompt-for-continue" messages.
     * The annotations lines printed by GDB are part of the lines count that
     * is used by GDB for "prompt-for-continue" messages, however we do not
     * display them. */
    if ((res = gdb_regexec(cmd, PAT_HEIGHT, 2, NULL)) != NULL)
    {
	fprintf(stderr, "Cannot change the screen size\n");
	xfree(cmd);
	xfree(res);
	return NULL;
    }
#  endif

    /* Cannot attach to oneself */
#  ifdef FEAT_GDB
    if ((res = gdb_regexec(cmd, PAT_PID, 1, NULL)) != NULL)
#  else
    if ((res = gdb_regexec(cmd, PAT_PID, 2, NULL)) != NULL)
#  endif
    {
	if (getpid() == atoi((char *)res))
	{
	    EMSG(_("I refuse to debug myself!"));
	    xfree(cmd);
	    xfree(res);
	    return NULL;
	}
	xfree(res);
    }

    /* no processing for 'define' type cmds */
    if (this->note == ANO_CMDS || this->note == ANO_OVERLOAD)
	return cmd;

    switch (this->cmd_type)
    {
#  ifdef GDB_LVL2_SUPPORT
	case CMD_DISPLAY:
	    /* A "display" command without argument is equivalent to the
	     * debuggee being stopped */
	    if ((res = gdb_regexec(cmd, PAT_DISPLAY, 0, NULL)) != NULL)
	    {
		xfree(res);
		this->cmd_type = CMD_ANY;
#   ifdef FEAT_GDB
		if (this->var_buf != NULL)
#   else
		if (this->var_buf > 0)
#   endif
		    this->lvl2.varlist.state = DSP_STOPPED;
	    }
	    break;
#  endif

	case CMD_SHELL:
	    EMSG(_("Sorry, cannot spawn a shell"));
	    xfree(cmd);
	    return NULL;

	/* unlite frame on 'detach' */
	case CMD_DETACH:
	    gdb_fr_unlite(this);
	    break;

	/* clear dirty asm buffers */
	case CMD_EXECF:
#  ifdef FEAT_GDB
	    for (i = 0; i < this->pool.max; i++)
	    {
		if ((curbuf = this->pool.buf[i]) == NULL)
		    continue;

		/* clear the buffer */
		gdb_clear_asmbuf(this, curbuf);

		/* asm buffer is displayed */
		if ((curwin = gdb_btowin(curbuf)) != NULL)
		{
		    check_cursor();
		    buf = curbuf;
		}
		curwin = oldwin;
	    }
	    curbuf = curwin->w_buffer;
	    gdb_redraw(buf);	/* redraw only if one asm displayed */
#  else
	    /* unlink all asm buffers */
	    cnb_unlink_asm();
#  endif

#  ifdef GDB_LVL3_SUPPORT
	    /* get source files list */
	    this->lvl3.get_source_list = TRUE;
#  endif
	    break;

	case CMD_SYMF:
#  ifdef GDB_LVL3_SUPPORT
	    /* get source files list */
	    this->lvl3.get_source_list = TRUE;
#  endif
	    break;

	case CMD_UP:
	case CMD_UP_SILENT:
	case CMD_DOWN:
	case CMD_DOWN_SILENT:
	case CMD_FRAME:
	case CMD_SLECT_FRAME:
	    if (this->mode != GDB_MODE_LVL3)
	    {
		this->pool.hilite = TRUE;

		/* this should not be necessary but sometimes we
		 * don't get the ANO_FRAME_INVALID */
		this->bp_state |= BPS_FR_INVALID;
	    }
	    break;

	case CMD_DELETE:
	case CMD_DISABLE:
	    /* keep BPS_FR_INVALID state */
	    this->bp_state &= BPS_FR_INVALID;
	    this->bp_state |= BPS_INVALID;
	    break;

	/* the restart command is mapped to the gdb 'quit' command,
	 * and that will cause gdb to be restarted (the 'quit'
	 * command itself is trapped and
	 * causes a clean termination of clewn and vim) */
	case CMD_RESTART:
	    xfree(cmd);
	    cmd = (char_u *)clewn_strsave("quit");
	    break;

	/* terminate clewn and vim */
	case CMD_QUIT:
	    this->state |= GS_QUITTING;
	    break;

	default:
	    break;
    }
    xfree(range);
    xfree(delete);
    return cmd;
}

/* Send a cmd to gdb */
    void
gdb_send_cmd(this, cmd)
    gdb_T *this;
    char_u  *cmd;	/* gdb cmd */
{
    int do_free = TRUE;		/* TRUE when readline must be freed */
    int offset = 0;
    char_u *res;
    char_u *start;
    int len;
    int l;

    if ( ! GDB_STATE(this, GS_UP))
	return;

    this->intr_sent = FALSE;

    /* make a copy so we can mess with it */
    if (cmd == NULL || (len = STRLEN(cmd)) == 0 )
	return;
    cmd = (char_u *)clewn_strsave((char *)cmd);

    /* paranoia: trim after NL */
    if ((res = STRCHR(cmd, (int)NL)) != NULL)
	*(res + 1) = NUL;

    /* answering to a query: any stuff not 'y[es]' is converted to 'no' */
    if (this->cli_cmd.state == CS_QUERY || this->note == ANO_QUERY)
    {
	len = 1;
	if ((res = gdb_regexec(cmd, PAT_YES, 0, NULL)) != NULL)
	{
	    *cmd = 'y';
	    xfree(res);
	}
	else
	    *cmd = 'n';

#  ifdef FEAT_GDB
	/* CMD_DIR is acted upon when parsing gdb output */
	if (this->cmd_type != CMD_DIR)
	    this->cmd_type = CMD_ANY;
#  endif

	if (this->cli_cmd.state == CS_QUERY)	/* no NL when a completion */
	{
	    this->cli_cmd.state = CS_CHOICE;
	    do_free = FALSE;

	    /* a standalone <Tab> */
	    if (this->cli_cmd.readline == NULL
		    && this->cli_cmd.gdb != NULL && *(this->cli_cmd.gdb) == NUL)
		this->oob.state &= ~OS_CMD;
	}
	else
	{
	    this->cli_cmd.state = CS_START;
	    *(cmd + 1) = NL;
	    len = 2;	/* enough room since cmd length not zero */
	}
	goto write_answer;
    }

    /* an interrupt */
    if (STRCHR(cmd, KEY_INTERUPT) != NULL)
    {
	this->intr_sent = TRUE;
	this->cli_cmd.state = CS_START;
	goto write_answer;
    }

    /* a cmd following a completion response by gdb */
    if (this->cli_cmd.state == CS_DONE && this->cli_cmd.readline != NULL)
    {
	/* readline matches start of cmd: send 'user cmd - readline' */
	if (STRSTR(cmd, this->cli_cmd.readline) == cmd)
	{
	    offset = STRLEN(this->cli_cmd.readline);
	    len -= offset;
	    do_free = FALSE;
	}
	/* no match */
	else
	{
	    /* send KEY_KILL to erase gdb's readline */
	    gdb_buf[0] = KEY_KILL;
	    write(this->fd, gdb_buf, 1);

	    /* discard gdb answer:
	     * '\r' plus the termcap entry 'ce' (clear to end of line) */
	    l = gdb_read(this, (char_u *)gdb_buf, MAX_BUFFSIZE, 1000);

	    /* do what the discarded stuff above was meant to do:
	     * replace last line by getting rid of readline */
	    /* find the rightmost match */
	    res = NULL;
	    for (start = this->line; *start != NUL; start++)
		if ((start = STRSTR(start, this->cli_cmd.readline)) != NULL)
		    res = start;
		else
		    break;

	    if (res != NULL)
	    {
		*res = NUL;
		gdb_write_buf(this, this->line, FALSE);
	    }
	    FREE(this->cli_cmd.echoed);
	}
    }

    this->cli_cmd.state = CS_START;

    /* a completion request */
    if (*(cmd + offset + len - 1) == TAB)
    {
	this->cli_cmd.state = CS_PENDING;

	xfree(this->cli_cmd.gdb);
	this->cli_cmd.gdb = (char_u *)clewn_strsave((char *)(cmd + offset));
	*(this->cli_cmd.gdb + len - 1) = NUL;	/* remove <Tab> */
    }
write_answer:
    if (do_free)
	FREE(this->cli_cmd.readline);
    write(this->fd, (char *)cmd + offset, len);
    xfree(cmd);
    return;
}

#  ifdef FEAT_GDB
#   if defined(MACOS_X) || defined(MACOS_X_UNIX)
/* Strip terminating carriage return from a line */
    static void
strip_cr(line)
    char_u *line;
{
    int len = STRLEN(line);

    if (len != 0 && line[len - 1] == '\r')
	line[len - 1] = NUL;
}
#   endif
#  endif

#  ifndef FEAT_GDB
    /* Handle "prompt-for-continue" annotations.
     * This is not needed by VimGDB as the screen height is very large. */
#   define MANAGE_PRMPT_FORMORE

     /* The fix for writing through readline the concatenation of multiple
      * lines output by gdb, is only correctly implemented for clewn */
#   define FIX_CONCATENATION_WRITE
#  endif
/*
 * Parse gdb output for annotation and completion. Update gdb console.
 * Return TRUE when the user must be prompted by the input-line window.
 */
    int
gdb_parse_output_cli(this)
    gdb_T *this;
{
#  ifdef FIX_CONCATENATION_WRITE
    static int pending_write = FALSE;   /* TRUE, this->line content has not been written yet */
#  endif
    struct obstack obs;	/* use an obstack for temporary allocated memory */
    char_u *start;
    char_u *end;
    char_u *line;
    char_u *res;
    int rc;
    int len;
#  ifdef MANAGE_PRMPT_FORMORE
    int do_newline;
#  endif

    /* read gdb data */
    if (this == NULL || ! GDB_STATE(this, GS_UP)
	    || gdb_read(this, (char_u *)gdb_buf, MAX_BUFFSIZE, 0) <= 0)
	return FALSE;

    (void)obstack_init(&obs);

    /* Process line after line */
    for (start = end = (char_u *)gdb_buf; end != NULL; start = end)
    {
	/* Get next line */
	if ((end = STRCHR(start, NL)) != NULL)
	    *end++ = NUL;
	else if (*start == NUL) /* nothing left to read */
	    break;

	/* concatenate with incomplete annotation */
	if (this->annotation != NULL)
	{
	    OBSTACK_STRCAT(&obs, this->annotation);
	    OBSTACK_STRCAT0(&obs, start);
	    line = (char_u *)obstack_finish(&obs);
	}
	else
	    line = obstack_strsave(&obs, start);

#  ifdef FEAT_GDB
#   if defined(MACOS_X) || defined(MACOS_X_UNIX)
	strip_cr(line);
#   endif
#  endif

	/* With GDB 6.0, completion giving as a result a long list of items causes
	 * a "--More--" prompt to be issued after what GDB (or readline) considers
	 * the height of the terminal (unfortunately not using the GDB height
	 * settings). In case we could not size the terminal to a very big height
	 * with an ioctl call in exec_gdb(), we must answer to the prompt. */
	if (this->height == 0				/* height not set */
		&& this->cli_cmd.state == CS_CHOICE	/* a completion list */
		&& end == NULL
		&& STRCMP(line, "--More--") == 0)	/* standalone prompt */
	{
	    write(this->fd, " ", 1);	/* prompt for more */
	    this->annoted = FALSE;
	    break;
	}

#  ifdef FEAT_GDB   /* Clewn does not use hiliting */
	/* some fuzzy gdb annotation corner: rewrite '(gdb) Quit' so that
	 * it gets highlited (must be done before the newline stuff) */
	if (parse_note(this, line) != NULL && this->note == ANO_QUIT)
	    gdb_write_buf(this, this->line, FALSE);
#  endif

	/* two consecutive NL: start a new line except when this annotation
	 * just follows another annotation
	 * IS_ANNOTATION() does not handle a standalone '\032' */
	if (this->newline && (*line != '\032' || !this->annoted))
	{
#  ifdef MANAGE_PRMPT_FORMORE
	    do_newline = TRUE;

	    /* do not start a new line when parsing annotations and
	     *	. the next chunk is a "pre-prompt-for-continue" and
	     *	  the previous chunk was an annotation
	     *	. or this is a "pre-prompt-for-continue" annotation */
	    if (this->mode == GDB_MODE_LVL2
		    && ! IS_OOBACTIVE(this) && (this->state & GS_ANO))
	    {
		if (end != NULL && get_note(this, end) == ANO_PREPMT_FORMORE
			&& this->prev_note != ANO_NONE)
		    do_newline = FALSE;
		if (line != NULL && get_note(this, line) == ANO_PREPMT_FORMORE)
		    do_newline = FALSE;
	    }

	    if (do_newline)
#  endif
	    {
		if (this->line == NULL && *line != '\032')
		{
		    if (IS_OOBACTIVE(this))
			gdb_oob_receive(this, (char_u *)"", &obs);
		    else
			gdb_write_buf(this, (char_u *)"", TRUE);
		}

#  ifdef FIX_CONCATENATION_WRITE
		if (pending_write)
		    gdb_write_buf(this, this->line, TRUE);
#  endif

		FREE(this->line);	/* start a new line */
		this->note = ANO_NONE;
	    }
	}
	this->newline = FALSE;

	/* Parse for annotations */
	/* we cannot parse "\n\032\032annotation\n" directly
	 * as any stuff may be split across two buff read;
	 * must concatenate line when interleaved with annotations */
	if (*line == NUL)
	{
	    /* ignore echoed NL after ANO_PMT_FORMORE msg */
	    if (this->note != ANO_PMT_FORMORE)
		this->newline = TRUE;

#  ifdef MANAGE_PRMPT_FORMORE
	    /* start a new line */
	    if (this->mode == GDB_MODE_LVL2 && this->prev_note == ANO_POSTPMT_FORMORE)
	    {
		FREE(this->line);
		this->note = ANO_NONE;
	    }
#  endif
	}
	/* a complete annotation */
	else if (IS_ANNOTATION(line) && end != NULL)
	{
	    /* remember last annotation type */
	    if (this->note != ANO_PREPMT_FORMORE
		    && this->note != ANO_PMT_FORMORE
		    && this->note != ANO_POSTPMT_FORMORE)
		this->valid_note = this->note;

	    res = parse_note(this, line); /* assert != NULL */

	    if ((res = process_annotation(this, res, &obs)) != NULL)
	    {
		gdb_setwinput((gdb_handle_T *)this, res);
		xfree(res);

		/* fake an interrupt: will empty stuff and typeahead
		 * buffers aborting insert mode, pending mappings
		 * and operations */
		got_int = TRUE;
	    }

	    FREE(this->annotation);
	    this->annoted = TRUE;

#  ifdef GDB_LVL2_SUPPORT
	    /* ANO_DISP_END has no content and must be trapped here */
	    if (
#   ifdef FEAT_GDB
		    this->mode == GDB_MODE_LVL2 && this->var_buf != NULL
#   else
		    this->mode == GDB_MODE_LVL2 && this->var_buf > 0
#   endif
		    && this->note == ANO_DISP_END)
	    {
		gdb_process_display(this, (char_u *)"", &obs);
	    }
#  endif

#  ifdef MANAGE_PRMPT_FORMORE
	    /* start a new line when end of annotation list or table */
	    if (this->mode == GDB_MODE_LVL2
		    && this->prev_note == ANO_POSTPMT_FORMORE
		    && (this->note == ANO_BP_TABLE
			|| this->note == ANO_BP_RECORD
			|| this->note == ANO_BP_END
			|| this->note == ANO_FRAME_END
			|| this->note == ANO_DISP_END))
	    {
		FREE(this->line);	/* start a new line */
		this->note = ANO_NONE;
	    }
#  endif

	    /* store current annotation type */
	    this->prev_note = this->note;

	    /* restore annotation type */
	    if (this->note == ANO_POSTPMT_FORMORE)
		this->note = this->valid_note;
	}
	/* a partial annotation */
	else if (*line == '\032'
		&& (*(line + 1) == NUL || *(line + 1) == '\032')
		&& end == NULL)
	{
	    xfree(this->annotation);
	    this->annotation = (char_u *)clewn_strsave((char *)line);
	    break;
	}
	else
	{
#  ifdef MANAGE_PRMPT_FORMORE
	    /* The breakpoint table:
	     * start a new line when line starts with <TAB>.*/
	    if (this->mode == GDB_MODE_LVL2
		    && this->prev_note == ANO_POSTPMT_FORMORE
		    && line != NULL && *line == TAB)
	    {
		FREE(this->line);	/* start a new line */
		this->note = ANO_NONE;
	    }
#  endif

	    if (this->note == ANO_PREPMT_FORMORE)
	    {
#  ifdef MANAGE_PRMPT_FORMORE
		if (! IS_OOBACTIVE(this))
		{
		    /* Starting with GDB 6.0, GDB sends an ANO_PREPMT_FORMORE
		     * whenever ^C have been typed more that gdb_screen_height/2. */
		    if (this->state & GS_ANO || this->intr_sent)
		    {
			write(this->fd, "\n", 1);	/* prompt for more */
		    }
		    else
		    {
			this->oob.state &= ~OS_CMD;	/* enable next command */
			xfree(this->prompt);
			if (line != NULL)
			    this->prompt = (char_u *)clewn_strsave((char *)line);
			else
			    this->prompt = (char_u *)clewn_strsave(
				"---type <return> to continue, or q <return> to quit---");
		    }
		    this->intr_sent = FALSE;
		}
#  endif
		line = NULL;		/* remove ANO_PMT_FORMORE msg */
	    }

#  ifdef GDB_LVL2_SUPPORT
	    /* handle a display annotation content
	     * take care of nested structures and arrays in value content */
#   ifdef FEAT_GDB
	    if (this->mode == GDB_MODE_LVL2 && this->var_buf != NULL)
#   else
	    if (this->mode == GDB_MODE_LVL2 && this->var_buf > 0)
#   endif
	    {
		if ((this->note >= ANO_DISP_BEG && this->note <= ANO_DISP_END)
			|| (this->lvl2.doing_value && this->note >= ANO_FIELD_BEG
			    && this->note <= ANO_FIELD_END)
			|| (this->lvl2.doing_value && this->note >= ANO_ARRAY_BEG
			    && this->note <= ANO_ARRAY_END))
		{
		    gdb_process_display(this, line, &obs);
		    line = NULL;
		}
	    }
#  endif

	    if (IS_OOBACTIVE(this))
	    {
		gdb_oob_receive(this, line, &obs);
	    }
	    else
	    {
		if (this->note == ANO_PROMPT || this->note == ANO_CMDS)
		    gdb_cat(&(this->cli_cmd.echoed), line);

		/* update line cnt before doing cpltn */
		if (this->line == NULL || !this->annoted)
		    this->cli_cmd.cnt++;

		/* Parse for completion */
		if ((res = process_completion( &(this->cli_cmd), line, &obs)) != NULL)
		{
		    gdb_setwinput((gdb_handle_T *)this, res);
		    xfree(res);
		    got_int = TRUE;
		}

		if (line != NULL && (len = STRLEN(line)) != 0
			&& *(line + len - 1) == BELL)
		    *(line + len - 1) = NUL;

		/* concatenate after an annotation and replace, otherwise add */
		if (this->line != NULL && this->annoted)
		{
		    if (this->note == ANO_ERROR_BEG && STRCMP(line, "Quit") == 0)
			this->syntax = TRUE;

		    OBSTACK_STRCAT(&obs, this->line);
		    OBSTACK_STRCAT0(&obs, line);
		    line = (char_u *)obstack_finish(&obs);

#  ifndef FEAT_GDB  /* do not write "(gdb) Quit"  with Clewn (already done) */
		    if (this->note != ANO_ERROR_BEG)
#  endif
		    {
#  ifdef FIX_CONCATENATION_WRITE
			/* write a complete line */
			if (end != NULL)
			{
			    if (pending_write)
				gdb_write_buf(this, line, TRUE);
			    else
				gdb_write_buf(this, line, FALSE);
			}
#  else
			gdb_write_buf(this, line, FALSE);
#  endif
		    }

		}
		else
		{
#  ifdef FEAT_GDB   /* no need to map T_CE when running Clewn */
		    /* a completion list */
		    if (this->height == 0		/* height not set */
			    && this->cli_cmd.state == CS_CHOICE)
		    {
			char_u *ptr;

			/* discard "^\rT_CE\r" which is sent by readline
			 * after answering to a prompt in a completion list
			 * (not needed on systems where TIOCSWINSZ ioctls are
			 * available) */
			if (STRSTR(line, T_CE) == line + 1
				&& STRLEN(line) >= STRLEN(T_CE) + 2)
			{
			    ptr = line + STRLEN(T_CE) + 2;
			    line = obstack_strsave(&obs, ptr);
			}
		    }
		    gdb_write_buf(this, line, TRUE);
#  else
		    /* do not write the query prompt */
		    if (this->cli_cmd.state == CS_QUERY)
			gdb_write_buf(this, (char_u *)"", TRUE);
		    else
		    {
#  ifdef FIX_CONCATENATION_WRITE
			/* write a complete line */
			if (end != NULL)
			{
			    gdb_write_buf(this, line, TRUE);
			}
#  else
			gdb_write_buf(this, line, TRUE);
#  endif
		    }
#  endif
		}

		xfree(this->line);
		this->line = (line != NULL ? (char_u *)clewn_strsave((char *)line) : NULL);
	    }

#  ifdef FIX_CONCATENATION_WRITE
	    pending_write = FALSE;
	    if (end == NULL)	/* incomplete line */
	    {
		pending_write = TRUE;
		this->annoted = TRUE;	/* force concatenation on next read */
		break;
	    }
#  else
	    if (end == NULL)	/* incomplete line */
	    {
		this->annoted = TRUE;	/* force concatenation on next read */
		break;
	    }
#  endif
	    this->annoted = FALSE;
	    this->note = ANO_NONE;
	}
    }	/* for (...) */

    obstack_free(&obs, NULL);

    if (IS_OOBACTIVE(this))
	return FALSE;

#  ifdef FEAT_GDB
    /* redraw gdb console window when displayed */
    gdb_redraw(this->buf);
#  endif

    /* Wait till we get the prompt to send the first cmd */
    if (this->firstcmd != NULL && this->note == ANO_PROMPT)
    {
	gdb_docmd((gdb_handle_T *)this, this->firstcmd);
	FREE(this->firstcmd);
    }

    if ((rc = gdb_iswinput((gdb_handle_T *)this)) == TRUE)
	this->oob.state &= ~OS_CMD;
    return rc;
}

/*
 * Process an annotation.
 * Return cmd to prompt the user with or NULL if none.
 */
    static char_u *
process_annotation(this, str, obs)
    gdb_T *this;
    char_u *str;	/* annotation's content */
    struct obstack *obs;
{
    int s_a = (this->state & GS_ALLOWED);
    char_u *file = NULL;
    char_u *line = NULL;
    linenr_T linenumber;
    char_u *cpn;
    int bp_number;
    bpinfo_T *r;

    this->syntax = FALSE;
    this->parser = PS_ANY;

    switch (this->note)
    {
	/* Output end: run directory function */
	case ANO_PREPROMPT:
	    this->syntax = TRUE;
	    this->parser = PS_PREPROMPT;
	    this->cmd_type = CMD_ANY;
	    this->state &= ~GS_ANO;
	    break;

	/* Send out of band cmd */
	case ANO_PROMPT:
	    this->syntax = TRUE;
	    this->parser = PS_PROMPT;
	    this->cli_cmd.cnt = 1;
	    FREE(this->cli_cmd.echoed);
#  ifndef FEAT_GDB	/* store the prompt */
	    if (! IS_OOBACTIVE(this) || (this->oob.state & OS_INTR))
	    {
		xfree(this->prompt);
		if (this->line != NULL)
		    this->prompt = (char_u *)clewn_strsave((char *)this->line);
		else
		    this->prompt = (char_u *)clewn_strsave("(gdb)   ");
	    }
#  endif
	    if (this->cli_cmd.state == CS_START) {
		gdb_oob_send(this, obs);

		/* reset the standard oobfunc table after invocation
		 * of a one function oobfunc array */
		this->oobfunc = this->std_oobfunc;
	    }
	    break;

	case ANO_CMDS:
	    this->syntax = TRUE;
	    this->cli_cmd.cnt = 1;
	    this->oob.state &= ~OS_CMD;
	    FREE(this->cli_cmd.echoed);
#  ifndef FEAT_GDB	/* store the prompt */
	    if (! IS_OOBACTIVE(this) || (this->oob.state & OS_INTR))
	    {
		xfree(this->prompt);
		if (this->line != NULL)
		    this->prompt = (char_u *)clewn_strsave((char *)this->line);
		else
		    this->prompt = (char_u *)clewn_strsave(">  ");
	    }
#  endif

	    /* should be in process_completion, but bug in gdb
	     * annotations where 'pre-commands' is missing in completions */
	    if (this->cli_cmd.state == CS_CHOICE
		    && (cpn = eol_choices(&(this->cli_cmd), obs)) != NULL)
		return cpn;

	    /* a lone <Tab> does not prompt the user except in define mode */
	    if ((this->cli_cmd.state != CS_PENDING
			&& this->cli_cmd.state != CS_CHOICE)
		    || (this->cli_cmd.readline == NULL && this->cli_cmd.gdb != NULL
			    && *(this->cli_cmd.gdb) == NUL))
		return (char_u *)clewn_strsave("");
	    break;

	case ANO_OVERLOAD:
	case ANO_QUERY:
	    this->oob.state &= ~OS_CMD;
#  ifndef FEAT_GDB	/* store the prompt */
	    if (! IS_OOBACTIVE(this) || (this->oob.state & OS_INTR))
	    {
		xfree(this->prompt);
		if (this->line != NULL)
		    this->prompt = (char_u *)clewn_strsave((char *)this->line);
		else
		    this->prompt = (char_u *)clewn_strsave(">   ");
	    }
#  endif
	    return (char_u *)clewn_strsave("");
	
	case ANO_PMT_FORMORE:
#  ifndef FEAT_GDB	/* handle ANO_PMT_FORMORE when IS_OOBACTIVE */
	    if (IS_OOBACTIVE(this))
#  endif
	    {
		if (this->oob.state & OS_INTR)
		{
		    clewn_beep();
		    gdb_send_cmd(this, (char_u *)"q\n");	/* abort */
		}
		else
		    gdb_send_cmd(this, (char_u *)"\n");	/* get more lines */
	    }
	    break;

	case ANO_QUIT:
	    this->syntax = TRUE;
	    this->oob.state |= OS_QUIT;
	    break;

	case ANO_ERROR_BEG:
	    this->pool.hilite = FALSE;
#  ifdef GDB_LVL2_SUPPORT
	    /* a display expression parse error */
	    this->lvl2.doing_value = FALSE;
	    FREE(this->lvl2.dentry.num);
	    FREE(this->lvl2.dentry.expression);
	    FREE(this->lvl2.dentry.value);
#  endif
#  ifdef GDB_LVL3_SUPPORT
	    if (this->lvl3.varitem != NULL)
		this->lvl3.varitem->state |= VS_ERROR;
#  endif
	    break;

	case ANO_STARTING:
	    this->frame_curlvl = -1;
	    this->frame_lnum = (linenr_T) -1;
	    FREE(this->frame_fname);

	    FREE(this->frame_pc);
	    this->state &= ~GS_STOPPED;
	    gdb_status(this, (char_u *)"running...", obs);

	    if (this->mode == GDB_MODE_LVL3)
	    {
		this->pool.hilite = FALSE;
		if (p_asm != 0 && this->cmd_type == CMD_STEPI)  /* asm frame highlite: when stepi nexti */
		    this->pool.hilite = TRUE;
	    }
	    else
	    {
		this->pool.hilite = TRUE;
	    }
	case ANO_FRAME_INVALID:
	    gdb_fr_unlite(this);
	    this->bp_state |= BPS_FR_INVALID;
	    break;

	case ANO_BP_INVALID:
	    /* keep BPS_FR_INVALID state */
	    this->bp_state &= BPS_FR_INVALID;
	    this->bp_state |= BPS_INVALID;

	    /* note the fact that the user is setting a 
	     * breakpoint */
	    if (p_asm != 0 && this->cmd_type == CMD_BREAK)
		this->bp_state |= BPS_BP_SET;
	    break;

	case ANO_STOPPED:
	    this->state |= GS_STOPPED;
	    gdb_status(this, (char_u *)"stopped", obs);
#  ifdef GDB_LVL2_SUPPORT
#   ifdef FEAT_GDB
	    if (this->var_buf != NULL)
#   else
	    if (this->var_buf > 0)
#   endif
		this->lvl2.varlist.state = DSP_STOPPED;
#  endif
	    break;

	case ANO_EXITED:
	case ANO_SIGNALLED:
	    FREE(this->frame_pc);

	    this->state |= GS_STOPPED;
	    gdb_status(this, (char_u *)"exited", obs);

#  ifdef FEAT_GDB
	    /* remove phantom highlite */
	    gdb_unlite(PHANTOM_SIGN);
#  endif
	    break;

	/* Breakpoint hit */
	case ANO_BREAKPOINT:
	    /* Look for this breakpoint in bpinfo list */
	    if (str != NULL && (bp_number = atoi((char *)str)) > 0)
	    {
		for (r = this->bpinfo; r != NULL; r = r->next)
		{
		    if (r->id == bp_number)
		    {
			/* Set this->cont TRUE if this breakpoint's 'commands'
			 * includes a 'continue' as last statement */
			if (r->cont)
			    this->cont = TRUE;

#  ifdef BP_INVALID_ANO_MISSING
			/* Trigger get_bp(): the breakpoint we hit is
			 * 'enable once' */
			if (r->enabled && ! r->disposition)
			{
			    /* keep BPS_FR_INVALID state */
			    this->bp_state &= BPS_FR_INVALID;
			    this->bp_state |= BPS_INVALID | BPS_BP_HIT;
			}
#  endif
			break;
		    }
		}

		/* Not found in the list. An asm buffer holding this bp
		 * can possibly be disassembled next by oob get_asm().
		 * Trigger oob get_bp() */
		if (r == NULL && p_asm != 0)
		{
		    /* keep BPS_FR_INVALID state */
		    this->bp_state &= BPS_FR_INVALID;
		    this->bp_state |= BPS_INVALID | BPS_BP_HIT;
		}
	    }
	    break;

#  ifdef GDB_LVL2_SUPPORT
	case ANO_SOURCE:
	    /* gdb stats the file when doing ANO_SOURCE, so we can't rely on
	     * that annotation for remote debugging with clewn when the source
	     * files are on the host and not on the target
	     * => use ANO_BREAKPOINT instead in level 3
	     */

	    if (! IS_OOBACTIVE(this) && this->mode == GDB_MODE_LVL2)
	    {
		/* Uncomment next line to print source text in gdb buffer */
		/* gdb_write_buf(this, str, TRUE); */

#   ifdef FEAT_GDB
		gdb_popup_console(this);
#   endif

		this->pool.hilite = FALSE;

		/* asm frame highlite: when stepi nexti */
		if (p_asm != 0 && this->cmd_type == CMD_STEPI)
		    this->pool.hilite = TRUE;
		else
		{
		    /* prevent recursive calls to parse_output() since breakpoint
		     * or frame highlighting may cause Vim to query the user when
		     * changes have been made in the previous buffer */
		    this->state &= ~GS_ALLOWED;

		    if ((line = gdb_regexec(str, PAT_SOURCE, 2, obs)) != NULL)
		    {
			file = gdb_regexec(str, PAT_SOURCE, 1, obs);
			linenumber = atoi((char *)line);
			gdb_fr_set(this, file, &linenumber, obs);
		    }

		    this->state &= ~GS_ALLOWED;
		    if (s_a)
			this->state |= GS_ALLOWED;
		}
	    }
	    break;

	case ANO_FRAME_BEGIN:
	case ANO_BP_HEADER:
	case ANO_DISP_BEG:
	    this->state |= GS_ANO;
	    break;

	case ANO_FRAME_END:
	case ANO_DISP_END:
	    break;

	/* Get the source for this frame */
	case ANO_BP_RECORD:
	    this->bp_state |= BPS_RECORD;
	    this->bp_state |= BPS_START;
	    break;

	case ANO_BP_FIELD0:
	case ANO_BP_FIELD1:
	case ANO_BP_FIELD2:
	case ANO_BP_FIELD3:
	case ANO_BP_FIELD4:
	case ANO_BP_FIELD5:
	case ANO_BP_FIELD6:
	case ANO_BP_FIELD7:
	case ANO_BP_FIELD8:
	case ANO_BP_FIELD9:
	    this->bp_state = SET_RECORD_IDX(this->bp_state,
			(this->note - ANO_BP_FIELD0) + BI_NUM);
	    break;

	case ANO_BP_END:
	    /* Initialize state but do not forget some of them.
	     * ANO_BP_END can be the one we get when fetching the
	     * table with oob function get_lastbp(): get_bp() still
	     * needs those states */
	    this->bp_state &= (BPS_INVALID | BPS_BP_HIT | BPS_FR_INVALID);
	    break;
#  endif
    }
    return NULL;
}

/*
 * Parse gdb output for completion.
 * Return cmd to prompt the user with or NULL if none.
 */
    static char_u *
process_completion(cmd, line, obs)
    cli_cmd_T *cmd;
    char_u *line;	/* line to parse */
    struct obstack *obs;
{
    char_u *res;
    char_u *start;
    int len;

    if (line == NULL || *line == NUL)
	return NULL;

    switch (cmd->state)
    {
	case CS_PENDING:
	    /* A completion query */
	    if (cmd->cnt == 2 && gdb_regexec(line, PAT_QUERY, 0, obs) != NULL)
	    {
		cmd->state = CS_QUERY;
		return (char_u *)clewn_strsave("");
	    }

	    /* A positive completion */
	    if (cmd->cnt == 1 && cmd->echoed != NULL
		    && (len = (cmd->gdb != NULL ? STRLEN(cmd->gdb) : 0)) > 0)
	    {
		/* find the rightmost match */
		res = NULL;
		for (start = cmd->echoed; *start != NUL; start++)
		    if ((start = STRSTR(start, cmd->gdb)) != NULL)
			res = start;
		    else
			break;

		/* strictly greater and not followed by NL */
		if (res != NULL && *(res + len) != NL
		    && cmd->echoed + STRLEN(cmd->echoed) > res + len)
		{
		    if (*(res + len) == BELL)
		    {
			clewn_beep();
			*(res + len) = NUL;
		    }
		    xfree(cmd->readline);
		    cmd->readline = (char_u *)clewn_strsave((char *)cmd->echoed);
		    cmd->state = CS_DONE;
		    return (char_u *)clewn_strsave((char *)cmd->readline);
		}
	    }

	    /* A BELL: first character */
	    if (cmd->cnt == 1 && *line == BELL)
	    {
		*line = NUL;
		clewn_beep();
		cmd->state = CS_DONE;
		return (cmd->readline != NULL ?
			(char_u *)clewn_strsave((char *)cmd->readline) : NULL);
	    }

	    if (cmd->cnt > 1)
		cmd->state = CS_CHOICE;
	    break;

	case CS_CHOICE:
	    /* The end of a list of completion choices */
	    if (cmd->cnt == 1 && (res = eol_choices(cmd, obs)) != NULL)
		return res;
	    break;
    }
    return NULL;
}

/*
 * The end of a list of completion choices.
 * Return cmd to prompt the user with.
 */
    static char_u *
eol_choices(cmd, obs)
    cli_cmd_T *cmd;
    struct obstack *obs;
{
    char_u *new;

    OBSTACK_STRCAT(obs, cmd->readline);
    OBSTACK_STRCAT0(obs, cmd->gdb);
    new = (char_u *)obstack_finish(obs);

    /* a match with the new readline */
    if (cmd->echoed != NULL && STRSTR(cmd->echoed, new) == cmd->echoed)
    {
	xfree(cmd->readline);
	cmd->readline = (char_u *)clewn_strsave((char *)cmd->echoed);
	cmd->state = CS_DONE;
	return (char_u *)clewn_strsave((char *)cmd->echoed);
    }
    return NULL;
}

#  ifdef GDB_LVL3_SUPPORT
/* Initialize the gdb_T structure lvl3 component that lvl3 is responsible for */
    static void
clear_gdb_T(this)
    gdb_T *this;
{
    varobj_T *item, *next;

    if (this != NULL)
    {
	FREE(this->lvl3.result);

	this->lvl3.get_source_list = TRUE;
	FREE(this->lvl3.source_cur);
	FREE(this->lvl3.source_list);

	for (item = this->lvl3.varlist; item != NULL; item = next)
	{
	    next = item->next;
	    xfree(item->name);
	    xfree(item->format);
	    xfree(item->expression);
	    xfree(item);
	}
	this->lvl3.varlist = NULL;
	this->lvl3.varitem = NULL;
    }
}

/* Initialize lvl3 function pointers */
    void
gdb_lvl3_init(this)
    gdb_T *this;
{
    this->mode = GDB_MODE_LVL3;
    this->oobfunc = oobfunc;
    this->std_oobfunc = oobfunc;
    this->parse_output = gdb_parse_output_cli;
    this->gdb_docmd = gdb_docmd_cli;
    this->var_delete = NULL;
    this->clear_gdb_T = clear_gdb_T;
}
#  endif /* GDB_LVL3_SUPPORT */

#  ifdef FEAT_GDB
#   define SG_INTRO_2	"Vim |+gdb| level 2 mode\n\n"
#   define SG_INTRO_3	"Vim |+gdb| level 3 mode\n\n"
#  else
#   define SG_INTRO_2	"...             \
                        \n... Clewn running GDB in level 2 mode\n...\n"
#   define SG_INTRO_3	"...             \
                        \n... Clewn %s running GDB in level 3 mode\n...\n"
#  endif
#  define SG_INTERP	"server interpreter-exec mi -gdb-version\n"
#  define SG_LVL_2	"server set annotate 2\n"
#  define SG_LVL_3	"server set annotate 3\n"
#  define SG_HEIGHT	"server set height "
#  define SG_WIDTH	"server set width 0\n "
#  define SG_EDITING	"server set editing on\n"
#  define SG_VERSION	"server show version\n"

/* SG_TIMEOUT must be quite big as we may have Clewn, Vim and GDB all
 * starting at the same time. Unless things go pretty bad in the parsing, we
 * always find the needle, so the user never wait SG_TIMEOUT. */
#  define SG_TIMEOUT 120000 /* msecs for when the needle cannot be found */
/*
 * Initialize gdb CLI (command line interface)
 * return OK when sucess, FAIL otherwise
 */
    int
gdb_setup_cli(this)
    gdb_T *this;
{
    char *err     = NULL;
    int needle    = FALSE;	/* defines when we can start writing to gdb console */
    int gdb_cnt   = 0;		/* count of received "(gdb)" prompts */
    int lvl3_mode = FALSE;	/* level */
    char_u *buff  = (char_u *)gdb_buf;/* can't add an int to the reference to an array */
    char * tty_name = NULL;
#   ifndef FEAT_GDB
    char tmp[128];
#   endif
    char_u *last;
    char_u *ptr;
    char_u *lpp_lines;
    char_u *line;
    int len;

    this->note = ANO_NONE;

    /* Does this GDB process supports "interpreter-exec" command ?
     * if yes we will use level 3 mode, otherwise level 2 mode */
    write(this->fd, SG_INTERP, strlen(SG_INTERP));

    /* discard gdb output till we find the needle SG_VERSION
     * MAX_BUFFSIZE must be greater than max gdb line length */
    for (last = (char_u *)gdb_buf; buff + MAX_BUFFSIZE - last > 1; )
    {
#   ifdef ONE_CHUNK
        /* emulate systems that read gdb responses in one chunk
         * (for testing purposes) */
        sleep(1);
	if ((len = gdb_read(this, last,
			buff + MAX_BUFFSIZE - last, SG_TIMEOUT)) < 0)
	{
	    err = "Unable to read from GDB pseudo tty";
	    goto fail;
	}
	else if (len == 0)	/* needle not found */
	    break;
	last += len;
        sleep(1);
#   endif

	if ((len = gdb_read(this, last,
			buff + MAX_BUFFSIZE - last, SG_TIMEOUT)) < 0)
	{
	    err = "Unable to read from GDB pseudo tty";
	    goto fail;
	}
	else if (len == 0)	/* needle not found */
	    break;

	last += len;
	ptr = (char_u *)gdb_buf;

	/* First step: determine which level, process line by line */
	do
	{
	    if (gdb_cnt >= 2)
		break;

            if (*ptr == NL)
                ptr++;

	    if (STRSTR(ptr, "^done") == ptr)
		lvl3_mode = TRUE;

	    /* the first prompt is newline terminated (count it only once) */
	    if (STRSTR(ptr, "(gdb) ") == ptr
		    && (gdb_cnt == 1 || STRCHR(ptr, NL) != NULL))
		gdb_cnt++;

	    if (gdb_cnt == 2)
	    {
#  ifdef GDB_LVL3_SUPPORT
		/* set annotation level 3 */
		if (lvl3_mode)
		    write(this->fd, SG_LVL_3, strlen(SG_LVL_3));
		else
#   ifdef GDB_LVL2_SUPPORT
		    write(this->fd, SG_LVL_2, strlen(SG_LVL_2));
#   else
		{
		    err = "This |+gdb| version does not support level 2 mode";
		    goto fail;
		}
#   endif
#  else /* LVL3 */
		lvl3_mode = FALSE;
#   ifdef GDB_LVL2_SUPPORT
		/* set annotation level 2 */
		if (! lvl3_mode)
		    write(this->fd, SG_LVL_2, strlen(SG_LVL_2));
#   endif
#  endif /* LVL3 */

#  ifdef FEAT_GDB   /* Clewn does not need this */
		/* ANO_PMT_FORMORE happen every LPP_LINES and provide us a mean to abort
		 * large output (in get_asm for example or any user cmd) */
		write(this->fd, SG_HEIGHT, strlen(SG_HEIGHT));
		lpp_lines = gdb_itoa(LPP_LINES);
		write(this->fd, (char *)lpp_lines, strlen((char *)lpp_lines));
		write(this->fd, "\n", 1);
		write(this->fd, SG_WIDTH, strlen(SG_WIDTH));
		write(this->fd, SG_EDITING, strlen(SG_EDITING));
#  else
		/* number of lines in a page is unlimited */
		write(this->fd, SG_HEIGHT, strlen(SG_HEIGHT));
		lpp_lines = gdb_itoa(0);
		write(this->fd, lpp_lines, strlen(lpp_lines));
		write(this->fd, "\n", 1);
		write(this->fd, SG_WIDTH, strlen(SG_WIDTH));
		write(this->fd, SG_EDITING, strlen(SG_EDITING));
		write(this->fd, " \n", 2); /* avoid Undefined command: "server".  Try "help". */

		/* `run' commands do input and output on clewn own terminal when possible */
		if (isatty(1) && (tty_name=ttyname(1)) != NULL) {
		    char_u * res = NULL;

		    gdb_cat(&res, (char_u *)"server tty ");
		    gdb_cat(&res, tty_name);
		    gdb_cat(&res, (char_u *)"\n");
		    write(this->fd, res, STRLEN(res));
		    xfree(res);
		}
#  endif
		write(this->fd, SG_VERSION, strlen(SG_VERSION));
	    }
	} while ((ptr = STRCHR(ptr, NL)) != NULL);

	/* Second step: find the needle, process line by line */
	do
	{
	    if (ptr == NULL)
		break;

            if (*ptr == NL)
                ptr++;

	    /* looking for the needle after a prompt */
	    if (this->note == ANO_PROMPT && STRSTR(ptr, SG_VERSION) == ptr)
	    {
		ptr += strlen(SG_VERSION);
		needle = TRUE;
	    }

	    /* discard post-prompt */
	    if (needle && this->note == ANO_POSTPROMPT)
	    {
		/* store a partial last line for later parse_output */
                if ((line = (char_u *)strrchr(gdb_buf, (int)NL)) != NULL
                        && *(line + 1) != NUL)
                {
                    xfree(this->line);
                    this->line = (char_u *)clewn_strsave((char *)(line + 1));
                    this->annoted = TRUE;
                }

		if (lvl3_mode)
#  ifdef GDB_LVL3_SUPPORT
		{
#   ifdef FEAT_GDB
		    gdb_popup_console(this);
#   endif

		    this->syntax = TRUE;	/* force syntax highlite */
#   ifdef FEAT_GDB
		    gdb_write_buf(this, (char_u *)SG_INTRO_3, TRUE);
#   else
		    sprintf(tmp, SG_INTRO_3, this->version);
		    gdb_write_buf(this, tmp, TRUE);
#   endif
		    this->syntax = FALSE;

#   ifdef FEAT_GDB  /* write unconditionally with Clewn */
		    /* write the remaining part and display it */
		    if (*ptr != NUL)
#   endif
                        print_version(this, ptr);

#   ifdef FEAT_GDB
		    /* redraw gdb console window when displayed */
		    gdb_redraw(this->buf);
#   endif

		    gdb_lvl3_init(this);
		}
#  else
		{
		    err = "This |+gdb| version does not support level 3 mode";
		    goto fail;
		}
#  endif
		else
#  ifdef GDB_LVL2_SUPPORT
		{
#   ifdef FEAT_GDB
		    gdb_popup_console(this);
#   endif

		    this->syntax = TRUE;	/* force syntax highlite */
		    gdb_write_buf(this, (char_u *)SG_INTRO_2, TRUE);
		    this->syntax = FALSE;

#   ifdef FEAT_GDB   /* write unconditionally with Clewn */
		    /* write the remaining part and display it */
		    if (*ptr != NUL)
#   endif
                        print_version(this, ptr);

#   ifdef FEAT_GDB
		    /* redraw gdb console window when displayed */
		    gdb_redraw(this->buf);
#   endif

		    gdb_lvl2_init(this);
		}
#  else
		{
		    err = "This |+gdb| version does not support level 2 mode";
		    goto fail;
		}
#  endif
		if (tty_name)
		    fprintf(stderr, "`run' commands do input and output on the terminal %s\n", tty_name);

		return OK;
	    }
	} while ((ptr = parse_note(this, ptr)) != NULL);

	/* left shift buffer to last start of line */
	if ((ptr = (char_u *)strrchr((char *)gdb_buf, (int)NL)) != NULL)
	{
	    len = STRLEN(ptr + 1);
	    clewn_memmove(gdb_buf, ptr + 1, len);
	    last = (char_u *)gdb_buf + len;
	}
    }

#  ifdef GDB_LVL2_SUPPORT
    /* could not find the needle: hope for the best and fall back 
     * to annotation level 2 and send version again */
    write(this->fd, SG_LVL_2, strlen(SG_LVL_2));
    write(this->fd, SG_VERSION, strlen(SG_VERSION));
#   ifdef FEAT_GDB
    gdb_popup_console(this);
#   endif

    this->syntax = TRUE;	/* force syntax highlite */
    gdb_write_buf(this, (char_u *)SG_INTRO_2, TRUE);
    this->syntax = FALSE;

    gdb_lvl2_init(this);
    return OK;
#  else
    err = "This |+gdb| version does not support level 2 mode";
    goto fail;
#  endif
fail:
    gdb_close(this);

    if (err != NULL)
	EMSG(_(err));

    return FAIL;
}

#  ifdef GDB_LVL3_SUPPORT
/* Print a value in a netbeans balloon */
#   define PRINT_VALUE	    "^done,value=\""
    char *
gdb_print_value(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
#   ifdef HAVE_CLEWN
    char_u * res = NULL;
    char_u * ptr;
    char_u * quote;

    switch(state)
    {
	case OOB_CMD:
	    if (this->balloon_txt != NULL) {
		obstack_strcat(obs, "server interpreter-exec mi \"-data-evaluate-expression ");
		OBSTACK_STRCAT(obs, this->balloon_txt);
		obstack_strcat0(obs, "\"\n");

		FREE(this->lvl3.result);
		return (char_u *)obstack_finish(obs);
	    }
	    break;

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.result);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.result);
	    this->lvl3.result = res;
	    break;

	case OOB_COMPLETE:
	    if (this->lvl3.result != NULL
		    && (ptr = STRSTR(this->lvl3.result, PRINT_VALUE)) != NULL
		    && (ptr += strlen(PRINT_VALUE))
		    && (quote = (char_u *)strrchr(ptr, '"')) != NULL)
	    {
		*quote = NUL;
		obstack_strcat(obs, "\" ");
		OBSTACK_STRCAT(obs, this->balloon_txt);
		obstack_strcat(obs, " = ");
		OBSTACK_STRCAT(obs, ptr);
		obstack_strcat0(obs, " \"");

		res = (char_u *)obstack_finish(obs);
		gdb_showBalloon(res, obs);
	    }

	    FREE(this->balloon_txt);
	    FREE(this->lvl3.result);
	    break;
    }
#   endif   /* HAVE_CLEWN*/
    return NULL;
}
#  endif    /* GDB_LVL3_SUPPORT */

/* Get instruction at $pc */
    char *
gdb_get_pc(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *ptr;

    if (obs) {}	    /* keep compiler happy */

    switch(state)
    {
	case OOB_CMD:
	    FREE(this->pc);

	    if (this->state & GS_STOPPED)
	    {
		this->state &= ~GS_STOPPED;
		return "server x/i $pc\n";
	    }
	    break;

	case OOB_COLLECT:
	    if ((this->pc = gdb_regexec(line, PAT_ADD, 1, NULL)) != NULL)
	    {
		/* Replace TAB with a space */
		for (ptr = line; *ptr != NUL; ptr++)
		    if (*ptr == TAB)
			*ptr = ' ';

		gdb_status(this, line, obs);
	    }
	    break;
    }
    return NULL;
}

/* Get frame info */
    char *
gdb_get_frame(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *res = NULL;

    if (obs) {}	    /* keep compiler happy */

    switch(state)
    {
	case OOB_CMD:
	    FREE(this->oob_result);
	    return "server frame\n";

	case OOB_COLLECT:
	    gdb_cat(&res, this->oob_result);
	    gdb_cat(&res, line);
	    xfree(this->oob_result);
	    this->oob_result = res;
	    break;

	case OOB_COMPLETE:
	    xfree(this->frame_pc);
	    if (this->oob_result != NULL
		    && (this->frame_pc = gdb_regexec(this->oob_result, PAT_FRAME, 1, NULL)) != NULL) {
		xfree(this->asm_add);
		this->asm_add = (char_u *)clewn_strsave((char *)this->frame_pc);
	    }
	    else {
		if (this->pc != NULL) {
		    this->frame_pc = (char_u *)clewn_strsave((char *)this->pc);
		    xfree(this->asm_add);
		    this->asm_add = (char_u *)clewn_strsave((char *)this->frame_pc);
		}
	    }

	    FREE(this->oob_result);
	    break;
    }
    return NULL;
}

#  ifdef GDB_LVL3_SUPPORT
/* Stack info frame */
    char *
gdb_info_frame(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *res = NULL;

    if (obs) {}	    /* keep compiler happy */

    switch(state)
    {
	case OOB_CMD:
	    FREE(this->lvl3.result);
	    return "server info frame\n";

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.result);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.result);
	    this->lvl3.result = res;
	    break;

	case OOB_COMPLETE:
	    this->frame_curlvl = -1;
	    if (this->lvl3.result != NULL
		    && (res = gdb_regexec(this->lvl3.result, PAT_INFO_FRAME, 1, NULL)) != NULL) {
		this->frame_curlvl = atoi((char *)res);
		xfree(res);
	    }

	    FREE(this->lvl3.result);
	    break;
    }
    return NULL;
}

#   define SOURCE_FILENAME  "\",file=\""
#   define SOURCE_LINENUM   "\",line=\""
/* Change frame highlight according to new frame level */
    char *
gdb_stack_frame(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u * res = NULL;
    int rc	 = -1;
    char tmp[32];
    char_u * fname;
    char_u * pnum;
    char_u * quote;
    char_u * ptr;
    linenr_T lnum;

    switch(state)
    {
	case OOB_CMD:
	    /* get info about frame_curlvl:
	     * ^done,stack=[frame={level="1",addr="0x080483bb",func="main",file="cltest_main.c",line="12"}] */
	    if (this->frame_curlvl >= 0) {
		FREE(this->lvl3.result);

		sprintf(tmp, "%d %d", this->frame_curlvl, this->frame_curlvl);
		obstack_strcat(obs, "server interpreter-exec mi \"-stack-list-frames ");
		OBSTACK_STRCAT(obs, tmp);
		obstack_strcat0(obs, "\"\n");
		return (char *)obstack_finish(obs);
	    }
	    break;

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.result);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.result);
	    this->lvl3.result = res;
	    break;

	case OOB_COMPLETE:
	    if (this->lvl3.result != NULL
		    && (ptr = STRSTR(this->lvl3.result, SOURCE_FILENAME)) != NULL
		    && (pnum = STRSTR(this->lvl3.result, SOURCE_LINENUM)) != NULL)
	    {
		pnum += strlen(SOURCE_FILENAME);
		lnum = atoi((char *)pnum);

		ptr += strlen(SOURCE_FILENAME);

		if ((quote = STRCHR(ptr, '"')) != NULL) {
		    fname = (char_u *)obstack_copy0(obs, ptr, (quote - ptr));

		    rc = 0;

		    /* set frame sign only if this is a new fname/lnum position */
		    if (this->frame_fname == NULL
			    || STRCMP(this->frame_fname, fname) != 0 || this->frame_lnum != lnum) {
			FREE(this->frame_fname);
			this->frame_fname = (char_u *)clewn_strsave((char *)fname);
			this->frame_lnum = lnum;
			rc = gdb_fr_set(this, fname, &lnum, obs);
		    }
		}
	    }

	    if (rc != 0 && p_asm != 0)
		this->pool.hilite = TRUE;   /* do asm frame highliting */
	    FREE(this->lvl3.result);
	    break;
    }
    return NULL;
}
#  endif /* GDB_LVL3_SUPPORT */

/* Get symbol file name */
    char *
gdb_get_sfile(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *res = NULL;

    switch(state)
    {
	case OOB_CMD:
	    return "server info target\n";

	case OOB_COLLECT:
	    if (this->oob.cnt == 1
		    && (res = gdb_regexec(line, PAT_SFILE, 1, obs)) != NULL
		    && (this->sfile == NULL || STRCMP(res, this->sfile) != 0))
	    {
		gdb_status(this, (char_u *)"new symbols", obs);
		xfree(this->sfile);
		this->sfile = (char_u *)clewn_strsave((char *)res);
	    }
	    break;

	case OOB_COMPLETE:
	    if (this->oob.cnt == 0)
	    {
		gdb_status(this, (char_u *)"empty target", obs);
		FREE(this->sfile);
	    }
	    break;
    }
    return NULL;
}

/* Get GDB source directories */
    char *
gdb_get_sourcedir(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *res = NULL;

    if (obs) {}	    /* keep compiler happy */

    switch(state)
    {
	case OOB_CMD:
	    FREE(this->lvl3.result);
	    return "server show directories\n";

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.result);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.result);
	    this->lvl3.result = res;
	    break;

	case OOB_COMPLETE:
	    if (this->lvl3.result != NULL)
	    {
		xfree(this->directories);
		this->directories = gdb_regexec(this->lvl3.result, PAT_DIR, 1, NULL);
		FREE(this->lvl3.result);
	    }
	    break;
    }
    return NULL;
}

#ifdef GDB_LVL3_SUPPORT
# ifndef FEAT_GDB
/* Source the project file */
    char *
gdb_source_project(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    switch(state)
    {
	case OOB_CMD:
	    if (this->project_file != NULL
		    && cnb_state()
		    && this->project_state == PROJ_SOURCEIT)
	    {
		this->project_state = PROJ_DONE;
		fprintf(stderr, "source %s\n", this->project_file);

		if (this->sfile != NULL)
		    fprintf(stderr, "Warning: symbol table \"%s\" was loaded before sourcing the project file\n", this->sfile);

		obstack_strcat(obs, "server source ");
		OBSTACK_STRCAT(obs, this->project_file);
		obstack_strcat0(obs, "\n");
		return (char_u *)obstack_finish(obs);
	    }
	    break;

	case OOB_COLLECT:
	    if (line != NULL)
		fprintf(stderr, "%s\n", line);
	    break;

	case OOB_COMPLETE:
	    /* force getting the source file list from gdb */
	    this->lvl3.get_source_list = TRUE;

	    /* this is required after 'restart' to display the gdb prompt */
	    fprintf(stderr, this->prompt);
	    break;
    }

    return NULL;
}

#   define CWD_VALUE	"^done,cwd=\""
/* Get current working directory */
    char *
gdb_get_pwd(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u * res = NULL;
    char_u * ptr;
    char_u * quote;

    if (obs) {}	    /* keep compiler happy */

    switch(state)
    {
	case OOB_CMD:
	    if (this->mode != GDB_MODE_LVL2) {
		FREE(this->lvl3.result);
		return "server interpreter-exec mi \"-environment-pwd\"\n";
	    }
	    break;

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.result);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.result);
	    this->lvl3.result = res;
	    break;

	case OOB_COMPLETE:
	    if (this->lvl3.result != NULL
		    && (ptr = STRSTR(this->lvl3.result, CWD_VALUE)) != NULL
		    && (ptr += strlen(CWD_VALUE))
		    && (quote = (char_u *)strrchr(ptr, '"')) != NULL)
	    {
		*quote = NUL;
		gdb_cat(&res, (char_u *)"cd ");
		gdb_cat(&res, ptr);
		gdb_cat(&res, (char_u *)"\n");
		xfree(this->pwd);
		this->pwd = res;
	    }

	    FREE(this->lvl3.result);
	    break;
    }
    return NULL;
}

#   define ARGS_VALUE	"Argument list to give program being debugged when it is started is \""
/* Get args */
    char *
gdb_get_args(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u * res = NULL;
    char_u * ptr;
    char_u * quote;

    if (obs) {}	    /* keep compiler happy */

    switch(state)
    {
	case OOB_CMD:
	    FREE(this->lvl3.result);
	    return "server show args\n";

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.result);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.result);
	    this->lvl3.result = res;
	    break;

	case OOB_COMPLETE:
	    if (this->lvl3.result != NULL
		    && (ptr = STRSTR(this->lvl3.result, ARGS_VALUE)) != NULL
		    && (ptr += strlen(ARGS_VALUE))
		    && (quote = (char_u *)strrchr(ptr, '"')) != NULL)
	    {
		*quote = NUL;
		gdb_cat(&res, "set args ");
		gdb_cat(&res, ptr);
		gdb_cat(&res, "\n");
		xfree(this->args);
		this->args = res;
	    }

	    FREE(this->lvl3.result);
	    break;
    }
    return NULL;
}
# endif

/* Get GDB current source file */
    char *
gdb_source_cur(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *res = NULL;

    if (obs) {}	    /* keep compiler happy */

    switch(state)
    {
	case OOB_CMD:
	    FREE(this->lvl3.source_cur);
	    return "server interpreter-exec mi \"-file-list-exec-source-file\"\n";

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.source_cur);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.source_cur);
	    this->lvl3.source_cur = res;
	    break;

	case OOB_COMPLETE:
	    break;
    }
    return NULL;
}

/* Get GDB current source file */
    char *
gdb_source_list(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *res = NULL;

    if (obs) {}	    /* keep compiler happy */

    switch(state)
    {
	case OOB_CMD:
	    if (this->lvl3.get_source_list || this->lvl3.source_list == NULL) {
		FREE(this->lvl3.source_list);
		this->lvl3.get_source_list = FALSE;
		return "server interpreter-exec mi \"-file-list-exec-source-files\"\n";
	    }
	    break;

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.source_list);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.source_list);
	    this->lvl3.source_list = res;
	    break;

	case OOB_COMPLETE:
	    break;
    }
    return NULL;
}
#endif /* GDB_LVL3_SUPPORT */

#  ifdef GDB_LVL3_SUPPORT
#   define BKPT_RECORD	"bkpt={number=\""
#   define BKPT_ADDR	"\",addr=\""
/*
 * When a breakpoint is being set and asm option is on, get the
 * instruction address of the last breakpoint, the one being set.
 * This address is used in get_asm() to disassemble the function
 * containing this address.
 */
    static char *
get_lastbp(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *res    = NULL;
    int found      = FALSE;
    char_u *record = NULL;
    char_u *ptr;
    char_u *addr;
    bpinfo_T *p;
    int bp_num;

    if (obs) {}	    /* keep compiler happy */

    switch (state)
    {
	case OOB_CMD:
	    /* GDB sends sometimes ANO_BP_INVALID when stepping, even
	     * though no breakpoints have been changed.
	     * Avoid checking for last bp in this case, because this
	     * may cause hiliting a bp in a new buffer when what is
	     * expected is a new frame */
	    if (this->bp_state & BPS_FR_INVALID)
		return NULL;

	    /* A breakpoint is being set in assembly */
	    if (p_asm != 0 && (this->bp_state & BPS_INVALID)
		    && (this->bp_state & BPS_BP_SET))
	    {
		this->bp_state &= ~BPS_BP_SET;
		FREE(this->lvl3.result);

		/* fetch GDB bp table info */
		return "server interpreter-exec mi -break-list\n";
	    }
	    break;

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.result);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.result);
	    this->lvl3.result = res;
	    break;

	case OOB_COMPLETE:
	    if ((ptr = this->lvl3.result) != NULL)
	    {
		/* search for last record in breakpoint table */
		do
		{
		    if ((ptr = STRSTR(ptr, BKPT_RECORD)) != NULL)
		    {
			record = ptr;
			ptr++;
		    }
		}
		while (ptr != NULL);

		/* last record */
		if (record != NULL
			&& (bp_num = atoi((char *)(record + strlen(BKPT_RECORD)))) > 0)
		{
		    /* look it up in bpinfo list */
		    for (p = this->bpinfo; p != NULL; p = p->next)
		    {
			if (bp_num == p->id)
			{
			    found = TRUE;
			    break;
			}
		    }

		    /* not found: set asm_add so that get_asm will
		     * do the disassembling */
		    if (! found && (addr = STRSTR(record, BKPT_ADDR)) != NULL)
		    {
			xfree(this->asm_add);
			this->asm_add =
			    gdb_regexec(addr + strlen(BKPT_ADDR), PAT_ADD, 1, NULL);
		    }
		}

		FREE(this->lvl3.result);
	    }
	    break;
    }
    return NULL;
}
#  endif /* GDB_LVL3_SUPPORT */

/* Get the function name corresponding to asm_add */
    char *
gdb_get_asmfunc(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    switch(state)
    {
	case OOB_CMD:
	    if (this->asm_add != NULL)
	    {
		obstack_strcat(obs, "server info symbol 0x");
		OBSTACK_STRCAT(obs, this->asm_add);
		obstack_strcat0(obs, "\n");
		return (char *)obstack_finish(obs);
	    }
	    FREE(this->asm_func);
	    break;

	case OOB_COLLECT:
	    xfree(this->asm_func);
	    this->asm_func = gdb_regexec(line, PAT_ASM_FUNC, 1, NULL);
	    break;
    }
    return NULL;
}

/*
 * This function is a hack to fix errors occuring in GDB command
 * "info symbol ADDR" that sometimes cannot find the symbol corresponding to
 * a given ADDR, when GDB command "print/a ADDR" can.
 * "print/a ADDR" is used as a last resort because it changes the value history.
 * "output/a ADDR" does not change the value history, but the output of this
 * command misses a newline.
 */
    char *
gdb_get_asmfunc_hack(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    switch(state)
    {
	case OOB_CMD:
	    if (this->asm_add != NULL && this->asm_func == NULL)
	    {
		obstack_strcat(obs, "server print/a 0x");
		OBSTACK_STRCAT(obs, this->asm_add);
		obstack_strcat0(obs, "\n");
		return (char *)obstack_finish(obs);
	    }
	    break;

	case OOB_COLLECT:
	    xfree(this->asm_func);
	    this->asm_func = gdb_regexec(line, PAT_ASM_FUNC_P, 1, NULL);
	    break;
    }
    return NULL;
}

#  define ASM_ABORTED "DISASSEMBLY ABORTED"

/*
 * Disassemble new function containing asm_add and highlight asm_add
 * if this->pool.hilite is TRUE
 */
    char *
gdb_get_asm(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
#  ifdef FEAT_GDB
{
    buf_T *buf = this->pool.buf[this->pool.idx];
    buf_T *oldbuf = curbuf;
    char_u *res = NULL;
    int oldest  = 0;
    int age	= 0;
    linenr_T lnum;
    win_T *win;
    int i;

    if (p_asm == 0)
    {
	FREE(this->asm_add);
	return NULL;
    }

    lnum = (buf != NULL ? BUFLASTL(buf) : 0);

    switch (state)
    {
	case OOB_CMD:
	    if (this->asm_add != NULL && !gdb_as_frset(this, obs))
	    {
		/* pickup least recent buffer */
		for (i = this->pool.max; i > 0; )
		{
		    i--;
		    if (this->pool.age[i] >= age && this->pool.buf[i] != NULL)
		    {
			age = this->pool.age[i];
			oldest = i;
		    }
		}

		/* no buffers in pool */
		if ((buf = this->pool.buf[oldest]) == NULL)
		{
		    this->pool.hilite = FALSE;
		    FREE(this->asm_add);
		    return NULL;
		}
		this->pool.idx = oldest;

		/* clear the buffer */
		gdb_clear_asmbuf(this, buf);

		/* init msg_busy */
		obstack_strcat(obs, "Disassembling 0x");
		OBSTACK_STRCAT0(obs, this->asm_add);
		res = (char_u *)obstack_finish(obs);
		gdb_msg_busy(res);

		/* send cmd */
		obstack_strcat(obs, "server disassemble 0x");
		OBSTACK_STRCAT(obs, this->asm_add);
		obstack_strcat0(obs, "\n");
		return (char *)obstack_finish(obs);
	    }
	    this->pool.hilite = FALSE;
	    break;

	case OOB_COLLECT:
	    if (buf == NULL)
		return NULL;
	    curbuf = buf;

	    /* concatenate with line after an annotation
	     * and replace line, otherwise add */
	    if (this->line != NULL && this->annoted)
	    {
		OBSTACK_STRCAT(obs, this->line);
		OBSTACK_STRCAT0(obs, line);
		line = (char_u *)obstack_finish(obs);
		ml_delete(lnum--, FALSE);
	    }

	    /* first line: remove empty line after the inserted one */
	    if (ml_append(lnum, line, 0, 0) == OK && lnum == 0)
		ml_delete(buf->b_ml.ml_line_count, FALSE);
	    curbuf = oldbuf;

	    if ((this->oob.cnt % 1000) == 0)
		gdb_msg_busy(NULL);

	    xfree(this->line);
	    this->line = (char_u *)clewn_strsave((char *)line);
	    break;

	case OOB_COMPLETE:
	    if (buf == NULL)
		return NULL;
	    curbuf = buf;

	    if (this->oob.state & OS_INTR)
	    {
		if (lnum >= 1 && STRCMP(ASM_ABORTED, ml_get(1)) != 0)
		{
		    /* Clear buffer when interrupted */
		    while (lnum-- > 0)
			ml_delete(buf->b_ml.ml_line_count, FALSE);

		    ml_append(0, (char_u *)ASM_ABORTED, 0, 0);
		    lnum = buf->b_ml.ml_line_count;
		    changed_lines(1, 0, lnum, lnum);
		    curbuf = oldbuf;

		    this->pool.age[this->pool.idx] = ASM_OLD;
		    gdb_edit_file(this, buf, NULL, 1, obs);
		}

		/* write the prompt */
		gdb_write_buf(this, this->line, TRUE);
		gdb_redraw(buf);
	    }
	    else
	    {
		changed_lines(1, 0, lnum, lnum);

		/* set buffer name to function name */
		if (this->asm_func != NULL)
		{
		    OBSTACK_STRCAT(obs, this->asm_func);
		    obstack_strcat0(obs, "-asm");
		    res = (char_u *)obstack_finish(obs);
		    gdb_as_setname(res);
		}
		curbuf = oldbuf;

		/* highlite $asm_add and update window if displayed */
		if (! gdb_as_frset(this, obs) && (win = gdb_btowin(buf)) != NULL)
		{
		    gdb_set_cursor(win, 1);
		    redraw_win_later(win, NOT_VALID);
		}
	    }

	    msg_clr_cmdline();
	    FREE(this->asm_add);
	    this->pool.hilite = FALSE;
	    break;
    }
    return NULL;
}
#  else
{
    char_u *res  = NULL;
    char_u *fname;

    if (p_asm == 0)
    {
	FREE(this->asm_add);
	return NULL;
    }

    switch (state)
    {
	case OOB_CMD:
	    if (this->asm_add != NULL && ! gdb_as_frset(this, obs))
	    {
		FREE(this->pool.name);

		/* get a unique file name for this disassembled function
		 * and open the file for creation
		 * the file name suffix is '.clasm' so that it can be set
		 * to 'autoread' by the runtime file clewn.vim */
		if (this->asm_func != NULL) {
		    OBSTACK_STRCAT(obs, this->asm_func);
		    obstack_strcat0(obs, ".clasm");
		    fname = (char_u *)obstack_finish(obs);
		}
		if (this->asm_func == NULL || (this->pool.fd = clewn_opentmpfile(fname,
			&(this->pool.name), 1)) == NULL)
		{
		    this->pool.hilite = FALSE;
		    return NULL;
		}
		this->pool.line_offset = 0L;

		/* create the buffer */
		if ((this->pool.buf = cnb_create_buf(this->pool.name)) == -1)
		{
		    this->pool.hilite = FALSE;

		    fclose(this->pool.fd);
		    this->pool.fd = NULL;
		    if (this->pool.name != NULL)
			(void)unlink(this->pool.name);
		    FREE(this->pool.name);
		    return NULL;
		}

		/* init msg_busy */
		obstack_strcat(obs, "Disassembling 0x");
		OBSTACK_STRCAT0(obs, this->asm_add);
		res = (char_u *)obstack_finish(obs);
		gdb_msg_busy(res);

		/* send cmd */
		this->lastline = 0;
		obstack_strcat(obs, "server disassemble 0x");
		OBSTACK_STRCAT(obs, this->asm_add);
		obstack_strcat0(obs, "\n");
		return (char_u *)obstack_finish(obs);
	    }
	    this->pool.hilite = FALSE;
	    break;

	case OOB_COLLECT:
	    if (this->pool.fd == NULL)
		goto fail;

	    /* ignore when interrupted */
	    if (this->oob.state & OS_INTR)
		return NULL;

	    /* concatenate with line after an annotation
	     * and replace line, otherwise add */
	    if (this->line != NULL && this->annoted)
	    {
		OBSTACK_STRCAT(obs, this->line);
		OBSTACK_STRCAT0(obs, line);
		line = (char_u *)obstack_finish(obs);

		this->lastline--;
		if (fseek(this->pool.fd, this->pool.line_offset, SEEK_SET) != 0)
		    goto fail;
	    }

	    /* write a newline except at first line */
	    if (this->lastline != 0)
	    {
		if ((this->pool.line_offset = ftell(this->pool.fd)) == -1
			|| fputs("\n", this->pool.fd) < 0)
		    goto fail;
	    }
	    this->lastline++;

	    /* write the line */
	    if (fputs(line, this->pool.fd) < 0)
		goto fail;

	    if ((this->oob.cnt % 400) == 0)
		gdb_msg_busy(NULL);

	    xfree(this->line);
	    this->line = (char_u *)clewn_strsave((char *)line);
	    break;

	case OOB_COMPLETE:
	    if (this->pool.fd == NULL)
		goto fail;

	    if (this->oob.state & OS_INTR)
	    {
		FREE(this->asm_add);
		this->pool.hilite = FALSE;
		goto fail;
	    }
	    else
	    {
		/* highlite $asm_add */
		(void)gdb_as_frset(this, obs);

		/* write last new line */
		if (fputs("\n", this->pool.fd) < 0)
		    goto fail;

		/* clear the line */
		gdb_msg_busy("FIN");

		/* set the buffer as an asm buffer */
		cnb_set_asm(this->pool.buf);
	    }

	    FREE(this->asm_add);

	    this->pool.buf = -1;
	    fclose(this->pool.fd);
	    this->pool.fd = NULL;
	    FREE(this->pool.name);
	    this->pool.hilite = FALSE;
	    break;
    }
    return NULL;
fail:
    /* clear the line */
    gdb_msg_busy("FIN");

    cnb_kill(this->pool.buf);
    this->pool.buf = -1;

    if (this->pool.fd != NULL)
	fclose(this->pool.fd);
    this->pool.fd = NULL;

    if (this->pool.name != NULL)
	(void)unlink(this->pool.name);
    FREE(this->pool.name);
    return NULL;
}
#  endif /* FEAT_GDB */

#  ifdef GDB_LVL3_SUPPORT
/*
 * Get the breakpoints info record table.
 */
    static char *
get_bp(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *res    = NULL;
    char_u *record = NULL;
    char_u *ptr;
    bpinfo_T *p;

    switch (state)
    {
	case OOB_CMD:
	    /* gdb 6.4 does not provide anymore the right annotations for breakpoints
	     * when in level 3 (note that they are still there when setting annotate
	     * level 2, I thought level 2 was deprecated, what a zoo !), so we look
	     * for new or changed breakpoints systematically now */
	    if (this->mode == GDB_MODE_LVL3)
		this->bp_state = BPS_INVALID;

	    /* When an ANO_BP_INVALID annotation has been received,
	     * or when we are stepping (more accurately: got a new frame):
	     * fetch bp table */
	    if (this->bp_state & BPS_INVALID || this->bp_state & BPS_FR_INVALID)
	    {
		FREE(this->lvl3.result);
		this->bufIsChanged = FALSE;

		/* handle case where an error occured last time */
		FREE(this->record);
		gdb_free_bplist(&(this->tmplist));

		/* fetch GDB bp table info */
		return "server interpreter-exec mi -break-list\n";
	    }

	    this->bp_state &= ~BPS_INVALID;
	    this->bp_state &= ~BPS_FR_INVALID;
	    this->bp_state &= ~BPS_BP_HIT;
	    break;

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.result);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.result);
	    this->lvl3.result = res;
	    break;

	case OOB_COMPLETE:
	    if ((ptr = this->lvl3.result) != NULL)
	    {
		/* process all records except last */
		do
		{
		    if ((ptr = STRSTR(ptr, BKPT_RECORD)) != NULL
			    && ptr > this->lvl3.result)
		    {
			*(ptr - 1) = NUL;
			if (record != NULL)
			    process_record(this, record, obs);

			record = ptr;
			ptr++;
		    }
		}
		while (ptr != NULL);

		/* process last  record */
		if (record != NULL)
		    process_record(this, record, obs);

		FREE(this->record);

		/* All records left in the old table are breakpoints that have
		 * been deleted: delete the corresponding highliting sign */
		for (p = this->bpinfo; p != NULL; p = p->next)
		    gdb_unlite(BP_SIGN_ID(p->id));

		this->bp_state &= ~BPS_INVALID;
		this->bp_state &= ~BPS_FR_INVALID;
		this->bp_state &= ~BPS_BP_HIT;

		/* replace with new table */
		gdb_free_bplist(&(this->bpinfo));
		this->bpinfo = this->tmplist;
		this->tmplist = NULL;

		FREE(this->lvl3.result);
	    }
	    break;
    }
    return NULL;
}

#   define BKPT_ENABLED	    "\",enabled=\""
#   define BKPT_DISP	    "\",disp=\""
#   define BKPT_SCRIPT	    "\",script={"
#   define BKPT_TYPE	    "\",type=\"breakpoint\""
#   define BKPT_LINE	    "\",line=\""
#   define BKPT_SOURCE	    "\",file=\""
#   define BKPT_AT	    "\",at=\"<"
/* Process the current info record */
    static void
process_record(this, record, obs)
    gdb_T *this;
    char_u *record;
    struct obstack *obs;
{
    char_u *bp_add    = NULL;
    char_u *bp_line   = NULL;
    char_u *bp_source = NULL;
    char_u *bp_at     = NULL;
    char_u *ptr;
    char_u *end;
    char_u *quote;
    char_u *plus;

    /* allocate a new record or reuse an existing one
     * and initialize its fields */
    if (this->record == NULL)
	this->record = (bpinfo_T *)xcalloc(sizeof(bpinfo_T));

    this->record->id		= -1;
    this->record->enabled	= TRUE;
#   ifdef BP_INVALID_ANO_MISSING
    this->record->disposition	= TRUE;
#   endif
    this->record->cont		= FALSE;
#   ifdef FEAT_GDB
    this->record->buf		= NULL;
#   else
    this->record->buf		= -1;
    this->record->typenr_en	= -1;
    this->record->typenr_dis	= -1;
#   endif
    this->record->lnum		= 0;
    this->record->next		= NULL;

    /* breakpoint number */
    this->record->id = atoi((char *)(record + strlen(BKPT_RECORD)));

    /* enabled state */
    if ((ptr = STRSTR(record, BKPT_ENABLED)) != NULL
	    && *(ptr + strlen(BKPT_ENABLED)) == 'n')
	this->record->enabled = FALSE;

#   ifdef BP_INVALID_ANO_MISSING
    /* disposition */
    if ((ptr = STRSTR(record, BKPT_DISP)) != NULL
	    && STRSTR(ptr + strlen(BKPT_DISP), "keep") == NULL)
	this->record->disposition = FALSE;
#   endif

    /* parse script for 'continue' as last statement */
    if ((ptr = STRSTR(record, BKPT_SCRIPT)) != NULL)
    {
	ptr += strlen(BKPT_SCRIPT);
	if ((end = STRCHR(ptr, '}')) != NULL)
	{
	    while ((ptr = STRCHR(ptr, '"')) != NULL && ptr < end)
	    {
		ptr++;
		if (gdb_regexec(ptr, PAT_BP_CONT, 1, obs) != NULL
			|| gdb_regexec(ptr, PAT_BP_CONT, 2, obs) != NULL)
		{
		    this->record->cont = TRUE;    /* continue */
		    break;
		}
	    }
	}
    }

    /* sanity check and discard watchpoints and others */
    if (this->record->id <= 0 || STRSTR(record, BKPT_TYPE) == NULL)
	return;

    /* address */
    if ((ptr = STRSTR(record, BKPT_ADDR)) != NULL)
	bp_add = gdb_regexec(ptr + strlen(BKPT_ADDR), PAT_ADD, 1, obs);

    /* line */
    if ((ptr = STRSTR(record, BKPT_LINE)) != NULL)
    {
	ptr += strlen(BKPT_LINE);
	if ((quote = STRCHR(ptr, '"')) != NULL)
	    bp_line = (char_u *)obstack_copy0(obs, ptr, (quote - ptr));
    }

    /* source */
    if ((ptr = STRSTR(record, BKPT_SOURCE)) != NULL)
    {
	ptr += strlen(BKPT_SOURCE);
	if ((quote = STRCHR(ptr, '"')) != NULL)
	    bp_source = (char_u *)obstack_copy0(obs, ptr, (quote - ptr));
    }

    /* at */
    if ((ptr = STRSTR(record, BKPT_AT)) != NULL)
    {
	ptr += strlen(BKPT_AT);
	if ((quote = STRCHR(ptr, '>')) != NULL)
	{
	    if ((plus = STRCHR(ptr, '+')) != NULL && plus < quote)
		bp_at = (char_u *)obstack_copy0(obs, ptr, (plus - ptr));
	    else
		bp_at = (char_u *)obstack_copy0(obs, ptr, (quote - ptr));
	}
    }

    gdb_process_record(this, bp_add, bp_at, bp_line, bp_source, obs);
}

/* Update the variables objects and the variables window */
    static char *
varobj_update(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
#  ifdef FEAT_GDB
    char_u *ptrn;
#  else
    static char *result = NULL;	    /* mark the start and end of object operations */
#  endif
    char_u *res  = NULL;
    varobj_T *obj;

    switch (state)
    {
	case OOB_CMD:
	    if (this->lvl3.varitem == NULL)
	    {
		/* start with first in list */
		this->lvl3.varitem = this->lvl3.varlist;
		this->lvl3.varnext_cmd = VCMD_INIT;
	    }
oob_cmd:
    /* next OOB_CMD */
	    if ((obj = this->lvl3.varitem) != NULL)
	    {
		FREE(this->lvl3.result);

		/* create a new object and give it a name */
		if (this->lvl3.varnext_cmd == VCMD_INIT && obj->name == NULL)
		{
		    if (
#   ifdef FEAT_GDB
			    this->var_buf != NULL
#   else
			    this->var_buf > 0
#   endif
		       )
		    {
			if (obj->format != NULL)
			    this->lvl3.varnext_cmd = VCMD_FORMAT;
			else
			    this->lvl3.varnext_cmd = VCMD_PRINT;

			this->lvl3.varcmd = VCMD_CREATE;

			obstack_strcat(obs, "server interpreter-exec mi \"-var-create - * (");
			OBSTACK_STRCAT(obs, obj->expression);
			obstack_strcat0(obs, ")\"\n");
			return (char *)obstack_finish(obs);
		    }
		    else
		    {
			/* remove silently from list */
			remove_object(this, obj);
			goto oob_cmd;
		    }
		}

		/* sanity check: an object must have a name
		 * this should never occur */
		if (obj->name == NULL)
		{
		    /* silently ignore this error */
		    this->lvl3.varnext_cmd = VCMD_INIT;
		    this->lvl3.varitem = obj->next;
		    goto oob_cmd;
		}

		/* search for the object in the variables window
		 * if it does not exist, then -var-delete it */
		if (this->lvl3.varnext_cmd == VCMD_INIT)
		{
#  ifdef FEAT_GDB
		    pos_T pos;

		    pos.lnum = 1;
		    pos.col = 0;

		    obstack_strcat(obs, "^\\s*");
		    OBSTACK_STRCAT(obs, obj->name);
		    obstack_strcat0(obs, ":");
		    ptrn = (char_u *)obstack_finish(obs);

		    if (this->var_buf == NULL
			    || (this->var_buf->b_ml.ml_flags & ML_EMPTY)
			    || (searchit(NULL, this->var_buf, &pos,
				    FORWARD, ptrn, 1L, SEARCH_KEEP, RE_LAST, (linenr_T)0, NULL) == FAIL))
#  else
		    int lnum;

		    if (this->var_buf <= 0 || cnb_search_obj(obj->name, &lnum) == NULL)
#  endif
		    {
			/* not in gdb variables window, delete it */
			this->lvl3.varcmd = VCMD_DELETE;

			obstack_strcat(obs, "server interpreter-exec mi \"-var-delete var");
			OBSTACK_STRCAT(obs, obj->name);
			obstack_strcat0(obs, "\"\n");
			return (char *)obstack_finish(obs);
		    }
		}

		/* list children if any (need to run this cmd before update) */
		if (this->lvl3.varnext_cmd == VCMD_INIT)
		{
		    this->lvl3.varnext_cmd = VCMD_UPDATE;

		    if (obj->children)
		    {
			this->lvl3.varcmd = VCMD_CHILDREN;

			obstack_strcat(obs, "server interpreter-exec mi \"-var-list-children var");
			OBSTACK_STRCAT(obs, obj->name);
			obstack_strcat0(obs, "\"\n");
			return (char *)obstack_finish(obs);
		    }
		}

		/* set the format */
		if (this->lvl3.varnext_cmd == VCMD_FORMAT)
		{
		    if (obj->format != NULL && STRLEN(obj->format) == 2)
		    {
			obstack_strcat(obs, "server interpreter-exec mi \"-var-set-format var");
			OBSTACK_STRCAT(obs, obj->name);

			switch(*(obj->format + 1))
			{
			    case 't':
				obstack_strcat0(obs, " binary\"\n");
				break;
			    case 'd':
				obstack_strcat0(obs, " decimal\"\n");
				break;
			    case 'x':
				obstack_strcat0(obs, " hexadecimal\"\n");
				break;
			    case 'o':
				obstack_strcat0(obs, " octal\"\n");
				break;
			    default:
				obstack_strcat0(obs, " natural\"\n");
				break;
			}

			this->lvl3.varnext_cmd = VCMD_PRINT;
			this->lvl3.varcmd = VCMD_FORMAT;

			return (char *)obstack_finish(obs);
		    }
		    else
		    {
			this->lvl3.varnext_cmd = VCMD_INIT;
			this->lvl3.varitem = obj->next;
			goto oob_cmd;
		    }
		}

		/* update object */
		if (this->lvl3.varnext_cmd == VCMD_UPDATE)
		{
		    this->lvl3.varnext_cmd = VCMD_PRINT;
		    this->lvl3.varcmd = VCMD_UPDATE;

		    obstack_strcat(obs, "server interpreter-exec mi \"-var-update var");
		    OBSTACK_STRCAT(obs, obj->name);
		    obstack_strcat0(obs, "\"\n");
		    return (char *)obstack_finish(obs);
		}

		/* print */
		if (this->lvl3.varnext_cmd == VCMD_PRINT)
		{
		    obj->state &= ~VS_ERROR;
		    this->lvl3.varcmd = VCMD_PRINT;

		    obstack_strcat(obs, "server output ");
		    OBSTACK_STRCAT(obs, obj->format);
		    obstack_strcat(obs, " ");
		    OBSTACK_STRCAT(obs, obj->expression);
		    obstack_strcat0(obs, "\n");
		    return (char *)obstack_finish(obs);
		}
		
		/* evaluate object expression */
		if (this->lvl3.varnext_cmd == VCMD_EVALUATE)
		{
		    this->lvl3.varcmd = VCMD_EVALUATE;

		    obstack_strcat(obs, "server interpreter-exec mi \"-var-evaluate-expression var");
		    OBSTACK_STRCAT(obs, obj->name);
		    obstack_strcat0(obs, "\"\n");
		    return (char *)obstack_finish(obs);
		}
	    }
	    break;

	case OOB_COLLECT:
	    gdb_cat(&res, this->lvl3.result);
	    gdb_cat(&res, line);
	    xfree(this->lvl3.result);
	    this->lvl3.result = res;
	    break;

	case OOB_COMPLETE:
#  ifdef FEAT_GDB
	    return varobj_complete(this, obs);
#  else
	    /* start of object operations */
	    if (result == NULL && this->var_buf > 0)
		cnb_startAtomic(this->var_buf);

	    result = varobj_complete(this, obs);

	    /* end of object operations */
	    if (result == NULL && this->var_buf > 0)
		cnb_endAtomic(this->var_buf);

	    return result;
#  endif
    }

    return NULL;
}

#   define VOBJ_NAME	    "^done,name=\"var"
#   define VOBJ_CHILD	    "\",numchild=\""
#   define VOBJ_SCOPE	    "\",in_scope=\""
#   define VOBJ_VALUE	    "^done,value=\""
/*
 * Process the OOB_COMPLETE part of varobj_update()
 * Return anything not NULL except when last object in varlist
 * and after the last varcmd which may be: a failed VCMD_CREATE,
 * VCMD_DELETE, highlighted VCMD_UPDATE, VCMD_PRINT, VCMD_EVALUATE
 */
    static char *
varobj_complete(this, obs)
    gdb_T *this;
    struct obstack *obs;
{
    char_u *displine;	/* the new display item line */
    char_u *res;
#  ifdef FEAT_GDB
    buf_T *oldbuf    = curbuf;
    linenr_T lnum;
    win_T *win;
#  endif
    varobj_T *obj;
    char_u *ptr;
    char_u *last;
    char_u *child;
    char_u *quote;

    if ((obj = this->lvl3.varitem) != NULL)
    {
	switch (this->lvl3.varcmd)
	{
	    case VCMD_CREATE:
		/* result of object creation */
		if (this->lvl3.result != NULL
			&& (ptr = STRSTR(this->lvl3.result, VOBJ_NAME)) != NULL
			&& (ptr += strlen(VOBJ_NAME))
			&& (quote = STRCHR(ptr, '"')) != NULL
			&& (child = STRSTR(this->lvl3.result, VOBJ_CHILD)) != NULL)
		{
		    obj->name = (char_u *)clewn_strnsave((char *)ptr, (quote - ptr));
		    obj->children = (atoi((char *)(child + strlen(VOBJ_CHILD))) > 0);

		    FREE(this->lvl3.result);
		    return (char *)obj;   /* next command */
		}
		else
		{
		    EMSG(_("Unable to create variable object"));
		    remove_object(this, obj);
		    FREE(this->lvl3.result);
		    this->lvl3.varnext_cmd = VCMD_INIT;
		    return (char *)this->lvl3.varitem;  /* next object */
		}
		break;

	    case VCMD_DELETE:
		remove_object(this, obj);
		FREE(this->lvl3.result);
		this->lvl3.varnext_cmd = VCMD_INIT;
		return (char *)this->lvl3.varitem;  /* next object */

	    case VCMD_CHILDREN:
		FREE(this->lvl3.result);
		return (char *)obj;   /* next command */

	    case VCMD_FORMAT:
		FREE(this->lvl3.result);
		return (char *)obj;   /* next command */

	    case VCMD_UPDATE:
		if (this->lvl3.result != NULL
			&&(ptr = STRSTR(this->lvl3.result, VOBJ_SCOPE)) != NULL)
		{
		    ptr += strlen(VOBJ_SCOPE);
		    if ((STRSTR(ptr, "false")) == ptr)
		    {
			/* set "out of scope" highlighting (={-}) */
			varobj_hilite(this, obj, (int)'-', obs);
			goto nextobj;
		    }
		    else
		    {
			/* object needs updating */
			FREE(this->lvl3.result);
			return (char *)obj;   /* next command */
		    }
		}
		else
		{
		    /* turn off highlighting */
		    varobj_hilite(this, obj, (int)'=', obs);
		    goto nextobj;
		}
		break;

	    case VCMD_PRINT:
		/* fall back to GDB/MI when unable to print expression
		 * because it's not the right frame */
		if (obj->state & VS_ERROR)
		{
		    obj->state &= ~VS_ERROR;
		    this->lvl3.varnext_cmd = VCMD_EVALUATE;
		    FREE(this->lvl3.result);
		    return (char *)obj;   /* next command */
		}
		
		/* build the display line including ={*}, the hiliting sign */
		OBSTACK_STRCAT(obs, obj->name);
		obstack_strcat(obs, ":");
		OBSTACK_STRCAT(obs, obj->format);
		obstack_strcat(obs, " ");
		OBSTACK_STRCAT(obs, obj->expression);
		obstack_strcat(obs, " ={*} ");
		if (this->lvl3.result != NULL) {
		    OBSTACK_STRCAT0(obs, this->lvl3.result);
		}
		displine = (char_u *)obstack_finish(obs);

		/* add the newly created object to variables window */
		if (obj->state & VS_INIT)
		{
#  ifdef FEAT_GDB
		    if (this->var_buf != NULL)
		    {
			lnum = BUFLASTL(this->var_buf);

			/* edit variables buffer in available window */
			if (gdb_edit_file(this, this->var_buf, NULL, lnum, obs) != NULL)
			{
			    /* add to variables buffer */
			    if (ml_append(lnum, displine, 0, 0) == OK)
			    {
				/* first line ever: remove empty line after the
				 * one just inserted */
				if (lnum == 0)
				    ml_delete(this->var_buf->b_ml.ml_line_count, FALSE);

				changed_lines(this->var_buf->b_ml.ml_line_count - 1,
					0, this->var_buf->b_ml.ml_line_count, 1);
			    }

			    /* update top line */
			    curwin->w_cursor.lnum = this->var_buf->b_ml.ml_line_count;
			    update_topline();

			    /* status line changed */
			    curwin->w_redr_status = TRUE;

			    /* move back to previous window if still there */
			    if ((win = gdb_btowin(oldbuf)) != NULL)
				win_goto(win);
			}
			obj->state &= ~VS_INIT;
		    }
#  else
		    if (this->var_buf > 0)
		    {
			cnb_append(this->var_buf, displine, obs);
			obj->state &= ~VS_INIT;
		    }
#  endif
		}

		/* update and highlight object value */
		else
		    varobj_replace(this, obj, displine, obs);

		goto nextobj;

	    case VCMD_EVALUATE:
		if (this->lvl3.result != NULL
			&& (ptr = STRSTR(this->lvl3.result, VOBJ_VALUE)) != NULL
			&& (ptr += strlen(VOBJ_VALUE))
			&& (quote = (char_u *)strrchr((char *)ptr, '"')) != NULL)
		{
		    res = (char_u *)obstack_copy0(obs, ptr, (quote - ptr));

		    /* remove all `\' in `\"' */
		    for (ptr = last = res; *ptr; ptr++)
		    {
			if (*ptr != '\\' || *(ptr + 1) != '"')
			    *last++ = *ptr;
		    }
		    *last = NUL;

		    /* build the display line including ={*}, the hiliting sign */
		    OBSTACK_STRCAT(obs, obj->name);
		    obstack_strcat(obs, ":");
		    OBSTACK_STRCAT(obs, obj->format);
		    obstack_strcat(obs, " ");
		    OBSTACK_STRCAT(obs, obj->expression);
		    obstack_strcat(obs, " ={*} ");
		    OBSTACK_STRCAT0(obs, res);
		    displine = (char_u *)obstack_finish(obs);


		    /* update and highlight object value */
		    varobj_replace(this, obj, displine, obs);

		}
		goto nextobj;
	}
    }

    return NULL;
nextobj:
    FREE(this->lvl3.result);
    this->lvl3.varitem = obj->next;
    this->lvl3.varnext_cmd = VCMD_INIT;

    /* When not at the end of varlist, return a non null object
     * to get gdb_oob_send() to call again varobj_update() for
     * the next object */
    return (char *)this->lvl3.varitem;
}

#   define OBHI_CHANGED	    " ={*} "
#   define OBHI_UNCHANGED   " ={=} "
#   define OBHI_DESCOPED    " ={-} "
/* Change object highlighting to type */
    static void
varobj_hilite(this, obj, type, obs)
    gdb_T *this;
    varobj_T *obj;
    int type;	    /* hiliting type, may be '*', '=' or '-' */
    struct obstack *obs;
#  ifdef FEAT_GDB
{
    buf_T *oldbuf = curbuf;
    char_u *ptrn;
    char_u *line;
    char_u *oldline;
    char_u *ptr;
    pos_T pos;
    win_T *win;

    pos.lnum = 1;
    pos.col = 0;

    /* search for obj in variables window */
    obstack_strcat(obs, "^\\s*");
    OBSTACK_STRCAT(obs, obj->name);
    obstack_strcat0(obs, ":");
    ptrn = (char_u *)obstack_finish(obs);

    if (this->var_buf != NULL && obj != NULL && obj->name != NULL
	    && ! (this->var_buf->b_ml.ml_flags & ML_EMPTY)
	    && searchit(NULL, this->var_buf, &pos,
		FORWARD, ptrn, 1L, SEARCH_KEEP, RE_LAST, (linenr_T)0, NULL) != FAIL)
    {
	line = ml_get_buf(this->var_buf, pos.lnum, FALSE);
	oldline = obstack_strsave(obs, line);

	if ((type == '-' && ((ptr = STRSTR(oldline, OBHI_CHANGED)) != NULL
			|| (ptr = STRSTR(oldline, OBHI_UNCHANGED)) != NULL))
		|| (type == '*' && ((ptr = STRSTR(oldline, OBHI_UNCHANGED)) != NULL
			|| (ptr = STRSTR(oldline, OBHI_DESCOPED)) != NULL))
		|| (type == '=' && ((ptr = STRSTR(oldline, OBHI_DESCOPED)) != NULL
			|| (ptr = STRSTR(oldline, OBHI_CHANGED)) != NULL)))
	{
	    *(ptr + 3) = type;

	    /* replace line */
	    curbuf = this->var_buf;
	    ml_replace(pos.lnum, oldline, TRUE);
	    changed_lines(pos.lnum, 0, pos.lnum + 1, 0);
	    curbuf = oldbuf;

	    if ((win = gdb_btowin(this->var_buf)) != NULL)
		redraw_win_later(win, NOT_VALID);
	}
    }
}
#  else
{
    char_u *oldline;
    char_u *line;
    char_u *ptr;
    int lnum;

    /* search for obj in variables buffer */
    if (this->var_buf > 0 && obj != NULL && obj->name != NULL
	    && (oldline = cnb_search_obj(obj->name, &lnum)) != NULL)
    {
	line = obstack_strsave(obs, oldline);

	if ((type == '-' && ((ptr = STRSTR(line, OBHI_CHANGED)) != NULL
			|| (ptr = STRSTR(line, OBHI_UNCHANGED)) != NULL))
		|| (type == '*' && ((ptr = STRSTR(line, OBHI_UNCHANGED)) != NULL
			|| (ptr = STRSTR(line, OBHI_DESCOPED)) != NULL))
		|| (type == '=' && ((ptr = STRSTR(line, OBHI_DESCOPED)) != NULL
			|| (ptr = STRSTR(line, OBHI_CHANGED)) != NULL)))
	{
	    *(ptr + 3) = type;

	    /* replace line */
	    cnb_replace(this->var_buf, line, lnum, obs);
	}
    }
}
#  endif

/* Replace object line in variables buffer with line */
    static void
varobj_replace(this, obj, line, obs)
    gdb_T *this;
    varobj_T *obj;
    char_u *line;
    struct obstack *obs;
#  ifdef FEAT_GDB
{
    buf_T *oldbuf = curbuf;
    win_T *oldwin = curwin;
    char_u *ptrn;
    linenr_T lnum;
    win_T *win;
    pos_T pos;

    pos.lnum = 1;
    pos.col = 0;

    /* search for object in variables window */
    obstack_strcat(obs, "^\\s*");
    OBSTACK_STRCAT(obs, obj->name);
    obstack_strcat0(obs, ":");
    ptrn = (char_u *)obstack_finish(obs);

    if (this->var_buf != NULL && obj != NULL && obj->name != NULL
	    && ! (this->var_buf->b_ml.ml_flags & ML_EMPTY)
	    && searchit(NULL, this->var_buf, &pos,
		FORWARD, ptrn, 1L, SEARCH_KEEP, RE_LAST, (linenr_T)0, NULL) != FAIL)
    {
	lnum = pos.lnum;

	/* replace line */
	curbuf = this->var_buf;
	ml_replace(lnum, line, TRUE);
	changed_lines(lnum, 0, lnum + 1, 0);

	if ((win = gdb_btowin(this->var_buf)) != NULL)
	{
	    win->w_cursor.lnum = lnum;

	    curwin = win;
	    check_cursor();
	    update_topline();
	    curwin = oldwin;

	    win->w_redr_status = TRUE;

	    redraw_win_later(win, NOT_VALID);
	}
	curbuf = oldbuf;
    }
}
#  else
{
    int lnum;

    /* search for object in variables buffer */
    if (this->var_buf > 0 && obj != NULL && obj->name != NULL
	    && cnb_search_obj(obj->name, &lnum) != NULL)
	cnb_replace(this->var_buf, line, lnum, obs);
}
#  endif

/* Remove varobj item from valist */
    static void
remove_object(this, item)
    gdb_T *this;
    varobj_T *item;
{
    varobj_T **pt;
    varobj_T *next;

    for (pt = &(this->lvl3.varlist); *pt != NULL; pt = &((*pt)->next))
	if (*pt == item)
	{
	    next = (*pt)->next;
	    this->lvl3.varitem = next;

	    xfree(item->name);
	    xfree(item->format);
	    xfree(item->expression);
	    xfree(item);
	    *pt = next;
	    return;
	}
}
#  endif /* GDB_LVL3_SUPPORT */

/*
 * Add a breakpoint record to the head of the tmp list and highlite
 * the corresponding sign if the record refers to a loaded buffer
 * or to an editable file, and in this case load it.
 */
    void
gdb_process_record(this, address, at, line, source, obs)
    gdb_T *this;
    char_u *address;	/* breakpoint address */
    char_u *at;		/* breakpoint function */
    char_u *line;	/* breakpoint line in source */
    char_u *source;	/* breakpoint source file name */
    struct obstack *obs;
#  ifdef FEAT_GDB
{
    win_T *oldwin    = curwin;
    bpinfo_T *record = this->record;
    char_u *bp_file  = NULL;
    buf_T *buf       = NULL;
    char_u *ptrn;
    bpinfo_T *p, **pt;
    pos_T pos;
    int i;
    int lnum;

    /*
     * Look for this breakpoint in previous list
     */
    for (pt = &(this->bpinfo); *pt != NULL; pt = &((*pt)->next))
    {
	p = *pt;

	/* The breakpoint number exists in the old table:
	 *	update the highliting sign and do not even parse
	 *	the line (assume the file and line of a breakpoint
	 *	are immutable)
	 *	move the old record to tmplist and update its sign
	 */
	if (record->id == p->id)
	{
	    /* update enabled */
	    if (record->enabled != p->enabled)
	    {
		p->enabled = record->enabled;
		if ((p->typenr = gdb_define_sign(p->id, p->enabled)) != -1)
		{
		    buf_addsign(p->buf, BP_SIGN_ID(p->id), p->lnum, p->typenr);
		    update_debug_sign(p->buf, p->lnum);
		}
	    }

#  ifdef BP_INVALID_ANO_MISSING
	    /* update disposition */
	    p->disposition = record->disposition;
#  endif

	    /* update 'continue' */
	    p->cont = record->cont;

	    /* move from old list to new list */
	    *pt = p->next;		    /* unlink from old list */
	    p->next = this->tmplist;	    /* link to head of new table */
	    this->tmplist = p;
	    return;			    /* record may be reused */
	}
    }

    /*
     * A new breakpoint number: edit the file if possible
     */
    /* An asm breakpoint */
    if (address != NULL && at != NULL)
    {
	bp_file = obstack_strsave(obs, at);

	/* Search all buffers in the asm buffer pool whose name starts
	 * with bp_file and find the one containing a line starting
	 * with address */
	obstack_strcat(obs, "^\\s*0x0*");
	OBSTACK_STRCAT0(obs, address);
	ptrn = (char_u *)obstack_finish(obs);

	for (i = 0; i < this->pool.max; i++)
	{
	    pos.lnum = 1;
	    pos.col = 0;

	    if (this->pool.buf[i] != NULL
		    && STRSTR(this->pool.buf[i]->b_fname, bp_file) != NULL
		    && !(this->pool.buf[i]->b_ml.ml_flags & ML_EMPTY)
		    && searchit(NULL, this->pool.buf[i], &pos,
			FORWARD, ptrn, 1L, SEARCH_KEEP, RE_LAST, (linenr_T)0, NULL) != FAIL)
	    {
		this->pool.age[i] = 0;
		record->lnum = pos.lnum;
		buf = this->pool.buf[i];
		break;
	    }
	}

	bp_file = NULL;
    }

    /* A plain breakpoint */
    else if (line != NULL && (record->lnum = atoi((char *)line)) > 0
	    && source != NULL)
    {
	bp_file = obstack_strsave(obs, source);
    }

    /*
     * Edit the file or asm buffer
     * when the table is reported by GDB as having changed
     * or when this buffer is the frame buffer and the frame
     * is invalid (because: we might be setting a frame in
     * a newly disassembled buffer and must set again all
     * its breakpoints)
     */
    if ((bp_file != NULL || buf != NULL)
	    && (this->bp_state & BPS_INVALID
		    || (p_asm != 0
			&& buf != NULL
			&& buf == this->fr_buf
			&& (this->bp_state & BPS_FR_INVALID)))
	    )
    {
	/* bufIsChanged is TRUE when previous gdb_edit_file() failed because
	 * the user cancelled the operation after having been warned the
	 * buffer is changed. In this case, do not try to gdb_edit_file()
	 * again for the next new breakpoints
	 * (as the operation is cancelled). */
	if (! this->bufIsChanged)
	{
	    if (gdb_edit_file(this, buf, bp_file, record->lnum, obs) != NULL)
	    {
		/* MUST redraw the screen before calling update_debug_sign():
		 *  update_debug_sign() invokes win_update()
		 *  the screen might have been scrolled when Vim ask the
		 *  user to confirm changes made to the previous buffer */
		gdb_redraw(curwin->w_buffer);

		record->buf = curwin->w_buffer;

		if ((record->typenr =
			    gdb_define_sign(record->id, record->enabled)) != -1)
		{
		    /* add bp sign */
		    buf_addsign(record->buf, BP_SIGN_ID(record->id),
			    record->lnum, record->typenr);
		    update_debug_sign(record->buf, record->lnum);

		    /* Set the frame sign again in case this is
		     * a newly disassembled buffer */
		    if (this->frame_pc != NULL
			    && p_asm != 0
			    && this->fr_buf == NULL
			    && ! (record->buf->b_ml.ml_flags & ML_EMPTY))
		    {
			pos.lnum = 1;
			pos.col = 0;

			obstack_strcat(obs, "^\\s*0x0*");
			OBSTACK_STRCAT0(obs, this->frame_pc);
			ptrn = (char_u *)obstack_finish(obs);

			if (ptrn != NULL
				&& searchit(NULL, record->buf, &pos, FORWARD,
				    ptrn, 1L, SEARCH_KEEP, RE_LAST, (linenr_T)0, NULL) != FAIL)
			{
			    gdb_fr_lite(this, record->buf, pos.lnum, obs);
			}
		    }

		    /* Handle the case where bps are set after the frame sign:
		     * new disassembled code or reediting a wiped-out plain
		     * buffer or stepping in a newly disassembled buffer */
		    if (record->buf == this->fr_buf
			    && (this->bp_state & BPS_BP_HIT
				|| (p_asm != 0
				    && (this->bp_state & BPS_FR_INVALID)))
			    && (lnum =
				buf_findsign(this->fr_buf, FRAME_SIGN)) != 0)
		    {
			/* remove frame sign and add it again on top
			 * of this new breakpoint */
			if (lnum == record->lnum)
			    gdb_fr_lite(this, this->fr_buf, lnum, obs);

			/* move back cursor to frame */
			gdb_set_cursor(gdb_btowin(this->fr_buf), lnum);
		    }
		}

		record->next = this->tmplist;	/* link to head of new table */
		this->tmplist = record;
		this->record = NULL;		/* forget record */
	    }
	    else
	    {
		this->bufIsChanged = bufIsChanged(curbuf);
		win_goto(oldwin);
	    }
	}
    }

    return;
}
#  else
{
    bpinfo_T *record = this->record;
    char_u *bp_file  = NULL;
    int buf          = -1;
    bpinfo_T *p, **pt;
    char_u *fname;
    linenr_T lnum;
    int bufno;

    /*
     * Look for this breakpoint in previous list
     */
    for (pt = &(this->bpinfo); *pt != NULL; pt = &((*pt)->next))
    {
	p = *pt;

	/* The breakpoint number exists in the old table:
	 *	remove old sign and replace by new one
	 *	move the old record to tmplist and update its sign
	 */
	if (record->id == p->id)
	{
	    /* update enabled */
	    if (record->enabled != p->enabled)
	    {
		p->enabled = record->enabled;
		if ((p->typenr = gdb_define_bpsign(p, obs)) != -1)
		{
		    cnb_buf_delsign(p->buf, BP_SIGN_ID(p->id));
		    cnb_buf_addsign(p->buf, BP_SIGN_ID(p->id), p->typenr, p->lnum, obs);
		}
	    }

#  ifdef BP_INVALID_ANO_MISSING
	    /* update disposition */
	    p->disposition = record->disposition;
#  endif

	    /* update 'continue' */
	    p->cont = record->cont;

	    /* move from old list to new list */
	    *pt = p->next;		    /* unlink from old list */
	    p->next = this->tmplist;	    /* link to head of new table */
	    this->tmplist = p;
	    return;			    /* record may be reused */
	}
    }

    /*
     * A new breakpoint number: edit the file if possible
     */
    /* An asm breakpoint */
    if (address != NULL && at != NULL)
    {
	bp_file = obstack_strsave(obs, at);

	/* Search all buffers in the asm buffer pool whose name starts
	 * with bp_file and find the one containing a line starting
	 * with address */
	for (bufno = 1; ! cnb_outofbounds(bufno); bufno++)
	{
	    if ((fname = cnb_filename(bufno)) != NULL
		    && STRSTR(fname, bp_file) != NULL
		    && (lnum = searchfor(fname, address)) > 0)
	    {
		record->lnum = lnum;
		buf = bufno;
		break;
	    }
	}

	bp_file = NULL;
    }

    /* A plain breakpoint */
    else if (line != NULL && (record->lnum = atoi((char *)line)) > 0
	    && source != NULL)
    {
	bp_file = obstack_strsave(obs, source);
    }

    /*
     * Edit the file or asm buffer
     * when the table is reported by GDB as having changed
     * or when this buffer is the frame buffer and the frame
     * is invalid (because: we might be setting a frame in
     * a newly disassembled buffer and must set again all
     * its breakpoints)
     */
    if ((bp_file != NULL || cnb_isvalid_buffer(buf))
	    && (this->bp_state & BPS_INVALID
		    || (p_asm != 0
			&& cnb_isvalid_buffer(buf)
			&& buf == this->fr_buf
			&& (this->bp_state & BPS_FR_INVALID)))
	    )
    {
	if ((record->buf = gdb_edit_file(buf, bp_file, record->lnum, 1, obs)) > 0)
	{
	    if ((record->typenr = gdb_define_bpsign(record, obs)) != -1)
	    {
		/* add bp sign */
		cnb_buf_addsign(record->buf, BP_SIGN_ID(record->id),
			record->typenr, record->lnum, obs);

		/* Set the frame sign again in case this is
		 * a newly disassembled buffer */
		if (this->frame_pc != NULL
			&& p_asm != 0
			&& ! cnb_isvalid_buffer(this->fr_buf)
			&& (fname = cnb_filename(record->buf)) != NULL
			&& (lnum = searchfor(fname, this->frame_pc)) > 0)
		{
		    gdb_fr_lite(this, record->buf, lnum, obs);
		}

		/* Handle the case where bps are set after the frame sign:
		 * new disassembled code or reediting a wiped-out plain
		 * buffer or stepping in a newly disassembled buffer */
		if (record->buf == this->fr_buf
			&& (this->bp_state & BPS_BP_HIT
			    || (p_asm != 0
				&& (this->bp_state & BPS_FR_INVALID)))
			&& (lnum = find_fr_sign(this->fr_buf)) != 0)
		{
		    /* remove frame sign and add it again on top
		     * of this new breakpoint */
		    if (lnum == record->lnum)
			gdb_fr_lite(this, this->fr_buf, lnum, obs);
		}
	    }

	    record->next = this->tmplist;	/* link to head of new table */
	    this->tmplist = record;
	    this->record = NULL;		/* forget record */
	}
    }

    return;
}
#  endif /* FEAT_GDB */

/*
 * Parse str for annotation, set this->note.
 * Return byte address following annotation, NULL if none found.
 * Return byte address following ^Z^Z when unknown annotation.
 */
    static char_u *
parse_note(this, str)
    gdb_T *this;
    char_u *str;	/* string to parse */
{
    char_u *note = NULL;
    annotation_T *pt;
    int len;

    if (str != NULL && (note = STRSTR(str, "\032\032")) != NULL)
    {
	this->note = ANO_ANY;
	note += 2;

	for (pt = annotations; pt->str != NULL; pt++)
	{
	    len = STRLEN(pt->str);

	    if (STRSTR(note, pt->str) == note
		    && (*(note + len) == NUL || isspace(*(note + len))))
	    {
		this->note = pt->id;
		return note + strlen((char *)pt->str);
	    }
	}
    }
    return note;
}

/*
 * Process the last chunk of data received from gdb at the init stage.
 * On some systems, it may contain:
 *      gdb intro + pre_prompt annotation + prompt + post-prompt annotation
 */
    static void
print_version(this, ptr)
    gdb_T * this;
    char_u * ptr;
{
    char_u * sp;
    char_u * ep;
    if ((sp=parse_note(this, ptr)) != NULL
            && (ep=STRSTR(sp, "\032\032")) != NULL)
    {
        *(ep - 1) = NUL;    /* skip annotation new line */
        xfree(this->prompt);
        this->prompt = (char_u *)clewn_strsave(sp + 1);
        if ((ep=STRSTR(ptr, "\032\032")) != NULL)
        {
            *ep = NUL;
            this->note = ANO_POSTPROMPT;
            gdb_write_buf(this, ptr, TRUE);
            this->note = ANO_PROMPT;
        }
    }
    else
        gdb_write_buf(this, ptr, TRUE);
}

#  ifndef FEAT_GDB
/* Return annotation type of 'str' or ANO_NONE when not found. */
    static int
get_note(this, str)
    gdb_T *this;
    char_u *str;	/* string to parse */
{
    int oldnote = this->note;
    int note    = ANO_NONE;

    if (IS_ANNOTATION(str) && parse_note(this, str) != NULL)
	note = this->note;

    this->note = oldnote;
    return note;
}
#  endif /* FEAT_GDB */
# endif /* defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT) */
#endif /* defined(FEAT_GDB) || defined(HAVE_CLEWN) */
