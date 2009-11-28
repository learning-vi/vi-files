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
 * $Id: gdb.h 230 2009-11-28 16:50:31Z xavier $
 */

#ifndef GDB_H
# define GDB_H

#define obstack_chunk_alloc  malloc
#define obstack_chunk_free   free
#define obstack_strsave(o,s) (char_u *)obstack_copy0((o), (s), STRLEN((s)))
#define obstack_strcat(o,s) obstack_grow((o), (s), STRLEN((s)))
#define OBSTACK_STRCAT(o,s)			    \
( (s) ? obstack_grow((o), (s), STRLEN((s))) : 0)
#define obstack_strcat0(o,s) obstack_grow0((o), (s), STRLEN((s)))
#define OBSTACK_STRCAT0(o,s)			    \
( (s)						    \
  ? obstack_grow0((o), (s), STRLEN((s)))	    \
  : obstack_grow0((o), "", 0))

#define GDB_LVL2_SUPPORT
#define GDB_LVL3_SUPPORT
/*#define GDB_MI_SUPPORT */

/* |+gdb| modes */
#define GDB_MODE_LVL2		1   /* CLI with annotations level 2 */
#define GDB_MODE_LVL3		2   /* CLI with annotations level 3 and GDB/MI */
#define GDB_MODE_MI		3   /* GDB/MI */

#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
/*
 * GDB does not send ANO_BP_INVALID annotation when hitting a breakpoint
 * that has been 'enable once'.
 * Undefine BP_INVALID_ANO_MISSING after GDB fixes this
 */
# define BP_INVALID_ANO_MISSING
#endif

#define KEY_INTERUPT	Ctrl_Z	/* interrupt */
#define MAX_BUFFSIZE	4096

/* The breakpoint info record structure */
typedef struct bpinfo_struct bpinfo_T;

struct bpinfo_struct
{
    int id;		/* breakpoint number */
    int typenr;		/* sign type number, for Clewn: sequence sign type number in this buffer */
#ifdef BP_INVALID_ANO_MISSING
    int disposition;	/* TRUE when keep, FALSE when disabled */
#endif
    int enabled;	/* TRUE when enabled */
    int cont;		/* TRUE when 'commands' includes continue */
#ifdef FEAT_GDB
    buf_T *buf;		/* breakpoint buffer */
#else
    int buf;		/* Clewn buffer number */
    int typenr_en;	/* enabled breakpoint sequence number */
    int typenr_dis;	/* disabled breakpoint sequence number */
#endif
    linenr_T lnum;	/* breakpoint line number */
    bpinfo_T *next;
};

#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
/* Annotation types */
# define ANO_NONE		0
# define ANO_ANY		1	/* some unrecognized annotation */

/* level 2 and level 3 annotations */
# define ANO_PREPROMPT		101
# define ANO_PROMPT		102
# define ANO_POSTPROMPT		103
# define ANO_PRECMDS		104
# define ANO_CMDS		105
# define ANO_PREOVERLOAD	106
# define ANO_OVERLOAD		107
# define ANO_PREQUERY		108
# define ANO_QUERY		109
# define ANO_PREPMT_FORMORE	110
# define ANO_PMT_FORMORE	111
# define ANO_POSTPMT_FORMORE	112
# define ANO_QUIT		113
# define ANO_ERROR_BEG		114
# define ANO_FRAME_INVALID	115
# define ANO_BP_INVALID		116
# define ANO_STARTING		117
# define ANO_STOPPED		118
# define ANO_EXITED		119
# define ANO_SIGNALLED		120
# define ANO_BREAKPOINT		121
# define ANO_SOURCE		122

# define KEY_KILL	Ctrl_U	/* readline 'unix-line-discard' */
# define LPP_LINES	500	/* large output interruption interval */

/* Command completion states */
# define CS_START	0
# define CS_PENDING	1
# define CS_CHOICE	2
# define CS_DONE	3
# define CS_QUERY	4

