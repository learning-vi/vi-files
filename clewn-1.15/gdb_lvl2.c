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
 * $Id: gdb_lvl2.c 230 2009-11-28 16:50:31Z xavier $
 */

# ifdef HAVE_CLEWN
#  include <config.h>
#  include "obstack.h"
#  include "clewn.h"
# else
#  include "vim.h"
#  include "clewn/obstack.h"
# endif

#if defined(FEAT_GDB) || defined(HAVE_CLEWN)

# include "gdb.h"
# include "misc.h"

# ifdef GDB_LVL2_SUPPORT

/* display item states */
#  define DISP_CHANGED 0x01	/* item expression value has changed */
#  define DISP_HILITED 0x02	/* item expression value is hilited */

/* Gdb process mgmt */
static void clear_gdb_T __ARGS((gdb_T *));

/* Out Of Band */
static char *get_lastbp __ARGS((gdb_T *, int, char_u *, struct obstack *));
static char *get_bp __ARGS((gdb_T *, int, char_u *, struct obstack *));
static void process_record __ARGS((gdb_T *, struct obstack *));
static void set_bpfield __ARGS((gdb_T *, char_u *));
static char *get_display __ARGS((gdb_T *, int, char_u *, struct obstack *));
static char *undisplay __ARGS((gdb_T *, int, char_u *, struct obstack *));

/* Variables window management */
static void var_delete __ARGS((gdb_T *));

/* The function ordering in this array is important as some of
 * these functions must be invoked in the right order */
static oobfunc_T oobfunc[] = {
    {gdb_get_pc},
    {gdb_get_frame},
    {gdb_get_sfile},
    {gdb_get_sourcedir},
    {get_lastbp},	    /* after gdb_get_frame */
    {gdb_get_asmfunc},	    /* after get_lastbp */
    {gdb_get_asmfunc_hack}, /* after gdb_get_asmfunc */
    {gdb_get_asm},	    /* after gdb_get_asmfunc */
    {get_bp},		    /* after gdb_get_asm and get_lastbp */
    {get_display},
    {undisplay},
    {NULL}
};

/* Initialize the gdb_T structure lvl2 component that lvl2 is responsible for */
    static void
clear_gdb_T(this)
    gdb_T *this;
{
    gdbdisp_T *item, *next;
    int i;

    if (this != NULL)
    {
	for (item = this->lvl2.varlist.list; item != NULL; item = next)
	{
	    next = item->next;
	    xfree(item);
	}
	this->lvl2.varlist.list = NULL;

	for (i = 0; i < BI_FIELDS; i++)
	    FREE(this->lvl2.info[i]);

	this->lvl2.doing_value = FALSE;
	FREE(this->lvl2.dentry.num);
	FREE(this->lvl2.dentry.expression);
	FREE(this->lvl2.dentry.value);
	FREE(this->lvl2.dispinfostr);
	FREE(this->lvl2.dispinfo);
    }
}

/* Initialize lvl2 function pointers */
    void