/* The gdb CLI cmd */
typedef struct
{
    int state;		/* cmd completion state */
    int cnt;		/* lines output by gdb since last cmd */
    char_u *gdb;	/* partial cmd sent to gdb */
    char_u *readline;	/* gdb readline content */
    char_u *echoed;	/* echoed cmd */
} cli_cmd_T;

/* breakpoint states */
# define BPS_INVALID	0x0100	/* got "breakpoints-invalid" annotation */
# define BPS_BP_HIT	0x0200	/* got "breakpoint" annotation */
# define BPS_FR_INVALID 0x0400	/* got "frames-invalid" annotation */
# define BPS_BP_SET	0x0800	/* a break type GDB cmd is being processed */
# define BPS_RECORD	0x1000	/* parsing a record in the breakpoint table */
# define BPS_START	0x2000	/* start of a record */
#endif /* defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT) */

#ifdef GDB_LVL2_SUPPORT
/* level 2 annotations */
# define ANO_FRAME_BEGIN	201
# define ANO_FRAME_END		202
# define ANO_BP_HEADER		203
# define ANO_BP_TABLE		204
# define ANO_BP_RECORD		205
# define ANO_BP_FIELD0		206	/* Number */
# define ANO_BP_FIELD1		207	/* Type */
# define ANO_BP_FIELD2		208	/* Disposition */
# define ANO_BP_FIELD3		209	/* Enable */
# define ANO_BP_FIELD4		210	/* Address */
# define ANO_BP_FIELD5		211	/* What */
# define ANO_BP_FIELD6		212	/* Frame */
# define ANO_BP_FIELD7		213	/* Condition */
# define ANO_BP_FIELD8		214	/* Ignore-count */
# define ANO_BP_FIELD9		215	/* Commands */
# define ANO_BP_END		216
# define ANO_DISP_BEG		217
# define ANO_DISP_NUMEND	218
# define ANO_DISP_FMT		219
# define ANO_DISP_EXP		220
# define ANO_DISP_EXPEND	221
# define ANO_DISP_VALUE		222	/* not output by GDB */
# define ANO_DISP_END		223
# define ANO_FIELD_BEG		224
# define ANO_FIELD_NAMEND	225
# define ANO_FIELD_VALUE	226
# define ANO_FIELD_END		227
# define ANO_ARRAY_BEG		228
# define ANO_ARRAY_ELT		229
# define ANO_ARRAY_ELTREP	230
# define ANO_ARRAY_ELTEND	231
# define ANO_ARRAY_END		232

/* breakpoints */
# define IS_RECORD(s)		((s) & BPS_RECORD)
# define RECORD_INDEX(s)	((s) & 0xff)
# define SET_RECORD_IDX(s, i)	(((s) & ~0xff) | (i))

/* breakpoint annotation fields indexes */
# define BI_NUM		0
# define BI_TYPE	1
# define BI_DISPO	2
# define BI_ENABLE	3
# define BI_ADDRESS	4
# define BI_WHAT	5
# define BI_FRAME	6
# define BI_COND	7
# define BI_COUNT	8
# define BI_CMMDS	9	/* commands */
# define BI_FIELDS	10	/* number of bp info annotation fields */

/* The display object structure */
typedef struct gdbdisp_struct gdbdisp_T;

struct gdbdisp_struct
{
    int num;		/* display number */
    int state;		/* highlighted or/and changed */
    gdbdisp_T * next;
};

/* display states */
# define DSP_INIT	0
# define DSP_STOPPED	1

/* The display list structure */
typedef struct
{
    int state;
    gdbdisp_T * list;
} displist_T;

/* The display entry structure used to store a GDB display output */
typedef struct
{
    char_u * num;	/* display item number */
    char_u * expression;/* display item format and expression */
    char_u * value;	/* display item value */
} doutput_T;

/* The lvl2 mode structure: lvl2 specific data */
typedef struct
{
    /* breakpoints */
    char_u *info[BI_FIELDS]; /* breakpoint info record contents */

    /* variables (display) window */
    displist_T varlist;	/* list of display items */
    doutput_T dentry;	/* current GDB display output entry */
    int doing_value;	/* when TRUE: currently handling a display value */
    char_u *dispinfostr;/* current line output after "info display" */
    char_u *dispinfo;	/* all item numbers result of a "info display" */
} lvl2_T;
#endif /* GDB_LVL2_SUPPORT */

#ifdef GDB_LVL3_SUPPORT
/* The variable object structure */
typedef struct varobj_struct varobj_T;

/* varobj states */
#define VS_INIT	    0x01    /* object creation */
#define VS_ERROR    0x02    /* a command on this object produced an error */

struct varobj_struct
{
    int state;
    char_u *name;	/* variable name ("NNN" in "varNNN") */
    int children;	/* TRUE when has children */
    char_u *format;	/* print format is "/[tdxo]"*/
    char_u *expression;	/* variable expression */
    varobj_T * next;
};

/* GDB/MI var commands */
# define VCMD_INIT	0
# define VCMD_CREATE	1
# define VCMD_DELETE	2
# define VCMD_CHILDREN	3
# define VCMD_UPDATE	4
# define VCMD_PRINT	5
# define VCMD_EVALUATE	6
# define VCMD_FORMAT	7

/* The lvl3 mode structure: lvl3 specific data */
typedef struct
{
    char_u *result;	/* result of a GDB/MI command */

    int get_source_list;
    char_u *source_cur;	/* result of -file-list-exec-source-file */
    char_u *source_list;/* result of -file-list-exec-source-files */

    /* variables window */
    varobj_T *varlist;	/* list of variable objects */
    varobj_T *varitem;  /* current object in varlist */
    int varcmd;		/* GDB/MI var cmd being processed */
    int varnext_cmd;	/* GDB/MI var next cmd to process */
} lvl3_T;
#endif /* GDB_LVL3_SUPPORT */

/* Command types */
#define CMD_ANY		0
#ifdef FEAT_GDB
# define CMD_DIR	1
#endif
#define CMD_DETACH	2
#define CMD_SHELL	3
#define CMD_STEPI	4	/* instruction stepping cmds */
#define CMD_EXECF	5	/* invalidating asm buffers cmds */
#define CMD_BREAK	6
#define CMD_DISPLAY	7
#define CMD_CREATEVAR	8
#define CMD_UP		9
#define CMD_DOWN	10
#define CMD_FRAME	11
#define CMD_DISABLE	12
#define CMD_DELETE	13
#define CMD_UP_SILENT	14
#define CMD_DOWN_SILENT	15
#define CMD_SLECT_FRAME 16
#define CMD_SYMF	17
#define CMD_RESTART	18
#define CMD_QUIT	19

#define ASM_MAX_BUFF	64	    /* asm buffers pool size */
#define ASM_BUF_NAME	"gdb-asm-"  /* asm buffer name prefix */
#define ASM_OLD	(char_u)(~0U)	    /* asm buffer max age */

/* The assembly buffer pool */
typedef struct
{
#ifdef FEAT_GDB
    int max;			/* buffers in use */
    int idx;			/* current buffer */
    buf_T * buf[ASM_MAX_BUFF];	/* asm buffers */
    char_u age[ASM_MAX_BUFF];	/* buffer's age */
    int last;			/* used for generating unique asm names */
#else
    int buf;			/* number of the asm buffer being disassembled */
    FILE *fd;			/* stream descriptor of asm file being written to */
    char_u *name;		/* file name of the function being disassembled */
    long line_offset;		/* offset of start of last line in file */
#endif
    linenr_T lnum;		/* highlited line number */
    int hilite;			/* TRUE when $pc in asm is highlited */
} asm_T;