gdb_lvl2_init(this)
    gdb_T *this;
{
    this->mode = GDB_MODE_LVL2;
    this->oobfunc = oobfunc;
    this->std_oobfunc = oobfunc;
    this->parse_output = gdb_parse_output_cli;
    this->gdb_docmd = gdb_docmd_cli;
    this->var_delete = var_delete;
    this->clear_gdb_T = clear_gdb_T;
}

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
    int found = FALSE;
    bpinfo_T *p;
    int bp_num;
    int i;

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

		/* fetch GDB bp table info */
		return "server info breakpoint\n";
	    }
	    break;

	case OOB_COLLECT:
	    /* initialize fields at start of bp record */
	    if (this->bp_state & BPS_START)
	    {
		this->bp_state &= ~BPS_START;

		for (i = 0; i < BI_FIELDS; i++)
		    FREE(this->lvl2.info[i]);
	    }

	    /* get info[] field from annotation content */
	    set_bpfield(this, line);
	    break;

	case OOB_COMPLETE:
	    /* the last bp in the table */
	    if (this->lvl2.info[BI_NUM] != NULL
		    && (bp_num = atoi((char *)this->lvl2.info[BI_NUM])) > 0)
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
		if (! found)
		{
		    xfree(this->asm_add);
		    this->asm_add =
			gdb_regexec(this->lvl2.info[BI_ADDRESS], PAT_ADD, 1, NULL);
		}
	    }

	    for (i = 0; i < BI_FIELDS; i++)
		FREE(this->lvl2.info[i]);
	    break;
    }
    return NULL;
}

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
    bpinfo_T *p;
    int i;

    switch (state)
    {
	case OOB_CMD:
	    /* When an ANO_BP_INVALID annotation has been received,
	     * or when we are stepping (more accurately: got a new frame):
	     * fetch bp table */
	    if (this->bp_state & BPS_INVALID || this->bp_state & BPS_FR_INVALID)
	    {
		this->bufIsChanged = FALSE;

		/* handle case where an error occured last time */
		FREE(this->record);
		gdb_free_bplist(&(this->tmplist));

		return "server info breakpoint\n";
	    }

	    this->bp_state &= ~BPS_INVALID;
	    this->bp_state &= ~BPS_FR_INVALID;
	    this->bp_state &= ~BPS_BP_HIT;
	    break;

	case OOB_COLLECT:
	    if (this->bp_state & BPS_START)
	    {
		this->bp_state &= ~BPS_START;

		/* process last record */
		process_record(this, obs);
		for (i = 0; i < BI_FIELDS; i++)
		    FREE(this->lvl2.info[i]);

		/* allocate a new record or reuse an existing one
		 * and initialize its fields */
		if (this->record == NULL)
		    this->record = (bpinfo_T *)xcalloc(sizeof(bpinfo_T));

		this->record->id	    = -1;
		this->record->enabled	    = TRUE;
#  ifdef BP_INVALID_ANO_MISSING
		this->record->disposition   = TRUE;
#  endif
		this->record->cont	    = FALSE;
#  ifdef FEAT_GDB
		this->record->buf	    = NULL;
#  else
		this->record->buf	    = -1;
		this->record->typenr_en	    = -1;
		this->record->typenr_dis    = -1;
#  endif
		this->record->lnum	    = 0;
		this->record->next	    = NULL;
	    }

	    /* get info[] field from annotation content */
	    set_bpfield(this, line);
	    break;

	case OOB_COMPLETE:
	    /* process last record */
	    process_record(this, obs);
	    FREE(this->record);
	    for (i = 0; i < BI_FIELDS; i++)
		FREE(this->lvl2.info[i]);

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
	    break;
    }
    return NULL;
}

/* Process the current info record */
    static void
process_record(this, obs)
    gdb_T *this;
    struct obstack *obs;
{
    bpinfo_T *record  = this->record;
    char_u *bp_add    = NULL;
    char_u *bp_at     = NULL;
    char_u *bp_line   = NULL;
    char_u *bp_source = NULL;
    char_u *ptr;

    if (record == NULL)
	return;

    /* breakpoint number */
    if (this->lvl2.info[BI_NUM] != NULL)
	record->id = atoi((char *)this->lvl2.info[BI_NUM]);

    /* enabled state */
    if (this->lvl2.info[BI_ENABLE] != NULL && *(this->lvl2.info[BI_ENABLE]) == 'n')
	record->enabled = FALSE;

#  ifdef BP_INVALID_ANO_MISSING
    /* disposition */
    if (this->lvl2.info[BI_DISPO] != NULL
	    && STRSTR(this->lvl2.info[BI_DISPO], "keep") == NULL)
	record->disposition = FALSE;
#  endif

    /* parse 'commands' for 'continue' as last statement */
    if (this->lvl2.info[BI_CMMDS] != NULL)
    {
	/* get last line */
	if ((ptr = (char_u *)strrchr(
	    (char *)this->lvl2.info[BI_CMMDS], (int)'\n')) == NULL)
	    ptr = this->lvl2.info[BI_CMMDS];
	else
	    ptr++;

	if (gdb_regexec(ptr, PAT_BP_CONT, 1, obs) != NULL
		|| gdb_regexec(ptr, PAT_BP_CONT, 2, obs) != NULL)
	{
	    record->cont = TRUE;    /* continue */
	}
    }

    /* sanity check and discard watchpoints and others */
    if (record->id <= 0 || this->lvl2.info[BI_TYPE] == NULL
	    || STRSTR(this->lvl2.info[BI_TYPE], "breakpoint") == NULL
	    || this->lvl2.info[BI_ADDRESS] == NULL
	    || this->lvl2.info[BI_WHAT] == NULL)
	return;