/* The gdb structure */
typedef struct gdb_struct gdb_T;

/* Out of band states */
#define OS_CMD    0x01		/* a cmd is being processed */
#define OS_INTR   0x02		/* interrupt sent to gdb */
#define OS_QUIT   0x04		/* 'quit' annotation received */

/* Out of Band */
#define OOB_CMD		0
#define OOB_COLLECT	1
#define OOB_COMPLETE	2
#define IS_OOBACTIVE(t) (((t)->oob.idx) >= 0)

/* The out of band process */
typedef struct
{
    int state;			/* oob state */
    int idx;			/* current function index */
    int cnt;			/* asm output lines count */
} oob_T;

/*
 * oobfunc_T function:
 *
 * When an oobfunc_T[] array is processed by gdb_oob_send() and
 * gdb_oob_receive(), each oob function in the array is called with
 * successive state values of:
 *
 * OOB_CMD:	called at each 'prompt'; should return the GDB cmd
 *		to send to GDB or NULL when no command must be sent;
 *		in this case the next oob function in the oobfunc_T[]
 *		array is processed next with OOB_CMD
 *
 * OOB_COLLECT:	oob may process chunk, a (possibly partial) line from GDB
 *		result of GDB command; may be invoked more than once, or not at all
 *
 * OOB_COMPLETE:indicates end of GDB output; oob may do any final processing;
 *		if oob returns a non NULL value, the same oob function is
 *		called once again with OOB_CMD and the same cycle starts again;
 *		if oob returns a NULL value, the next oob function in the
 *		oobfunc_T[] array is processed with OOB_CMD
 *
 * Return:	NULL, except when state is OOB_CMD or OOB_COMPLETE
 */

typedef struct {
    char * (*oob)(gdb_T *, int state, char_u *chunk, struct obstack *);
} oobfunc_T;

/* gdb states */
#define GDB_STATE(i,s) (((i) != NULL) ? ((((gdb_T *)(i))->state) & (s)):FALSE)
#define GS_INIT	    0		/* initial state */
#define GS_CLOSING  0x001	/* gdb process closing */
#define GS_UP	    0x002	/* gdb up and running */
#define GS_STARTED  0x004	/* gdb just started */
#define GS_ALLOWED  0x008	/* select on pty is allowed */
#define GS_EVENT    0x010	/* got SIGCHLD or gdb output some data */
#define GS_SIGCHLD  0x020	/* got SIGCHLD from gdb */
#define GS_STOPPED  0x040	/* debuggee is stopped */
#define GS_ANO	    0x080	/* parsing an annotation table or list */
#define GS_QUITTING 0x100	/* quitting clewn */

/* parser states */
#define PS_ANY		0
#define PS_PREPROMPT	1	/* before the prompt, after any output from GDB */
#define PS_PROMPT	2	/* after prompt, but before user input */

/* project file sourced states */
#define PROJ_INIT	0
#define PROJ_SOURCEIT	1
#define PROJ_DONE	2

/* The gdb structure */
struct gdb_struct
{
    /* process */
    int instance;	/* gdb instance number */
    int fd;		/* pty file descriptor */
    pid_t pid;		/* process id */
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
    int height;		/* pseudo tty height, non zero when ioctl set */
    int intr_sent;	/* hack: an interrupt has been sent by the user */
#endif
    int state;		/* gdb state */
    char_u *status;	/* gdb status */
    int recurse;	/* disable GS_ALLOWED when calling safe_vgetc */