    bp_add = gdb_regexec(this->lvl2.info[BI_ADDRESS], PAT_ADD, 1, obs);
    bp_line = gdb_regexec(this->lvl2.info[BI_WHAT], PAT_BP_SOURCE, 2, obs);
    bp_source = gdb_regexec(this->lvl2.info[BI_WHAT], PAT_BP_SOURCE, 1, obs);
    if ((bp_at = gdb_regexec(this->lvl2.info[BI_WHAT], PAT_BP_ASM, 1, obs)) == NULL)
	bp_at = gdb_regexec(this->lvl2.info[BI_WHAT], PAT_BP_ASM, 2, obs);

    gdb_process_record(this, bp_add, bp_at, bp_line, bp_source, obs);
}

/* Set the annotated indexed field with its possibly partial content */
    static void
set_bpfield(this, content)
    gdb_T *this;
    char_u *content;
{
    char_u *res = NULL;
    char_u **pfield;

    /*
     * Set info[] with each breakpoint info field annotation
     */
    if (IS_RECORD(this->bp_state))
    {
	pfield = &(this->lvl2.info[RECORD_INDEX(this->bp_state)]);

	if (*pfield == NULL)	    /* a new field */
	    *pfield = (char_u *)clewn_strsave((char *)content);
	else if (this->annoted)	    /* concatenate */
	{
	    gdb_cat(&res, *pfield);
	    gdb_cat(&res, content);
	    xfree(*pfield);
	    *pfield = res;
	}
	else			    /* a new line in this field */
	{
	    gdb_cat(&res, *pfield);
	    gdb_cat(&res, (char_u *)"\n");
	    gdb_cat(&res, content);
	    xfree(*pfield);
	    *pfield = res;
	}
    }
}

/*
 * Remove highlighting for all values that are unchanged and get
 * from GDB the current "automatic display list"
 */
    static char *
get_display(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
{
    char_u *res   = NULL;
    gdbdisp_T *item;
    char_u *oldline;
    char_u *ptr;
#  ifdef FEAT_GDB
    buf_T *oldbuf = curbuf;
    char_u *ptrn;
    win_T *win;
    pos_T pos;
#  endif

    switch(state)
    {
	case OOB_CMD:
#  ifndef FEAT_GDB
	    /* tell Vim to not update screen */
	    cnb_startAtomic(this->var_buf);
#  endif

	    if (
#  ifdef FEAT_GDB
		    this->var_buf != NULL && this->lvl2.varlist.state == DSP_STOPPED)
#  else
		    this->var_buf > 0 && this->lvl2.varlist.state == DSP_STOPPED)
#  endif
	    {
		/* remove highlighting for all values that are unchanged */
		for (item = this->lvl2.varlist.list; item != NULL; item = item->next)
		{
#  ifdef FEAT_GDB
		    if (! (this->var_buf->b_ml.ml_flags & ML_EMPTY)
			    && ! (item->state & DISP_CHANGED)
			    && (item->state & DISP_HILITED))
		    {
			pos.lnum = 1;
			pos.col = 0;

			/* search for display item in variables window */
			obstack_strcat(obs, "^\\s*");
			OBSTACK_STRCAT(obs, gdb_itoa(item->num));
			obstack_strcat0(obs, ":");
			ptrn = (char_u *)obstack_finish(obs);

			if (searchit(NULL, this->var_buf, &pos,
				    FORWARD, ptrn, 1L, SEARCH_KEEP, RE_LAST, (linenr_T)0, NULL) != FAIL)
			{
			    oldline = ml_get_buf(this->var_buf, pos.lnum, FALSE);
			    oldline = obstack_strsave(obs, oldline);

			    /* this assumes that  "={*}" is not valid in
			     * any expression or value for all GDB supported
			     * languages */
			    if ((ptr = STRSTR(oldline, " ={*} ")) != NULL)
			    {
				*(ptr + 3) = '=';

				/* replace line */
				curbuf = this->var_buf;
				ml_replace(pos.lnum, oldline, TRUE);
				changed_lines(pos.lnum, 0, pos.lnum + 1, 0);
				curbuf = oldbuf;

				if ((win = gdb_btowin(this->var_buf)) != NULL)
				    redraw_win_later(win, NOT_VALID);
			    }
			}

			item->state &= ~DISP_HILITED;
		    }
#  else
		    if (! (item->state & DISP_CHANGED)
			    && (item->state & DISP_HILITED))
		    {
			int lnum;

			/* search for display item in variables window */
			if ((oldline = cnb_search_obj(gdb_itoa(item->num), &lnum)) != NULL)
			{
			    line = obstack_strsave(obs, oldline);

			    /* this assumes that  "={*}" is not valid in
			     * any expression or value for all GDB supported
			     * languages */
			    if ((ptr = STRSTR(line, " ={*} ")) != NULL)
			    {
				*(ptr + 3) = '=';

				/* replace line */
				cnb_replace(this->var_buf, line, lnum, obs);
			    }
			}
			item->state &= ~DISP_HILITED;
		    }
#  endif

		    item->state &= ~DISP_CHANGED;
		}

		FREE(this->lvl2.dispinfostr);
		xfree(this->lvl2.dispinfo);
		this->lvl2.dispinfo = (char_u *)clewn_strsave(" ");
		return "server info display\n";
	    }
	    break;

	case OOB_COLLECT:
	    if (this->annoted)	    /* concatenate */
	    {
		gdb_cat(&res, this->lvl2.dispinfostr);
		gdb_cat(&res, line);
		xfree(this->lvl2.dispinfostr);
		this->lvl2.dispinfostr = res;
	    }
	    else		    /* a new line */
	    {
		/* add item number to list */
		if ((res = gdb_regexec(this->lvl2.dispinfostr, PAT_DISPINFO, 1, obs)) != NULL)
		{
		    gdb_cat(&(this->lvl2.dispinfo), res);
		    gdb_cat(&(this->lvl2.dispinfo), (char_u *)" ");
		}

		xfree(this->lvl2.dispinfostr);
		this->lvl2.dispinfostr = (char_u *)clewn_strsave((char *)line);
	    }
	    break;

	case OOB_COMPLETE:
	    /* add last item number to list */
	    if ((res = gdb_regexec(this->lvl2.dispinfostr, PAT_DISPINFO, 1, obs)) != NULL)
	    {
		gdb_cat(&(this->lvl2.dispinfo), res);
		gdb_cat(&(this->lvl2.dispinfo), (char_u *)" ");
	    }
	    FREE(this->lvl2.dispinfostr);
	    break;
    }
    return NULL;
}

/*
 * Synchronise the gdb variables window list and our varlist with
 * GDB "automatic display list".
 * It should be sufficient that all items in varlist are included
 * both in the gdb variables window list and in GDB
 * "automatic display list".
 */
    static char *
undisplay(this, state, line, obs)
    gdb_T *this;
    int state;
    char_u *line;
    struct obstack *obs;
#  ifdef FEAT_GDB
{
    char_u *sequence = NULL;	/* sequence of items to undisplay */
    win_T *oldwin    = curwin;
    buf_T *oldbuf    = curbuf;
    char_u *num;
    char_u *ptrn;
    gdbdisp_T **pt;
    gdbdisp_T *next;
    char_u *numstr;
    pos_T pos;
    win_T *win;

    switch(state)
    {
	case OOB_CMD:
	    if (this->lvl2.varlist.state == DSP_STOPPED)
	    {
		for (pt = &(this->lvl2.varlist.list); *pt != NULL; )
		{
		    numstr = gdb_itoa((*pt)->num);

		    /* not in * GDB "automatic display list" */
		    obstack_strcat(obs, " ");
		    OBSTACK_STRCAT(obs, numstr);
		    obstack_strcat0(obs, " ");
		    num = (char_u *)obstack_finish(obs);

		    if (this->lvl2.dispinfo != NULL
			    && this->var_buf != NULL
			    && STRSTR(this->lvl2.dispinfo, num) == NULL)
		    {
			pos.lnum = 1;
			pos.col = 0;

			/* remove line from window if exists */
			obstack_strcat(obs, "^\\s*");
			OBSTACK_STRCAT(obs, numstr);
			obstack_strcat0(obs, ":");
			ptrn = (char_u *)obstack_finish(obs);

			if (! (this->var_buf->b_ml.ml_flags & ML_EMPTY)
				&& searchit(NULL, this->var_buf, &pos,
				    FORWARD, ptrn, 1L, SEARCH_KEEP, RE_LAST, (linenr_T)0, NULL) != FAIL)
			{
			    curbuf = this->var_buf;
			    ml_delete(pos.lnum, FALSE);
			    deleted_lines(pos.lnum, 1);

			    if ((win = gdb_btowin(this->var_buf)) != NULL)
			    {
				curwin = win;
				check_cursor();
				update_topline();
				curwin = oldwin;

				win->w_redr_status = TRUE;

				redraw_win_later(win, NOT_VALID);
			    }
			    curbuf = oldbuf;
			}

			/* remove from varlist */
			next = (*pt)->next;
			xfree(*pt);
			*pt = next;

			continue;
		    }

		    /* not in gdb variables window */
		    obstack_strcat(obs, "^\\s*");
		    OBSTACK_STRCAT(obs, numstr);
		    obstack_strcat0(obs, ":");
		    ptrn = (char_u *)obstack_finish(obs);

		    if (this->var_buf == NULL
			    || (this->var_buf->b_ml.ml_flags & ML_EMPTY)
			    || (searchit(NULL, this->var_buf, &pos,
				    FORWARD, ptrn, 1L, SEARCH_KEEP, RE_LAST, (linenr_T)0, NULL) == FAIL))
		    {
			/* add to sequence */
			OBSTACK_STRCAT(obs, numstr);
			obstack_strcat0(obs, " ");
			sequence = (char_u *)obstack_finish(obs);

			/* remove from varlist */
			next = (*pt)->next;
			xfree(*pt);
			*pt = next;

			continue;
		    }

		    pt = &((*pt)->next);
		}

		this->lvl2.varlist.state = DSP_INIT;
		FREE(this->lvl2.dispinfo);

		if (sequence != NULL)
		{
		    obstack_strcat(obs, "server undisplay ");
		    OBSTACK_STRCAT(obs, sequence);
		    obstack_strcat0(obs, "\n");
		    return (char *)obstack_finish(obs);
		}
	    }
	    break;

	case OOB_COMPLETE:
	    line = NULL;    /* keep compiler happy */
	    break;
    }
    return NULL;
}
#  else
{
    char_u *sequence = NULL;	/* sequence of items to undisplay */
    char_u *num;
    gdbdisp_T **pt;
    gdbdisp_T *next;
    char_u *numstr;
    int lnum;

    switch(state)
    {
	case OOB_CMD:
	    if (this->lvl2.varlist.state == DSP_STOPPED)
	    {
		for (pt = &(this->lvl2.varlist.list); *pt != NULL; )
		{
		    numstr = gdb_itoa((*pt)->num);

		    /* not in * GDB "automatic display list" */
		    obstack_strcat(obs, " ");
		    OBSTACK_STRCAT(obs, numstr);
		    obstack_strcat0(obs, " ");
		    num = (char_u *)obstack_finish(obs);

		    if (this->lvl2.dispinfo != NULL
			    && this->var_buf > 0
			    && STRSTR(this->lvl2.dispinfo, num) == NULL)
		    {
			/* replace line in window if exists with an empty line */
			if (cnb_search_obj(numstr, &lnum) != NULL)
			    cnb_replace(this->var_buf, "", lnum, obs);

			/* remove from varlist */
			next = (*pt)->next;
			xfree(*pt);
			*pt = next;

			continue;
		    }

		    /* not in gdb variables window */
		    if (this->var_buf <= 0 || cnb_search_obj(numstr, &lnum) == NULL)
		    {
			/* add to sequence */
			OBSTACK_STRCAT(obs, numstr);
			obstack_strcat0(obs, " ");
			sequence = (char_u *)obstack_finish(obs);

			/* remove from varlist */
			next = (*pt)->next;
			xfree(*pt);
			*pt = next;

			continue;
		    }

		    pt = &((*pt)->next);
		}

		this->lvl2.varlist.state = DSP_INIT;
		FREE(this->lvl2.dispinfo);

		if (sequence != NULL)
		{
		    obstack_strcat(obs, "server undisplay ");
		    OBSTACK_STRCAT(obs, sequence);
		    obstack_strcat0(obs, "\n");
		    return (char_u *)obstack_finish(obs);
		}
	    }
	    /* tell Vim to update screen */
	    cnb_endAtomic(this->var_buf);
	    break;

	case OOB_COMPLETE:
	    /* tell Vim to update screen */
	    cnb_endAtomic(this->var_buf);
	    line = NULL;    /* keep compiler happy */
	    break;
    }
    return NULL;
}
#  endif