    /* cmds */
    int cmd_type;	/* cmd type */
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
    cli_cmd_T cli_cmd;	/* current CLI cmd (common to lvl2 and lvl3) */
#endif
    char_u *firstcmd;	/* first cmd in new GDB session */
    int parser;		/* parser states */
    int syntax;		/* when TRUE, enable syntax highlighting */
    char_u *line;	/* last incomplete line */
    char_u *winput_cmd; /* cmd inserted in input-line window */
    char_u *directories;/* gdb search path for source files */
#ifndef FEAT_GDB
    char_u *prompt;	/* the current GDB prompt */
    char * version;	/* clewn version */
    char_u * pwd;	/* current working directory */
    char_u * args;	/* debugge command line arguments */
#endif
    char_u *sfile;	/* symbol file name */

#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
    /* annotations */
    int note;		/* annotation type */
    int annoted;	/* TRUE when parsing an annotation */
    int newline;	/* pending NL, possibly an annotation first char */
    int valid_note;	/* last non prompt-for-more annotation type */
    int prev_note;	/* previous annotation type */
    char_u *annotation;	/* last incomplete annotation */
#endif

    /* disassembly */
    asm_T pool;		/* asm buffers pool */
    char_u *pc;		/* program counter where debuggee is stopped */
    char_u *frame_pc;	/* current frame program counter */
    char_u *oob_result;	/* result of oob gdb_get_frame */
    char_u *asm_add;	/* address whose function must be disassembled */
    char_u *asm_func;	/* asm function name */

    /* breakpoints */
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
    int bp_state;	/* breakpoint annotation state (and index) */
    bpinfo_T *tmplist;	/* tmp new bp list when reading "info beakpoints" */
#endif
    bpinfo_T *record;	/* current record  */
    int bufIsChanged;	/* TRUE when curbuf is changed after edit failed */
    int cont;		/* TRUE when pgm is continuing at this bp */
    bpinfo_T *bpinfo;	/* breakpoints info table */
    int frame_curlvl;	/* current frame level */
    linenr_T frame_lnum;/* current frame line number */
    char_u * frame_fname;/* current frame file name */

#ifdef FEAT_GDB
    buf_T *buf;		/* gdb buffer */
    buf_T *fr_buf;	/* frame sign buffer */
    buf_T *var_buf;	/* variables window buffer */
#else /* Clewn implementation */
    linenr_T lastline;	/* lastline being disassembled */
    int fr_buf;		/* frame sign buffer number */
    linenr_T lnum;	/* frame sign line number */
    int var_buf;	/* variables buffer number */
    char_u *var_name;	/* variables file name */
    char_u *balloon_txt;/* text over which is pointed the mouse */
    char * project_file;/* project file name */
    int project_state;	/* project file sourced state */
#endif
    oob_T oob;		/* out of band data */

    /* |+gdb| modes */
    int mode;		/* current |+gdb| mode */
#ifdef GDB_LVL2_SUPPORT
    lvl2_T lvl2;	/* annotations level 2 */
#endif
#ifdef GDB_LVL3_SUPPORT
    lvl3_T lvl3;	/* annotations level 3 and GDB/MI */
#endif

    /* pointers to mode specific functions */
    oobfunc_T *oobfunc;		    /* the current oob functions array */
    oobfunc_T *std_oobfunc;	    /* the standard oob functions array */
    int (*parse_output)(gdb_T *);   /* parser function */
    void (*gdb_docmd)(gdb_T *, char_u *);/* GDB command input function */
    void (*var_delete)(gdb_T *);    /* delete all variables function */
    void (*clear_gdb_T)(gdb_T *);   /* clear mode specific stuff */
};

/* Regexp patterns */
#define PAT_DIR		1
#define PAT_CHG_ANNO	2
#define PAT_ADD		3
#define PAT_PID		4

#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
# define PAT_SOURCE	101
# define PAT_QUERY	102
# define PAT_YES	103
# define PAT_SFILE	104
# define PAT_BP_CONT	105
# define PAT_ASM_FUNC	106
# define PAT_ASM_FUNC_P	107
# define PAT_FRAME	108
# define PAT_HEIGHT	109
#endif

#ifdef GDB_LVL2_SUPPORT
# define PAT_BP_ASM	201
# define PAT_BP_SOURCE	202
# define PAT_DISPLAY	203
# define PAT_DISPINFO	204
# define PAT_CREATEVAR	205
#endif