/*
 * Handle the content of a display annotation.
 * Fill up each field.
 * When entry complete, process it:
 *	. if it's the result of a GDB "display" command with an argument:
 *	  add to variables window and varlist
 *	. if it is the output produced by GDB when the program stops or
 *	  an equivalent GDB "display" command without argument: hilite
 *	  changed values in variables window
 */
    void
gdb_process_display(this, line, obs)
    gdb_T *this;
    char_u *line;	/* display annotation content */
    struct obstack *obs;
{
    char_u *res = NULL;
    int note    = this->note;
    char_u *displine;		    /* the new display item line */
    gdbdisp_T *item;
    char_u *last, *end, *start;
    char_u *ptr;
    int num;
#  ifdef FEAT_GDB
    win_T *oldwin    = curwin;
    buf_T *oldbuf    = curbuf;
    char_u *ptrn;
    linenr_T lnum;
    win_T *win;
    pos_T pos;
#  endif

#  ifdef FEAT_GDB
    if (this->var_buf == NULL)
#  else
    if (this->var_buf <= 0)
#  endif
	return;

    /* GDB bug: outputs ANO_DISP_EXP instead of ANO_DISP_VALUE
     * use doing_value as a hack to know when we are parsing the value */
    if (this->lvl2.doing_value && note == ANO_DISP_EXP)
	note = ANO_DISP_VALUE;