# ifdef GDB_LVL3_SUPPORT
# define PAT_CRVAR_FMT	301
# define PAT_INFO_FRAME	302
#endif

/* User interface */
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
void gdb_docmd_cli __ARGS((gdb_T *, char_u *));
void gdb_send_cmd __ARGS((gdb_T *, char_u *));
#endif

/* Vim low level hook */
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
int gdb_parse_output_cli __ARGS((gdb_T *));
#endif

/* Gdb process mgmt */
void gdb_close __ARGS((gdb_T *));
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
int gdb_setup_cli __ARGS((gdb_T *));
#endif

/* Gdb mode initialization */
#ifdef GDB_LVL2_SUPPORT
void gdb_lvl2_init __ARGS((gdb_T *));
#endif
#ifdef GDB_LVL3_SUPPORT
void gdb_lvl3_init __ARGS((gdb_T *));
#endif

/* Sign highliting */
#define FRAME_SIGN 1
#ifdef FEAT_GDB
# define PHANTOM_SIGN 2
# define BP_SIGN_ID(n) ((n)+2)	/* reserved sign ids: 1 frame, 2 phantom */
void gdb_fr_lite __ARGS((gdb_T *, buf_T *, linenr_T, struct obstack *));
int gdb_define_sign __ARGS((int, int));
void gdb_undefine_sign __ARGS((int));
buf_T * gdb_unlite __ARGS((int));
#else
# define BP_SIGN_ID(n) ((n)+1)	/* reserved sign id: 1 frame */
void gdb_fr_lite __ARGS((gdb_T *, int, linenr_T, struct obstack *));
int gdb_define_bpsign __ARGS((bpinfo_T *, struct obstack *));
void gdb_unlite __ARGS((int));
#endif
int gdb_as_frset __ARGS((gdb_T *, struct obstack *));
int gdb_fr_set __ARGS((gdb_T *, char_u *, linenr_T *, struct obstack *));
void gdb_fr_unlite __ARGS((gdb_T *));

/* Window and buffer mgmt */
#ifdef FEAT_GDB
win_T *gdb_btowin __ARGS((buf_T *));
void gdb_as_setname __ARGS((char_u *));
void gdb_clear_asmbuf __ARGS((gdb_T *, buf_T *));
win_T * gdb_edit_file __ARGS((gdb_T *, buf_T *, char_u *, linenr_T, struct obstack *));
void gdb_set_cursor __ARGS((win_T *, linenr_T));
void gdb_popup_console __ARGS((gdb_T *));
void gdb_redraw __ARGS((buf_T *));
#else
int gdb_edit_file __ARGS((int, char_u *, linenr_T, int, struct obstack *));
#endif
void gdb_msg_busy __ARGS((char_u *));
void gdb_showBalloon __ARGS((char_u *, struct obstack *));
void gdb_status __ARGS((gdb_T *, char_u *, struct obstack *));
void gdb_write_buf __ARGS((gdb_T *, char_u *, int));

/* Out Of Band */
void gdb_oob_send __ARGS((gdb_T *, struct obstack *));
void gdb_oob_receive __ARGS((gdb_T *, char_u *, struct obstack *));
#if defined(GDB_LVL2_SUPPORT) || defined(GDB_LVL3_SUPPORT)
char * gdb_print_value __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_get_pc __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_get_frame __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_info_frame __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_stack_frame __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_get_sfile __ARGS((gdb_T *, int, char_u *, struct obstack *));
#ifndef FEAT_GDB
char * gdb_source_project __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_get_pwd __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_get_args __ARGS((gdb_T *, int, char_u *, struct obstack *));
#endif
char * gdb_source_cur __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_source_list __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_get_sourcedir __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_get_asmfunc __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_get_asmfunc_hack __ARGS((gdb_T *, int, char_u *, struct obstack *));
char * gdb_get_asm __ARGS((gdb_T *, int, char_u *, struct obstack *));
void gdb_process_record __ARGS((gdb_T *, char_u *, char_u *, char_u *, char_u *, struct obstack *));
#endif

/* Variables window management */
#ifdef GDB_LVL2_SUPPORT
void gdb_process_display __ARGS((gdb_T *, char_u *, struct obstack *));
#endif

/* Utilities */
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef ABS
#define ABS(x) (((x)<0)?(-(x)):(x))
#endif
#define STRCHR(s,c) (char_u *)strchr((char *)(s),(int)(c))
#define STRSTR(h,n) (char_u *)strstr((char *)(h),(char *)(n))
#define FREE(p) {xfree((p)); (p) = NULL;}
#define IS_ANNOTATION(l) (STRSTR((l),"\032\032") == (l))
#define BUFLASTL(b) (!(((b)->b_ml.ml_flags) & ML_EMPTY)?((b)->b_ml.ml_line_count):0)
int gdb_read __ARGS((gdb_T *, char_u *, int, int));
void gdb_free_bplist __ARGS((bpinfo_T **));
void gdb_cmd_type __ARGS((gdb_T *, char_u *));
void gdb_cat __ARGS((char_u **, char_u *));
char_u * gdb_regexec __ARGS((char_u *, int, int, struct obstack *));
char_u * gdb_itoa __ARGS((int));

/* Gdb memory leaks mtrace */
#if defined(GDB_MTRACE) && defined(HAVE_MTRACE)
# include <malloc.h>
# include <mcheck.h>
/* storage for mtrace hooks */
extern __ptr_t (*s_malloc) (size_t, const void *);
extern void (*s_free) (void *, const void *);
extern __ptr_t (*s_realloc) (void *, size_t, const void *);

# define mv_hooks() do {	\
    s_malloc = __malloc_hook;	\
    s_free = __free_hook;	\
    s_realloc = __realloc_hook;	\
    __malloc_hook = NULL;	\
    __free_hook = NULL;		\
    __realloc_hook = NULL;	\
    } while (0)

# define get_hooks() do {	\
    __malloc_hook = s_malloc;	\
    __free_hook = s_free;	\
    __realloc_hook = s_realloc;	\
    } while (0)

/* we do call sometimes vim_free directly and allocation is not mtraced:
 * when the called Vim function does not free all its allocated memory
 * after it returns */
# define xmalloc(s) ({char_u *mret; get_hooks(); mret=xmalloc((s)); mv_hooks(); mret;})
# define xcalloc(s) ({char_u *mret; get_hooks(); mret=xcalloc((s)); mv_hooks(); mret;})
# define xrealloc(m,s) ({char_u *mret; get_hooks(); mret=xrealloc((m),(s)); mv_hooks(); mret;})

# define xfree(x) do {		\
    get_hooks();		\
    xfree((x));			\
    mv_hooks();			\
    } while (0)

# define clewn_strsave(s) ({char_u *mret; get_hooks(); mret=clewn_strsave((s)); mv_hooks(); mret;})
# define clewn_strnsave(s,l) ({char_u *mret; get_hooks(); mret=clewn_strnsave((s),(l)); mv_hooks(); mret;})

# define vim_strsave_escaped(s,e) ({char_u *mret; get_hooks(); mret=vim_strsave_escaped((s),(e)); mv_hooks(); mret;})
# define vim_regcomp(s,m) ({regprog_T *mret; get_hooks(); mret=vim_regcomp((s),(m)); mv_hooks(); mret;})
# define FullName_save(n,f) ({char_u *mret; get_hooks(); mret=FullName_save((n),(f)); mv_hooks(); mret;})
# define get_option_value(n,u,s,o) ({int r; get_hooks(); r=get_option_value((n),(u),(s),(o)); mv_hooks(); r;})
#endif	/* GDB_MTRACE */
#endif	/* GDB_H */