    /* nested structure or array */
    if (this->lvl2.doing_value
	    && ((note >= ANO_FIELD_BEG && note <= ANO_FIELD_END)
		|| (note >= ANO_ARRAY_BEG && note <= ANO_ARRAY_END)))
	note = ANO_DISP_VALUE;

    switch (note)
    {
	case ANO_DISP_BEG:
#  ifndef FEAT_GDB
	    /* tell Vim to not update screen */
	    cnb_startAtomic(this->var_buf);
#  endif

	    this->lvl2.doing_value = FALSE;

	    /* display number */
	    if (this->lvl2.dentry.num == NULL)
		this->lvl2.dentry.num = (char_u *)clewn_strsave((char *)line);
	    else
	    {
		gdb_cat(&res, this->lvl2.dentry.num);
		gdb_cat(&res, line);
		xfree(this->lvl2.dentry.num);
		this->lvl2.dentry.num = res;
	    }
	    break;

	case ANO_DISP_FMT:
	case ANO_DISP_EXP:
	    /* display format and expression */
	    if (this->lvl2.dentry.expression == NULL)
		this->lvl2.dentry.expression = (char_u *)clewn_strsave((char *)line);
	    else
	    {
		gdb_cat(&res, this->lvl2.dentry.expression);
		gdb_cat(&res, line);
		xfree(this->lvl2.dentry.expression);
		this->lvl2.dentry.expression = res;
	    }
	    break;

	case ANO_DISP_EXPEND:
	    this->lvl2.doing_value = TRUE;
	    break;

	case ANO_DISP_VALUE:
	    /* display value */
	    if (this->lvl2.dentry.value == NULL)
		this->lvl2.dentry.value = (char_u *)clewn_strsave((char *)line);
	    else
	    {
		gdb_cat(&res, this->lvl2.dentry.value);
		gdb_cat(&res, line);
		xfree(this->lvl2.dentry.value);
		this->lvl2.dentry.value = res;
	    }
	    break;

	/* Process the entry now it's complete */
	case ANO_DISP_END:
	    this->lvl2.doing_value = FALSE;

	    /* sanity checks */
	    if (this->lvl2.dentry.num == NULL
		    || (num = atoi((char *)this->lvl2.dentry.num)) <= 0
		    || this->lvl2.dentry.expression == NULL
		    || this->lvl2.dentry.value == NULL)
	    {
		FREE(this->lvl2.dentry.num);
		FREE(this->lvl2.dentry.expression);
		FREE(this->lvl2.dentry.value);
		return;
	    }

	    /* Build the display line including ={*}, the hiliting sign */
	    OBSTACK_STRCAT(obs, this->lvl2.dentry.num);
	    obstack_strcat(obs, ":");
	    OBSTACK_STRCAT(obs, this->lvl2.dentry.expression);
	    obstack_strcat(obs, " ={*} ");
	    OBSTACK_STRCAT0(obs, this->lvl2.dentry.value);
	    displine = (char_u *)obstack_finish(obs);

	    /*
	     * The result of a GDB "display" command with argument
	     */
	    if (this->cmd_type == CMD_DISPLAY)
	    {
		item = (gdbdisp_T *)xcalloc(sizeof(gdbdisp_T));

		/* add item to varlist */
		item->num = num;
		item->state = DISP_HILITED;
		item->next = this->lvl2.varlist.list;
		this->lvl2.varlist.list = item;

#  ifdef FEAT_GDB
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
#  else
		/* add to variables buffer */
		cnb_append(this->var_buf, displine, obs);
#  endif
	    }

	    /*
	     * Highlight existing changed values in variables window
	     */
	    else
	    {
#  ifdef FEAT_GDB
		pos.lnum = 1;
		pos.col = 0;

		/* search for display item in variables window */
		obstack_strcat(obs, "^\\s*");
		OBSTACK_STRCAT(obs, this->lvl2.dentry.num);
		obstack_strcat0(obs, ":");
		ptrn = (char_u *)obstack_finish(obs);

		if ( ! (this->var_buf->b_ml.ml_flags & ML_EMPTY)
			&& searchit(NULL, this->var_buf, &pos,
			    FORWARD, ptrn, 1L, SEARCH_KEEP, RE_LAST, (linenr_T)0, NULL) != FAIL)
		{
		    lnum = pos.lnum;

		    obstack_strcat(obs, "} ");
		    OBSTACK_STRCAT0(obs, this->lvl2.dentry.value);
		    res = (char_u *)obstack_finish(obs);

		    last = NULL;
		    start = ml_get_buf(this->var_buf, lnum, FALSE);
		    end = start + STRLEN(start);

		    /* get last occurence of "} " + value in this line */
		    for (ptr = start; ptr != NULL; )
			if ((ptr=STRSTR(ptr, res)) != NULL)
			{
			    last = ptr;
			    ptr++;
			}

		    /* a changed value (res not found at line end) */
		    if (last == NULL || end != last + STRLEN(res))
		    {
			/* replace line */
			curbuf = this->var_buf;
			ml_replace(lnum, displine, TRUE);
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

			/* update item state */
			for (item = this->lvl2.varlist.list; item != NULL;
				item = item->next)
			    if (item->num == num)
			    {
				item->state |= DISP_HILITED | DISP_CHANGED;
				break;
			    }
		    }
		}
#  else
		int line_nb;

		/* search for display item in variables window */
		if ((start = cnb_search_obj(this->lvl2.dentry.num, &line_nb)) != NULL)
		{
		    obstack_strcat(obs, "} ");
		    OBSTACK_STRCAT0(obs, this->lvl2.dentry.value);
		    res = (char_u *)obstack_finish(obs);

		    last = NULL;
		    end = start + STRLEN(start);

		    /* get last occurence of "} " + value in this line */
		    for (ptr = start; ptr != NULL; )
			if ((ptr=STRSTR(ptr, res)) != NULL)
			{
			    last = ptr;
			    ptr++;
			}

		    /* a changed value (res not found at line end) */
		    if (last == NULL || end != last + STRLEN(res))
		    {
			/* replace line */
			cnb_replace(this->var_buf, displine, line_nb, obs);

			/* update item state */
			for (item = this->lvl2.varlist.list; item != NULL;
				item = item->next)
			    if (item->num == num)
			    {
				item->state |= DISP_HILITED | DISP_CHANGED;
				break;
			    }
		    }
		}
#  endif
	    }

	    FREE(this->lvl2.dentry.num);
	    FREE(this->lvl2.dentry.expression);
	    FREE(this->lvl2.dentry.value);

#  ifndef FEAT_GDB
	    /* tell Vim to update screen */
	    cnb_endAtomic(this->var_buf);
#  endif
	    break;

	default:
	    break;
    }
}

/* Undisplay all variables */
    static void
var_delete(this)
    gdb_T *this;
{
    struct obstack obs;	/* use an obstack for temporary allocated memory */
    char_u *res;

    /* undisplay all */
    this->lvl2.varlist.state = DSP_STOPPED;

    (void)obstack_init(&obs);

    if ((res = (char_u *)undisplay(this, OOB_CMD, (char_u *)"", &obs)) != NULL)
	gdb_send_cmd(this, res);

    obstack_free(&obs, NULL);
}
# endif /* GDB_LVL2_SUPPORT */
#endif /* defined(FEAT_GDB) || defined(HAVE_CLEWN) */
