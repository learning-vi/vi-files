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
 * $Id: netbeans.c 230 2009-11-28 16:50:31Z xavier $
 */

#include <config.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "obstack.h"
#include "clewn.h"
#include "gdb.h"
#include "misc.h"

#define NETBEANS_REQSTED_VERSION "2.1"
#define NETBEANS_SPECIALKEYS_VERSION "2.3"
#define NETBEANS_GETANNO_VERSION "2.4"
#define NETBEANS_CLOSE_BUGFIXED_VERSION "2.4"

#define BUFF_ALLOC_INCREMENT	    20
#define MAXMSGSIZE		    4096

/* NetBeans parsed events */
#define EVT_ERROR		-1
#define EVT_BALLOONEVAL		0
#define EVT_BALLOONTEXT		1
#define EVT_FILEOPENED		2
#define EVT_GEOMETRY		3
#define EVT_INSERT		4
#define EVT_KEYCOMMAND		5
#define EVT_KEYATPOS		6
#define EVT_KILLED		7
#define EVT_NEWDOTANDMARK	8
#define EVT_REMOVE		9
#define EVT_SAVE		10
#define EVT_STARTUPDONE		11
#define EVT_UNMODIFIED		12
#define EVT_VERSION		13

typedef struct cnbl_struct cnbl_T;

/*
 * The code handling the 'txt' content of a buffer assumes:
 *
 *	- not much editing is done by a Vim user on a file: a user
 *	  usually removes a line in the variables file (we may realloc
 *	  for just one character inserted, sending this char over TCP
 *	  is already using resources anyway)
 *	- a NetBeans 'insert' or 'remove' event is atomic and operates
 *	  on one line
 *	- a new line is inserted by Vim with an 'insert' event containing
 *	  only the new line character
 *
 * No effort is made to handle race conditions when "insert" or "remove"
 * events are sent by the editor while we are sending an "insert" or "remove"
 * function: the NetBeans protocol cannot guarantee that events are
 * not queued in the network, so this would be difficult to cleanly
 * manage anyway.
 */

/* NetBeans buffer states */
#define BUFS_INITDONE	0x01
#define BUFS_KILLED	0x02
#define BUFS_ASM	0x04

/* The Netbeans buffer structure */
typedef struct
{
    char *name;		/* buffer file name */
    int state;		/* buffer state */
    cnbl_T *txt;	/* buffer content: a list of cnbl_T lines */
    int lastsign;	/* last sign type number */
} cnbuf_T;

/* The line structure */
struct cnbl_struct
{
    char *line;		/* the line */
    int len;		/* line length */
    cnbl_T *next;	/* next line */
};

typedef struct
{
    char * gdb_path;
    char * vim_path;
} pathmap_T;

/* NetBeans states */
#define NBS_INIT	0x00
#define NBS_AUTH	0x01
#define NBS_VERSION	0x02
#define NBS_READY	0x04
#define NBS_DISCONN	0x08
#define NBS_CLOSING	0x10
#define NBS_SPECIALKEYS	0x20
#define NBS_GETANNO	0x40
#define NBS_CLOSE_FIX	0x80

/* The Clewn NetBeans structure */
typedef struct
{
    int instance;	/* gdb instance number */
    int state;		/* NetBeans state */
    char *passwd;	/* non allocated NetBeans password */
    int debug;		/* TRUE debugging info enabled */
    pathmap_T * remote_map; /* array of path mappings, when remote debugging */
    int fdcon;		/* socket over which the connection is accepted */
    int fdata;		/* socket over which NetBeans messages are exchanged */
    int seqno;		/* current command or function sequence number */
    int lastbuf;	/* last buffer number (buffer numbers start at 1) */
    char *line;		/* partial line received from NetBeans */
    cnbuf_T *cnbuf;	/* buffer list */
    int cnbuf_size;	/* buffer list current size */
    int fr_buf;		/* frame sign buffer number */
    int cur_buf;	/* current buffer number */
    int cur_line;	/* current buffer line number */

    /* the variables data */
    int * pvar_buf;	/* variables buffer number */
    int modified;	/* TRUE when var_buf is modified */
    int first_noaq;	/* first function seqno not acknowledged for var_buf */
    int last_noaq;	/* last function seqno not acknowledged for var_buf */
} cnb_T;

static cnb_T *cnb;		    /* the cnb_T instance */
static nb_event_T cnb_event;	    /* netbeans event */
static char cnb_buf[MAXMSGSIZE];    /* buffer for reading NetBeans messages */

/* Forward declarations */
static int parse_msg __ARGS((char *, struct obstack *));
static int conn_setup __ARGS((char *, struct obstack *));
static int get_evt __ARGS((char *, int *, char **));
static void send_cmd __ARGS((int, char *, char *));
static void send_function __ARGS((int, char *, char *));
static pathmap_T * pm_parse __ARGS((char *));
static char * pm_mapto_vim __ARGS((char *, char *, char *, char *, struct obstack *));
static char * pm_mapto_gdb __ARGS((char *, struct obstack *));
static char * unquote __ARGS((char *, char **, struct obstack *));
static char * quote __ARGS((char *, struct obstack *));
static cnbuf_T * lowlevel_buf __ARGS((int));
static cnbuf_T * getbuf __ARGS((int));
static int edit_file __ARGS((int, struct obstack *));
static cnbl_T ** line_get __ARGS((int, int, int *));
static void line_insert __ARGS((int, int, char *));
static void line_remove __ARGS((int, int, int));
static void line_debug __ARGS((int));

/*
 * Set socket fdcon to listen for connections.
 * When the variables file name is not NULL, create a buffer for this file
 * and set the buffer number pointed to by pvar_buf.
 * Return FAIL when error.
 */
    int
cnb_open(arg, pvar_buf, debug, reuse_addr, pathnames_map)
    char *arg;		/* netbeans connection parameters */
    int *pvar_buf;	/* pointer to buffer number of variables file name */
    int debug;		/* TRUE when debug mode */
    int reuse_addr;	/* TRUE when socket option SO_REUSEADDR */
    char * pathnames_map;/* pathnames_map when remote debugging */
{
    char *hostname;
    char *address;
    char *password;
    struct sockaddr_in local_add;
    struct hostent *host;
    int port;
    int flags;

    /* create the cnb instance */
    cnb = (cnb_T *)xcalloc(sizeof(cnb_T));
    cnb->fdcon	    = -1;
    cnb->fdata	    = -1;
    cnb->pvar_buf   = pvar_buf;
    cnb->debug	    = debug;
    cnb->remote_map = pm_parse(pathnames_map);

    /* arg is NULL or <host>:<addr>:<password> */
    hostname = arg;
    if (hostname == NULL || *hostname == '\0')
	hostname = getenv("__NETBEANS_HOST");
    if (hostname == NULL || *hostname == '\0')
	hostname = "localhost"; /* default */

    address = strchr(hostname, ':');
    if (address != NULL)
	*address++ = '\0';
    else
	address = getenv("__NETBEANS_SOCKET");
    if (address == NULL || *address == '\0')
	address = "3219";  /* default */

    password = strchr(address, ':');
    if (password != NULL)
	*password++ = '\0';
    else
	password = getenv("__NETBEANS_VIM_PASSWORD");
    if (password == NULL || *password == '\0')
	password = "changeme"; /* default */
    cnb->passwd = password;

    /* create the socket */
    if ((cnb->fdcon = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
	perror("socket() failed in cnb_listen()");
	return FAIL;
    }

    /* set this socket as non blocking */
    flags = fcntl(cnb->fdcon, F_GETFL, 0);
    if (flags != -1)
    {
	flags |= O_NONBLOCK;
	flags = fcntl(cnb->fdcon, F_SETFL, flags);
    }
    if (flags == -1)
    {
	perror("fcntl() failed in cnb_listen()");
	goto fail;
    }

    /* build the local address */
    memset((void *)&local_add, 0, sizeof(local_add));
    port = atoi(address);
    local_add.sin_family = AF_INET;
    local_add.sin_port = htons(port);
    if ((host = gethostbyname(hostname)) == NULL)
    {
	fprintf(stderr, "gethostbyname() failed in cnb_listen() with h_errno: %d\n", h_errno);
	goto fail;
    }
    memcpy((void *)&local_add.sin_addr, host->h_addr, (size_t)host->h_length);

    /* set socket option SO_REUSEADDR */
    if (reuse_addr)
    {
	int on = 1;
	setsockopt(cnb->fdcon, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    }

    /* bind the local address to the socket */
    if (bind(cnb->fdcon, (struct sockaddr *)&local_add, sizeof(local_add)) == -1)
    {
	perror("bind() failed in cnb_listen()");
	fprintf(stderr,"check the network\n");
	if (! reuse_addr)
	{
	    fprintf(stderr,"    possibly waiting for previous tcp connection to gracefully shutdown\n");
	    fprintf(stderr,"    next time: do not use -r option to avoid waiting\n");
	    fprintf(stderr,"    now: wait or use another port\n");
	}
	goto fail;
    }

    /* we are willing to accept connections on this socket */
    if (listen(cnb->fdcon, 5) == -1 )
    {
	perror("listen() failed in cnb_listen()");
	goto fail;
    }

    fprintf(stderr, "\nNetBeans listens on %s:%s\n\n", hostname, address);
    return OK;
fail:
    close(cnb->fdcon);
    cnb->fdcon = -1;
    fprintf(stderr, "hostname: %s, port: %s\n", hostname, address);
    return FAIL;
}

#define DETACH_MSG    "DETACH\n"
/* Close NetBeans sockets */
    void
cnb_close()
{
    struct obstack obs;	/* use an obstack for temporary allocated memory */
    cnbuf_T * buf;
    cnbl_T *l;
    cnbl_T *next;
    pathmap_T * m;
    int i;

    if (cnb == NULL || (cnb->state & NBS_CLOSING))
	return;
    cnb->state |= NBS_CLOSING;

    (void)obstack_init(&obs);

    /* free the allocated cnb_event fields */
    FREE(cnb_event.key);
    FREE(cnb_event.lnum);
    FREE(cnb_event.pathname);
    FREE(cnb_event.text);
    cnb_event.text_event = FALSE;
    cnb_event.seqno = 0;

    if (cnb->fdcon != -1)
    {
	close(cnb->fdcon);
	cnb->fdcon = -1;
    }
    if (cnb->fdata != -1)
    {
	fprintf(stderr, "NetBeans data socket disconnected\n");
	cnb_showBalloon("NetBeans connection to Clewn has been closed", FALSE, &obs);

	/* fussy: this write may be interrupted by a SIGPIPE which prevents
	 * the cnb structure to be freed since state is NBS_CLOSING
	 * and we never return from the write in this case */
	if (! (cnb->state & NBS_DISCONN))
	    write(cnb->fdata, DETACH_MSG, strlen(DETACH_MSG));
	shutdown(cnb->fdata, 2);	/* sends and receives are disallowed */
	close(cnb->fdata);
	cnb->fdata = -1;
    }

    /* free memory allocated by each buffer */
    for (i = 0; i < cnb->cnbuf_size; i++)
    {
	buf = cnb->cnbuf + i;
	xfree(buf->name);

	/* free all lines */
	for (l= buf->txt; l != NULL; l = next)
	{
	    next = l->next;
	    xfree(l->line);
	    xfree(l);
	}
    }

    obstack_free(&obs, NULL);

    /* free the pathmap array */
    for (m=cnb->remote_map; m; m++) {
	if (m->gdb_path == NULL)
	    break;
	xfree(m->gdb_path);
	xfree(m->vim_path);
    }
    FREE(cnb->remote_map);

    xfree(cnb->line);
    xfree(cnb->cnbuf);
    FREE(cnb);
}

/* Perform the equivalent of closing Vim: ":confirm qall" */
    void
cnb_saveAndExit()
{
    send_cmd(0, "raise", NULL);
    send_function(0, "saveAndExit", NULL);
}

/* Return TRUE when the data socket is opened */
    int
cnb_state()
{
    return (cnb != NULL && cnb->fdata != -1 && cnb->state & NBS_READY);
}

/* Return TRUE when we are running a netbeans version that supports specialKeys */
    int
cnb_specialKeys()
{
    return (cnb->state & NBS_SPECIALKEYS);
}

/* Map in vim a key with the corresponding type */
    void
cnb_keymap(type, key)
    int type;
    int key;
{
    char arg[] = "\" - \"";

    switch (type) {
	case KEYMAP_CONTROL:
	    arg[1] = 'C';
	    arg[3] = key;
	    break;
	case KEYMAP_UPPERCASE:
	    arg[1] = 'S';
	    arg[3] = key;
	    break;
	case KEYMAP_FUNCTION:
	    arg[1] = 'F';
	    if (key >= 10) {
		arg[2] = key / 10 + '0';
		arg[3] = key % 10 + '0';
	    }
	    else
		arg[2] = key % 10 + '0';
	    break;
	default:
	    return;
    }
    send_cmd(0, "specialKeys", arg);
}

/* Create or empty the variables buffer */
    void
cnb_create_varbuf(var_name)
    char *var_name;	/* variables file name */
{
    struct obstack obs;	/* use an obstack for temporary allocated memory */
    cnbuf_T *buf;

    if (cnb == NULL || cnb->pvar_buf == NULL || var_name == NULL)
	return;

    /* create or empty the variables buffer if not remote debugging */
    if (! cnb->remote_map) {
	if ((buf = getbuf(*cnb->pvar_buf)) != NULL) {	/* empty the buffer */
	    (void)obstack_init(&obs);
	    cnb_clear(*cnb->pvar_buf, &obs);
	    obstack_free(&obs, NULL);
	}
	else						/* create it */
	    *cnb->pvar_buf = cnb_create_buf(var_name);
    }
}

/* set the gdb instance number (used in cnb_define_sign) */
    void
cnb_set_instance(instance)
    int instance;
{
    if (cnb == NULL)
	return;

    cnb->instance = instance;
}

/* Return the socket over which the connection is accepted */
    int
cnb_get_connsock()
{
    if (cnb == NULL)
	return -1;
    return cnb->fdcon;
}

/* Return the socket over which NetBeans messages are exchanged */
    int
cnb_get_datasock()
{
    if (cnb == NULL)
	return -1;
    return cnb->fdata;
}

/* Handle an event on the connection socket */
    void
cnb_conn_evt()
{
    struct sockaddr_in from;
    char *remote;
    int port;
    socklen_t fromlen;
    int s;

    if (cnb == NULL || cnb->fdcon == -1)
	return;

    fromlen = (socklen_t) sizeof(from);

    /* accept the connection */
    if ((s = accept(cnb->fdcon, (struct sockaddr *)&from, &fromlen)) != -1)
    {
	remote = inet_ntoa(from.sin_addr);
	port = (int)ntohs(from.sin_port);

	if (cnb->fdata == -1)
	{
	    cnb->fdata = s;	/* data socket connected */
	    close(cnb->fdcon);
	    cnb->fdcon = -1;	/* connection socket not needed anymore */
	    fprintf(stderr, "NetBeans connected to %s:%d\n\n", remote, port);
	}
	else
	{
	    close(s);	/* reject connection, we already have a data socket */
	    fprintf(stderr, "connection attempt from %s:%d failed\n", remote, port);
	}
    }
    else if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
	perror("accept() failed in cnb_conn_evt()");
}

/*
 * Handle data received on the data socket.
 * Return a pointer to a static nb_event_T structure,
 * which is used when one of the NetBeans messages
 * in the data received is EVT_KEYATPOS or EVT_BALLOONTEXT
 */
    nb_event_T *
cnb_data_evt()
{
    struct obstack obs;	/* use an obstack for temporary allocated memory */
    char *evt;
    char *start;
    char *end;
    int len;

    if (cnb == NULL || cnb->fdata == -1)
	return NULL;

    /* key has been consumed */
    FREE(cnb_event.key);

    if ((len = read(cnb->fdata, cnb_buf, MAXMSGSIZE - 1)) == 0)
    {
	fprintf(stderr, "disconnected by editor\n");
	cnb->state |= NBS_DISCONN;
	cnb_close();
	return NULL;
    }
    else if (len < 0 && errno != EINTR)
    {
	perror("read() failed in cnb_data_evt()");
	cnb_close();
	return NULL;
    }
    else if (len > 0)
    {
	cnb_buf[len] = NUL;
	if (cnb->debug)
	    fprintf(stderr, "%s", cnb_buf);

	(void)obstack_init(&obs);

	/* add to previous partial line */
	OBSTACK_STRCAT(&obs, cnb->line);
	obstack_strcat0(&obs, cnb_buf);
	evt = (char *)obstack_finish(&obs);

	/* handle multiple NetBeans messages */
	for (start = evt; *start; start = end)
	{
	    if ((end = strchr(start, (int)'\n')) != NULL)
	    {
		*end++ = NUL;
		if (parse_msg(start, &obs) == FAIL)
		{
		    FREE(cnb->line);
		    obstack_free(&obs, NULL);
		    cnb_close();
		    return NULL;
		}
	    }
	    else
		break;
	}

	if (*start != NUL)	    /* a partial line */
	{
	    xfree(cnb->line);
	    cnb->line = clewn_strsave(start);
	    obstack_free(&obs, NULL);

	    return &cnb_event;
	}
	FREE(cnb->line);
	obstack_free(&obs, NULL);
    }

    return &cnb_event;
}

/*
 * Parse a NetBeans event of the form: 'bufid:event=seqno args'
 * Accept also the authentication message: 'AUTH password'
 * Return FAIL when the error is fatal and requires closing the connection.
 */
    static int
parse_msg(event, obs)
    char *event;
    struct obstack *obs;
{
    cnbuf_T *buf;
    char *args;
    char *pathname;
    char *next;
    char *end;
    char *tmp;
    int bufno;
    int offset;

    /* parse NetBeans connection setup events */
    if (! (cnb->state & NBS_READY))
	return conn_setup(event, obs);

    /* NetBeans ready and running */
    cnb_event.seqno = (int)strtol(event, &tmp, 10);
    next = tmp;

    /*
     * A reply to a function.
     */
    if (next != event && (*next == ' ' || *next == NUL))
    {
	if (*next == ' ')
	    next++;
	FREE(cnb_event.text);
	cnb_event.text_event = FALSE;
	cnb_event.text = clewn_strsave(next);

	/* handle acknowledgements for var_buf */
	if ((buf = getbuf(*cnb->pvar_buf)) != NULL && cnb_event.seqno != 0
		&& cnb_event.seqno >= cnb->first_noaq && cnb_event.seqno <= cnb->last_noaq)
	{
	    if (*(next + 1) == '!')
	    {
		send_cmd(*cnb->pvar_buf, "stopDocumentListen", NULL);
		buf->state |= BUFS_KILLED;
		fprintf(stderr,
			"error updating variables, the buffer has been killed\n");
	    }

	    /* all functions acknowledged */
	    if (cnb_event.seqno == cnb->last_noaq)
	    {
		cnb->first_noaq = 0;
		cnb->last_noaq = 0;
	    }
	}
	return OK;
    }

    /*
     * An event.
     */
    switch (get_evt(event, &bufno, &args))
    {
	/*
	 * `fileOpened pathname open modified'
	 *	open	    boolean   'T' always
	 *	modified    boolean   'F' always
	 */
	case EVT_FILEOPENED:
	    if ((pathname = unquote(args, &next, obs)) != NULL)
	    {
		/* unvalidate balloon text */
		FREE(cnb_event.text);

		if (*next == ' ' && *(next + 1) == 'T'
			&& strcmp(pathname, "(null)") != 0)
		{
		    *next = NUL;    /* terminate quoted pathname */

		    /* create the buffer
		     * even when we already know it (this is historical as in previous
		     * versions of Vim, we got fileOpened only for buffers clewn created,
		     * it does not hurt anyway to record all buffers ever loaded by Vim
		     * and it is necessary now to learn about var_buf when doing remote
		     * debugging and var_buf is created remotely) */
		    if ((bufno = cnb_create_buf(pathname)) != -1)
		    {
			/* send to the editor the new bufID */
			send_cmd(bufno, "putBufferNumber", args);

			/* learn about var_buf when it is created remotely */
			if (*cnb->pvar_buf == -1 && (tmp=strstr(pathname, VARIABLES_FNAME)) != NULL
				&& strcmp(tmp, VARIABLES_FNAME) == 0)
			    *cnb->pvar_buf = bufno;

			/* keep listening on changes to var_buf */
			if (bufno != *cnb->pvar_buf)
			    send_cmd(bufno, "stopDocumentListen", NULL);

			/* define the frame sign */
			(void)cnb_define_sign(bufno, FRAME_SIGN, SIGN_ANY, obs);

			/* set buffer as initialized */
			if ((buf = getbuf(bufno)) != NULL)
			    buf->state |= BUFS_INITDONE;
		    }
		}
		return OK;
	    }

	    if (cnb->debug)
		fprintf(stderr, "parse error in EVT_FILEOPENED in parse_msg()\n");
	    break;

	    break;

	/*
	 * `balloonText text'
	 *
	 * Used when 'ballooneval' is set and the mouse pointer rests
	 * on some text for a moment
	 */
	case EVT_BALLOONTEXT:
	    FREE(cnb_event.text);
	    if ((args = unquote(args, &next, obs)) == NULL)
	    {
		if (cnb->debug)
		    fprintf(stderr, "parse error in EVT_BALLOONTEXT in parse_msg()\n");
	    }
	    else {
		cnb_event.text = clewn_strsave(args);
		cnb_event.text_event = TRUE;
	    }
	    break;

	/*
	 * `keyAtPos keyName offset lnum/col'
	 *
	 * Reports a special key being pressed with name "keyName"
	 * and also reports the line number and column of the cursor
	 */
	case EVT_KEYATPOS:
	    FREE(cnb_event.key);
	    if ((args = unquote(args, &next, obs)) != NULL)
	    {
		cnb_event.key = clewn_strsave(args);
		
		/* skip offset and parse line number */
		if (*next++ == ' ' && (next = strchr(next, ' ')) != NULL
			&& (end = strchr(++next, '/')) != NULL)
		{
		    *end = NUL;

		    /* get buffer name */
		    if ((buf = getbuf(bufno)) != NULL)
		    {
			xfree(cnb_event.lnum);
			cnb_event.lnum = clewn_strsave(next);

			if (cnb->debug)
			    fprintf(stderr, "vim source file name: \"%s\"\n", buf->name);

			xfree(cnb_event.pathname);
			if (cnb->remote_map)
			    cnb_event.pathname = pm_mapto_gdb(buf->name, obs);
			else
			    cnb_event.pathname = clewn_strsave(buf->name);

			return OK;
		    }
		}
		FREE(cnb_event.key);
	    }

	    if (cnb->debug)
		fprintf(stderr, "parse error in EVT_KEYATPOS in parse_msg()\n");
	    break;

	/* buffer is wiped out and can't be reused */
	case EVT_KILLED:
	    if ((buf = getbuf(bufno)) != NULL)
	    {
		buf->state |= BUFS_KILLED;
		FREE(buf->name);

		/* remove in bpinfo list the records corresponding to the signs in buf,
		 * the signs themselves are removed by Vim in free_buffer() */
		gdb_free_records(bufno);
	    }
	    break;

	/* Text "next" has been inserted in Vim at position "offset" */
	case EVT_INSERT:
	    offset = (int)strtol(args, &tmp, 10);
	    next = tmp;

	    if (next != args && *next++ == ' ' && *next == '"'
		    && (next = unquote(next, NULL, obs)) != NULL)
	    {
		line_insert(bufno, offset, next);
		if (cnb->debug)
		    line_debug(bufno);
	    }
	    else
	    {
		if (cnb->debug)
		    fprintf(stderr, "parse error in EVT_INSERT in parse_msg()\n");
	    }
	    break;

	/* Text was deleted in Vim at position "offset" with byte length "len" */
	case EVT_REMOVE:
	    offset = (int)strtol(args, &tmp, 10);
	    next = tmp;

	    if (next != args && *next++ == ' ')
	    {
		line_remove(bufno, offset, atoi(next));
		if (cnb->debug)
		    line_debug(bufno);
	    }
	    else
	    {
		if (cnb->debug)
		    fprintf(stderr, "parse error in EVT_REMOVE in parse_msg()\n");
	    }
	    break;

	case EVT_UNMODIFIED:
	    if (bufno == *cnb->pvar_buf)
		cnb->modified = FALSE;
	    break;

	/* ignored events */
	case EVT_KEYCOMMAND:	/* use instead EVT_KEYATPOS */
	case EVT_NEWDOTANDMARK:	/* use instead EVT_KEYATPOS */
	case EVT_GEOMETRY:
	case EVT_BALLOONEVAL:
	case EVT_SAVE:
	    break;

	default:
	    if (cnb->debug)
		fprintf(stderr, "discarded event in parse_msg()\n");
	    break;
    }
    return OK;
}

#define AUTH_MSG    "AUTH "
/* Parse messages received during connection setup
 * Return FAIL when the error is fatal and requires closing the connection.
 */
    static int
conn_setup(event, obs)
    char *event;
    struct obstack *obs;
{
    char *args;
    char *version;
    int bufno;

    /* password: no quotes are used! */
    if (strstr(event, AUTH_MSG) == event)
    {
	if (strcmp(event + strlen(AUTH_MSG), cnb->passwd) == 0)
	    cnb->state |= NBS_AUTH;
	else
	{
	    fprintf(stderr, "got invalid NetBeans password\n");
	    return FAIL;
	}
	return OK;
    }

    switch (get_evt(event, &bufno, &args))
    {
	/* version including the quotes */
	case EVT_VERSION:
	    if ((args = unquote(args, NULL, obs)) != NULL)
	    {
		version = clewn_stripwhite(args);
		if (strcmp(version, NETBEANS_REQSTED_VERSION) >= 0)
		{
		    cnb->state |= NBS_VERSION;
		    if (strcmp(version, NETBEANS_SPECIALKEYS_VERSION) >= 0)
			cnb->state |= NBS_SPECIALKEYS;
		    if (strcmp(version, NETBEANS_GETANNO_VERSION) >= 0)
			cnb->state |= NBS_GETANNO;
		    if (strcmp(version, NETBEANS_CLOSE_BUGFIXED_VERSION) >= 0)
			cnb->state |= NBS_CLOSE_FIX;

		    return OK;
		}
	    }
	    fprintf(stderr, "connection rejected: need NetBeans version greater than %s\n",
		    NETBEANS_REQSTED_VERSION);
	    return FAIL;

	/* editor has finished its startup work and is ready for editing files */
	case EVT_STARTUPDONE:
	    if (cnb->state & NBS_AUTH && cnb->state & NBS_VERSION)
	    {
		cnb->state |= NBS_READY;
		send_cmd(0, "setExitDelay", "0"); /* don't need an exit delay */

		if (cnb->debug)
		    fprintf(stderr, "NBS_READY in conn_setup()\n");
	    }
	    else
	    {
		fprintf(stderr, "connection rejected: missing password or version\n");
		return FAIL;
	    }
	    break;

	default:
	    if (cnb->debug)
		fprintf(stderr, "discarded event in conn_setup()\n");
	    break;
    }
    return OK;
}

/*
 * Parse a received event: 'bufid:event=seqno args'
 * seqno is checked for validity
 * *parg is set to NULL when there is no argument
 */
    static int
get_evt(event, pbuf, parg)
    char *event;	/* event to parse */
    int *pbuf;		/* pointer to a bufID */
    char **parg;	/* pointer to event arguments */
{
    int id = EVT_ERROR;	/* event id */
    char *name;		/* event name */
    char *ptr;
    char *tmp;
    int seqno;

    if (event != NULL && (name = strchr(event, ':')) != NULL)
    {
	*name++ = NUL;
	*pbuf = (int)strtol(event, &tmp, 10);
	ptr = tmp;

	/* check bufID validity */
	if (*ptr != NUL || *pbuf < 0 || *pbuf > cnb->lastbuf)
	{
	    if (cnb->debug)
		fprintf(stderr, "invalid bufID in get_evt()\n");
	    return EVT_ERROR;
	}

	if ((ptr = strchr(name, '=')) != NULL)
	{
	    *ptr++ = NUL;
	    seqno = atoi(ptr);

	    /* check seqno validity */
	    if (seqno != 0 && (seqno > cnb->seqno || seqno < 0))
	    {
		fprintf(stderr, "invalid seqno in get_evt()\n");
		return EVT_ERROR;
	    }

	    /* strip <SPACE> at arguments start */
	    if ((*parg = strchr(ptr, ' ')) != NULL)
		while (**parg == ' ')
		    (*parg)++;

	    /* get event id */
	    if (strcmp(name, "balloonText") == 0)
		id = EVT_BALLOONTEXT;
	    else if (strcmp(name, "fileOpened") == 0)
		id = EVT_FILEOPENED;
	    else if (strcmp(name, "balloonEval") == 0)
		id = EVT_BALLOONEVAL;
	    else if (strcmp(name, "geometry") == 0)
		id = EVT_GEOMETRY;
	    else if (strcmp(name, "insert") == 0)
		id = EVT_INSERT;
	    else if (strcmp(name, "keyCommand") == 0)
		id = EVT_KEYCOMMAND;
	    else if (strcmp(name, "keyAtPos") == 0)
		id = EVT_KEYATPOS;
	    else if (strcmp(name, "killed") == 0)
		id = EVT_KILLED;
	    else if (strcmp(name, "newDotAndMark") == 0)
		id = EVT_NEWDOTANDMARK;
	    else if (strcmp(name, "remove") == 0)
		id = EVT_REMOVE;
	    else if (strcmp(name, "save") == 0)
		id = EVT_SAVE;
	    else if (strcmp(name, "startupDone") == 0)
		id = EVT_STARTUPDONE;
	    else if (strcmp(name, "unmodified") == 0)
		id = EVT_UNMODIFIED;
	    else if (strcmp(name, "version") == 0)
		id = EVT_VERSION;
	    else
	    {
		if (cnb->debug)
		    fprintf(stderr, "unknown event in get_evt()\n");
	    }
	}
    }
    return id;
}

/*
 * Send a NetBeans command.
 * ATTENTION: do not use the general purpose array cnb_buf[] for 'arg'
 */
    static void
send_cmd(bufno, cmd, arg)
    int bufno;	/* buffer id, generic messages use a bufID of zero */
    char *cmd;	/* command */
    char *arg;	/* command arguments, may be NULL */
{
    if (cnb == NULL || cnb->fdata == -1 || ! (cnb->state & NBS_READY)
	    || cmd == NULL || *cmd == NUL)
	return;

    /* can't use allocated memory here as we may be called to "removeAnno"
     * when aborting after memory allocation failure */
    if (2 * NUMBUFLEN + strlen(cmd) + (arg != NULL ? (strlen(arg) + 1) : 0) + 3 + 1
	    > MAXMSGSIZE)
	return;

    if (arg != NULL)
	sprintf(cnb_buf, "%d:%s!%d %s\n", bufno, cmd, ++(cnb->seqno), arg);
    else
	sprintf(cnb_buf, "%d:%s!%d\n", bufno, cmd, ++(cnb->seqno));
    if (cnb->debug)
	fprintf(stderr, cnb_buf);

    (void)write(cnb->fdata, cnb_buf, strlen(cnb_buf));
}

/*
 * Send a NetBeans function.
 * ATTENTION: do not use the general purpose array cnb_buf[] for 'arg' 
 */
    static void
send_function(bufno, function, arg)
    int bufno;	    /* buffer id, generic messages use a bufID of zero */
    char *function; /* function */
    char *arg;	    /* command arguments, may be NULL */
{
    if (cnb == NULL || cnb->fdata == -1 || ! (cnb->state & NBS_READY)
	    || function == NULL || *function == NUL)
	return;

    if (2 * NUMBUFLEN + strlen(function) + (arg != NULL ? (strlen(arg) + 1) : 0) + 3 + 1
	    > MAXMSGSIZE)
	return;

    if (arg != NULL)
	sprintf(cnb_buf, "%d:%s/%d %s\n", bufno, function, ++(cnb->seqno), arg);
    else
	sprintf(cnb_buf, "%d:%s/%d\n", bufno, function, ++(cnb->seqno));
    if (cnb->debug)
	fprintf(stderr, cnb_buf);

    (void)write(cnb->fdata, cnb_buf, strlen(cnb_buf));

    /* handle acknowledgements for var_buf */
    if (bufno == *cnb->pvar_buf)
    {
	if (cnb->first_noaq == 0)
	    cnb->first_noaq = cnb->seqno;
	cnb->last_noaq = cnb->seqno;
    }
}

/*
 * Send a command or function to NetBeans.
 * The format of line is: `bufID {command|function} arg1 arg2 ...'
 * bufID is a number
 */
    void
cnb_send_debug(type, line)
    int type;
    char *line;
{
    char *token;
    char *args;
    char *tmp;
    int bufno;

    if (cnb == NULL || ! (cnb->state & NBS_READY))
	return;

    bufno = (int)strtol(line, &tmp, 10);
    token = tmp;

    if ((bufno != 0 && getbuf(bufno) == NULL) || *token != ' ')
    {
	fprintf(stderr, "invalid bufID in cnb_send_debug()\n");
	return;
    }

    while (isspace(*token))
	token++;

    if (*token == NUL)
    {
	fprintf(stderr, "missing token in cnb_send_debug()\n");
	return;
    }

    args = strchr(token, ' ');
    if (args != NULL)
    {
	*args++ = NUL;
	while (isspace(*args))
	    args++;
	if (*args == NUL)
	    args = NULL;
    }

    if (type == DEBUG_ESC_CMD)		/* a command */
	send_cmd(bufno, token, args);
    else if (type == DEBUG_ESC_FUN)	/* a function */
	send_function(bufno, token, args);
}

#define MAX_BALLOON_SIZE 2000
/* Show a balloon (popup window) at the mouse pointer position, containing "text". */
    void
cnb_showBalloon(text, doquote, obs)
    char *text;
    struct obstack *obs;
    int doquote;
{
    char *quoted = NULL;
    char *wk_copy;

    if (text != NULL)
    {
	wk_copy = obstack_strsave(obs, text);

	if (strlen(wk_copy) > MAX_BALLOON_SIZE)
	    strcpy(wk_copy + MAX_BALLOON_SIZE - 3, "...");

	if (doquote)
	    quoted = quote(wk_copy, obs);
	else
	    quoted = wk_copy;

	/* Use bufID 1, even though it might not be defined or
	 * might be 'killed' */
	if (quoted != NULL)
	    send_cmd(1, "showBalloon", quoted);
    }
}

/* Do a startAtomic command. */
    void
cnb_startAtomic(bufno) int bufno; { send_cmd(bufno, "startAtomic", NULL); }

/* Do a endAtomic command. */
    void
cnb_endAtomic(bufno) int bufno; { send_cmd(bufno, "endAtomic", NULL); }

/*
 * Edit a file and set cursor at lnum.
 * Return the buffer number or -1 when error.
 */
    int
cnb_editFile(name, lnum, sourcedir, source_cur, source_list, silent, obs)
    char *name;		/* file name */
    linenr_T lnum;	/* line number */
    char *sourcedir;	/* GDB source directories */
    char *source_cur;	/* GDB current source */
    char *source_list;	/* GDB source list */
    int silent;
    struct obstack *obs;
{
    int bufno = -1;
    char *pathname;	/* file full path name */
    cnbuf_T *buf;
    char *str;
    char *quoted;

    if (cnb == NULL || ! (cnb->state & NBS_READY))
	return -1;

    if (name == NULL || *name == NUL || lnum <= 0)
	return -1;

    if (cnb->debug)
	fprintf(stderr, "gdb source file name: \"%s\"\n", name);

    if (cnb->remote_map)
	/* map source filenames when doing remote debugging */
	pathname = pm_mapto_vim(name, sourcedir, source_cur, source_list, obs);
    else
	/* get the first existing full path name in GDB source directories
	 * matching this name */
	pathname = get_fullpath(name, sourcedir, source_cur, source_list, obs);

    if (pathname == NULL)
    {
	if (! silent && strlen(name) < MAXMSGSIZE - 100)
	{
	    sprintf(cnb_buf,
		    "Clewn cannot find file \"%s\" in GDB source directories\n", name);
	    fprintf(stderr, cnb_buf);
	}
	return -1;
    }

    /* create a new buffer if this name does not refer to an existing buffer */
    if ((bufno = cnb_lookup_buf(pathname)) == -1)
	bufno = cnb_create_buf(pathname);

    if ((buf = getbuf(bufno)) != NULL)
    {
	/* initialize the buffer */
	if (! (buf->state & BUFS_INITDONE))
	{
	    if ((quoted = quote(pathname, obs)) != NULL)
	    {
		send_cmd(bufno, "editFile", quoted);

		/* send to the editor the new bufID */
		send_cmd(bufno, "stopDocumentListen", NULL);

		/* define the frame sign */
		(void)cnb_define_sign(bufno, FRAME_SIGN, SIGN_ANY, obs);

		buf->state |= BUFS_INITDONE;
	    }
	    else
		return -1;
	}

	sprintf(cnb_buf, "%ld/0", (long)lnum);

	/* set the cursor position to "lnum/0" */
	str = obstack_strsave(obs, cnb_buf);
	send_cmd(bufno, "setDot", str);
	send_cmd(bufno, "setVisible", "T"); /* need this to update Vim cmd line */
    }
    else
	bufno = -1;

    return bufno;
}

#define COLOR_BLUE	802287
#define COLOR_GREEN	4190027
#define COLOR_ORANGE	15710005
/*
 * Define a sign for this buffer according to its type.
 * Vim's NetBeans will not define duplicate signs, so it is safe
 * to call this function more than once with the same id.
 * Vim's NetBeans creates a new hilite group for each new defined sign.
 * As these are never removed (there is no such msg in NetBeans), this
 * is somehow a waste of memory.
 * Return the sequence sign type number in this buffer or -1 if error.
 */
    int
cnb_define_sign(bufno, id, type, obs)
    int bufno;	    /* buffer number where this sign is defined */
    int id;	    /* sign id (1:frame or breakpoint number) */
    int type;	    /* sign type (frame or enabled/disabled bp) */
    struct obstack *obs;
{
    char *prefix   = NULL;	/* keep compiler happy */
    char *typeName;		/* sign name (Vim use it to build the hilite group) */
    char *glyphFile;		/* 2 characters text */
    int fg = 0;			/* foreground color: black */
    int bg = 0;			/* background color; set to 0 to keep compiler happy */
    char text[NUMBUFLEN];
    char *arg;
    cnbuf_T *buf;
    int r;
    int typenr;

    if (cnb == NULL || ! (cnb->state & NBS_READY))
	return -1;

    if ((buf = getbuf(bufno)) == NULL || id <= 0)
	return -1;

    if (type == SIGN_BP_ENABLED || type == SIGN_BP_DISABLED)
    {
	if (type == SIGN_BP_ENABLED)
	{
	    prefix = "BP";
	    bg = COLOR_BLUE;
	}
	else if (type == SIGN_BP_DISABLED)
	{
	    prefix = "BPd";
	    bg = COLOR_GREEN;
	}

	/* build typeName: "BP[d]x_nnn"
	 * where:
	 *  x is the gdb instance number
	 *  nnn is the breakpoint number */
	OBSTACK_STRCAT(obs, prefix);
	strcpy(text, gdb_itoa(cnb->instance));
	obstack_strcat(obs, text);
	obstack_strcat(obs, "_");

	strcpy(text, gdb_itoa(id));
	obstack_strcat0(obs, text);
	typeName = (char *)obstack_finish(obs);

	/* the sign text is two chars max */
	if (id < 100)
	    strcpy(text, gdb_itoa(id));
	else
	{
	    if ((r = id % 100) < 10)
	    {
		text[0] = '0';
		strcpy(text + 1, gdb_itoa(r));
	    }
	    else
		strcpy(text, gdb_itoa(r));
	}
	glyphFile = text;
    }
    else    /* frame sign */
    {
	typeName = "1";
	glyphFile = "=>";
	bg = COLOR_ORANGE;
    }

    if (type == SIGN_BP_ENABLED || type == SIGN_BP_DISABLED)
	typenr = ++(buf->lastsign);
    else
    {
	typenr = 1;
	if (buf->lastsign == 0)
	    buf->lastsign = 1;
    }

    sprintf(cnb_buf, "%d \"%s\" \"\" \"%s\" %d %d",
	    typenr, typeName, glyphFile, fg, bg);

    arg = obstack_strsave(obs, cnb_buf);
    send_cmd(bufno, "defineAnnoType", arg);

    /*
     * Attempt to have Vim 6.2 corrupt memory bug (netbeans.c: 748:
     * buf_list[buf_list_used].fireChanges = 1;) write in the version text.
     * Otherwise, it may replace a quote with '#' as in:
     * "2:stopDocumentListen!3\n2:defineAnnoType!4 1 \"1\" \"\" \"=>\" 0 >15710005\n"
     * "2:stopDocumentListen!3\n2:defineAnnoType!4 1 \"1\" \"\" \"=># 0 >15710005\n"
     *
     * This may happen only for the first Vim instantiated netbeans buffer, so this
     * is only needed when bufno == 2 (bufno == 1 is the variables buffer) and when
     * setting the frame sign.
     */
    if (bufno <= 2 && id == FRAME_SIGN && type != SIGN_BP_ENABLED
	    && type != SIGN_BP_DISABLED)
    {
	send_cmd(bufno, "version",
	    "\"Clewn version x.x                                           \
									   \
									  .\"");
    }

    return typenr;
}

/* Add a sign in a buffer. */
    void
cnb_buf_addsign(bufno, id, typenr, lnum, obs)
    int bufno;	    /* buffer number */
    int id;	    /* sign id (1:frame or BP_SIGN_ID(bp_id)) */
    int typenr;	    /* sequence number: nth sign type number defined in this buffer */
    linenr_T lnum;  /* line number */
    struct obstack *obs;
{
    char *arg;

    if (cnb == NULL || ! (cnb->state & NBS_READY))
	return;

    if (getbuf(bufno) == NULL || id <= 0 || typenr <= 0 || lnum <= 0)
	return;

    sprintf(cnb_buf, "%d %d %ld/0", id, typenr, (long)lnum);

    arg = obstack_strsave(obs, cnb_buf);
    send_cmd(bufno, "addAnno", arg);

    sprintf(cnb_buf, "%ld/0", (long)lnum);

    arg = obstack_strsave(obs, cnb_buf);
    send_cmd(bufno, "setDot", arg);
    send_cmd(bufno, "setVisible", "T"); /* need this to update Vim cmd line */

    if (id == FRAME_SIGN)
	cnb->fr_buf = bufno;

    /* note the last position */
    cnb->cur_buf = bufno;
    cnb->cur_line = (int)lnum;
}

#define GETANNO_TIMEOUT 600
/* Return the sign line number.
 * return 0 when error (0 is an illegal line number) */
    int
cnb_buf_getsign(bufno, id)
    int bufno;	    /* buffer number */
    int id;	    /* sign id (1:frame or BP_SIGN_ID(bp_id)) */
{
    int wtime = GETANNO_TIMEOUT;    /* msecs */
    char arg[NUMBUFLEN];
    int lnum;
    int seqno;
    int rc;
#ifndef HAVE_SELECT
    struct pollfd fds;
#else
    struct timeval tv;
    struct timeval start_tv;
    fd_set rfds;
# ifdef HAVE_GETTIMEOFDAY
    gettimeofday(&start_tv, NULL);
# endif
#endif

    /* require a recent version of the netbeans protocol */
    if (cnb == NULL || cnb->fdata == -1 || ! (cnb->state & NBS_READY) || ! (cnb->state & NBS_GETANNO))
	return 0;

    if (getbuf(bufno) == NULL || id == FRAME_SIGN || id <= 0)
	return 0;

    /* send the nebeans function */
    sprintf(arg, "%d", id);
    send_function(bufno, "getAnno", arg);
    seqno = cnb->seqno;

    /* listen for the reply to the function */
    lnum = 0;
    while (wtime > 0) {
# ifndef HAVE_SELECT
	fds.fd = cnb->fdata;
	fds.events = POLLIN;

	rc = poll(&fds, 1, wtime);
# else
	FD_ZERO(&rfds);
	FD_SET(cnb->fdata, &rfds);

	tv.tv_sec = 0;
	tv.tv_usec = wtime * 1000;

	rc = select(cnb->fdata + 1, &rfds, NULL, NULL, &tv);
# endif

	if ((rc == -1 && errno != EINTR) || rc == 0)
	    break;
	else if (rc > 0 && cnb_data_evt() != NULL && cnb_event.seqno == seqno) {
	    if(cnb_event.text != NULL)
		lnum = atoi(cnb_event.text);
	    break;
	}

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

    return lnum;
}

/* Delete a sign in a buffer. */
    void
cnb_buf_delsign(bufno, id)
    int bufno;	    /* buffer number */
    int id;	    /* sign id (1:frame or BP_SIGN_ID(bp_id)) */
{
    char arg[NUMBUFLEN];

    if (cnb == NULL || ! (cnb->state & NBS_READY))
	return;

    if (id == FRAME_SIGN)
    {
	bufno = cnb->fr_buf;
	cnb->fr_buf = -1;
    }

    if (getbuf(bufno) == NULL || id <= 0)
	return;

    sprintf(arg, "%d", id);
    send_cmd(bufno, "removeAnno", arg);
}

#define PATHMAP_PATH_SEPARATOR ':'
#define PATHMAP_MAP_SEPARATOR '|'
/* Return an array of path mappings.
 * The array is built from the the pathnames_map string:
 * "gdb_path:vim_path|..." */
    static pathmap_T *
pm_parse(pathnames_map)
    char * pathnames_map;
{
    pathmap_T * pathmap;
    pathmap_T * m;
    char * map_list;
    char * ptr;
    char * end;
    char * map;
    int count;

    if (pathnames_map == NULL)
	return NULL;

    /* count how many mappings */
    for (count=1, ptr=pathnames_map; *ptr; ptr++)
	if (*ptr == PATHMAP_MAP_SEPARATOR)
	    count++;

    /* allocate the array */
    if ((pathmap = (pathmap_T *) xcalloc((size_t)((count + 1) * sizeof(pathmap_T)))) == NULL)
	return NULL;

    /* make a copy so we can mess with it */
    map_list = clewn_strsave(pathnames_map);

    /* build the array */
    for (m=pathmap, ptr=map_list; ; m++) {
	map = end = NULL;
	if ((end=strchr(ptr, PATHMAP_MAP_SEPARATOR)) != NULL)
	    *end = NUL;
	if ((map=strchr(ptr, PATHMAP_PATH_SEPARATOR)) != NULL)
	    *map = NUL;

	m->gdb_path = clewn_strsave(ptr);
	if (map != NULL)
	    m->vim_path = clewn_strsave(map + 1);
	else
	    m->vim_path = clewn_strsave("");

	if (end == NULL)
	    break;
	ptr = end + 1;
    }

    xfree(map_list);
    return pathmap;
}

/*
 * For each mapping in the remote_map array, if gdb_path is a prefix
 * or is the empty string, then replace it with vim_path
 */
    static char *
pm_mapto_vim(name, sourcedir, source_cur, source_list, obs)
    char *name;		/* file name */
    char *sourcedir;	/* GDB source directories */
    char *source_cur;	/* GDB current source */
    char *source_list;	/* GDB source list */
    struct obstack *obs;
{
    char *pathname = name;
    char *ptr = sourcedir;
    pathmap_T * m;
    char *next;
    char *hay;
    char *found;
    char *end;

    /* first, get the full path name from gdb when the gdb variable 'directories'
     * contains the compilation directory "$cdir" */
    do {
	if (sourcedir == NULL)
	    break;

	if ((next = strchr(ptr, ':')) != NULL)
	    *next++ = NUL;

	/* compilation directory */
	if (strcmp(ptr, GDB_CDIR) == 0) {
	    /* hay: file="NAME",fullname=" */
	    obstack_strcat(obs, "file=\"");
	    OBSTACK_STRCAT(obs, name);
	    obstack_strcat0(obs, "\",fullname=\"");
	    hay = (char *)obstack_finish(obs);

	    /* name is the current sourcefile: use gdb compilation directory */
	    if (source_cur != NULL && (found=strstr(source_cur, hay)) != NULL ){
		found += strlen(hay);
		if ((end=strstr(found, "\"")) != NULL) {
		    pathname = (char *)obstack_copy0(obs, found, end - found);
		    break;
		}
	    }

	    /* lookup for first occurence of name in source list */
	    if (source_list != NULL && (found=strstr(source_list, hay)) != NULL ){
		found += strlen(hay);
		if ((end=strstr(found, "\"")) != NULL)
		    pathname = (char *)obstack_copy0(obs, found, end - found);
	    }

	    break;
	}

	ptr = next;

    } while (ptr != NULL && *ptr != NUL);

    /* map the path name */
    for (m=cnb->remote_map; m != NULL && m->gdb_path != NULL; m++) {
	if ((ptr=strstr(pathname, m->gdb_path)) == pathname
		|| ((ptr=pathname) && *(m->gdb_path) == NUL))
	{
	    ptr += strlen(m->gdb_path);
	    OBSTACK_STRCAT(obs, m->vim_path);
	    OBSTACK_STRCAT0(obs, ptr);
	    return (char *)obstack_finish(obs);
	}
    }

    return pathname;
}

/*
 * For each mapping in the remote_map array, if vim_path is a prefix
 * or is the empty string, then replace it with gdb_path
 * Return an allocated string
 */
    static char *
pm_mapto_gdb(name, obs)
    char *name;	/* file name */
    struct obstack *obs;
{
    pathmap_T * m;
    char *ptr;

    for (m=cnb->remote_map; m != NULL && m->gdb_path != NULL; m++) {
	if ((ptr=strstr(name, m->vim_path)) == name || ((ptr=name) && *(m->vim_path) == NUL)) {
	    ptr += strlen(m->vim_path);
	    OBSTACK_STRCAT(obs, m->gdb_path);
	    OBSTACK_STRCAT0(obs, ptr);
	    return clewn_strsave(obstack_finish(obs));
	}
    }

    return clewn_strsave(name);
}

/*
 * Remove double quotes and convert backslashed chars.
 * When parameter end is not NULL, the address it is pointing to
 * is set to the first char after end of quoted string
 * Return an allocated string.
 */
    static char *
unquote(str, end, obs)
    char *str;	/* quoted string */
    char **end;	/* set to first char after end of quoted string */
    struct obstack *obs;
{
    if (str == NULL || *str != '"')
	return NULL;

    while (*++str != NUL)
    {
	switch (*str)
	{
	    /* matching quote */
	    case '"':
		obstack_1grow(obs, NUL);

		/* tag first char after end of quoted string */
		if (end != NULL)
		    *end = str + 1;
		return (char *)obstack_finish(obs);

	    case '\\':
		/* remove escape char and illegal escape sequences */
		switch (*++str)
		{
		    case '\\':
			obstack_1grow(obs, '\\');
			break;
		    case 'n':
			obstack_1grow(obs, '\n');
			break;
		    case 't':
			obstack_1grow(obs, '\t');
			break;
		    case 'r':
			obstack_1grow(obs, '\r');
			break;
		    case '"':
			obstack_1grow(obs, '"');
			break;
		    case NUL:
			return NULL;
		    default:
			break;
		}
		break;

	    default:
		obstack_1grow(obs, *str);
	}
    }

    /* failed to get matching quote */
    return NULL;
}

/*
 * Quote the given string and escape newline, '\r', backslash, double quote
 * characters inside the string.
 * Return an allocated string.
 */
    static char *
quote(str, obs)
    char *str;
    struct obstack *obs;
{
    char escaped;

    if (str != NULL)
    {
	obstack_1grow(obs, '"');

	for (; *str; str++)
	{
	    switch (*str)
	    {
		case '"':
		case '\\':
		    escaped = *str;
		    break;

		case '\n':
		    escaped = 'n';
		    break;

		case '\r':
		    escaped = 'r';
		    break;

		default:
		    escaped = NUL;
		    break;
	    }

	    if (escaped != NUL)
	    {
		obstack_1grow(obs, '\\');
		obstack_1grow(obs, escaped);
	    }
	    else
		obstack_1grow(obs, *str);
	}
	obstack_grow0(obs, "\"", 1);
	return (char *)obstack_finish(obs);
    }
    return NULL;
}

/*
 * This function may only be invoked by cnb_create_buf() and getbuf().
 * Find or create a buffer with the given number. Buffers are never
 * removed or freed. They are 'killed', which means they can't be used.
 * Buffer numbers start at 1 not 0.
 * Return this buffer or NULL if it has been 'killed' or when memory
 * allocation error or when this would require reallocating more than
 * BUFF_ALLOC_INCREMENT.
 */
    static cnbuf_T *
lowlevel_buf(bufno)
    int bufno;
{
    cnbuf_T * buf;

    if (cnb == NULL || bufno <= 0 || bufno > cnb->cnbuf_size + BUFF_ALLOC_INCREMENT)
	return NULL;

    /* grow buffer list*/
    if (bufno > cnb->cnbuf_size)
    {
	cnb->cnbuf = (cnbuf_T *)xrealloc(cnb->cnbuf,
			(cnb->cnbuf_size + BUFF_ALLOC_INCREMENT) * sizeof(cnbuf_T));

	memset(cnb->cnbuf + cnb->cnbuf_size, 0, BUFF_ALLOC_INCREMENT * sizeof(cnbuf_T));

	cnb->cnbuf_size += BUFF_ALLOC_INCREMENT;
    }

    buf = cnb->cnbuf + bufno - 1;

    /* a killed buffer can't be used anymore */
    if (buf->state & BUFS_KILLED)
	return NULL;

    return buf;
}

/*
 * Get buffer bufno.
 * Return the buffer or NULL if error.
 */
    static cnbuf_T *
getbuf(bufno)
    int bufno;
{
    if (cnb == NULL)
	return NULL;

    if (bufno > 0 && bufno <= cnb->lastbuf)
	return lowlevel_buf(bufno);
    return NULL;
}

/*
 * Create a new buffer.
 * Return the buffer number or -1 if error.
 */
    int
cnb_create_buf(name)
    char *name;		/* buffer name */
{
    cnbuf_T *buf;
    int bufno;
    int j;

    if (cnb == NULL || name == NULL)
	return -1;

    if (cnb->lastbuf > cnb->cnbuf_size)
    {
	if (cnb->debug)
	    fprintf(stderr, "last buffer out of bounds in cnb_create_buf()\n");
	return -1;
    }

    /* look first if the buffer already exists:
     * this handles the case where asm files have been unlinked
     * and their buffers are being reused again (after a "file"
     * GDB command for example) */
    for (j = 0; j < cnb->lastbuf; j++)
    {
	buf = cnb->cnbuf + j;

	if (! (buf->state & BUFS_KILLED) && buf->name != NULL
		&& strcmp(buf->name, name) == 0)
	    return j + 1;
    }

    bufno = cnb->lastbuf;

    /* create a new buffer */
    if ((buf = lowlevel_buf(++bufno)) != NULL)
    {
	buf->name = clewn_strsave(name);
	cnb->lastbuf = bufno;
	return cnb->lastbuf;
    }
    return -1;
}

/* Kill a buffer. */
    void
cnb_kill(bufno)
    int bufno;
{
    cnbuf_T *buf;

    if ((buf = getbuf(bufno)) != NULL)
    {
	buf->state |= BUFS_KILLED;
	FREE(buf->name);
    }
}

/* Set the buffer as an asm buffer. */
    void
cnb_set_asm(bufno)
    int bufno;
{
    cnbuf_T *buf;

    if ((buf = getbuf(bufno)) != NULL)
	buf->state |= BUFS_ASM;
}

/* Unlink all asm buffers. */
    void
cnb_unlink_asm()
{
    cnbuf_T *buf;
    int j;

    if (cnb == NULL)
	return;

    if (cnb->lastbuf > cnb->cnbuf_size)
    {
	if (cnb->debug)
	    fprintf(stderr, "last buffer out of bounds in cnb_killall_asm()\n");
	return;
    }

    for (j = 0; j < cnb->lastbuf; j++)
    {
	buf = cnb->cnbuf + j;

	if (! (buf->state & BUFS_KILLED) && (buf->state & BUFS_ASM) && buf->name != NULL) {
	    if (cnb->state & NBS_READY && cnb->state & NBS_CLOSE_FIX) {
		int idx;

		if ((idx = cnb_lookup_buf(buf->name)) != -1) {
		    send_cmd(idx, "close", NULL);
		    cnb_kill(idx);
		}
	    }

	    (void)unlink(buf->name);
	}
    }
}
/*
 * Look up for name as an existing buffer name, starting with the
 * last buffer that was looked up last time.
 * Return the buffer number or -1 when not found.
 */
    int
cnb_lookup_buf(name)
    char *name;			/* buffer file path name */
{
    static int last_used = 0;	/* last index used */
    cnbuf_T *buf;
    int i;
    int j;

    if (cnb == NULL)
	return -1;

    if (cnb->lastbuf > cnb->cnbuf_size)
    {
	if (cnb->debug)
	    fprintf(stderr, "last buffer out of bounds in cnb_lookup_buf()\n");
	return -1;
    }

    for (j = 0; j < cnb->lastbuf; j++)
    {
	i = (j + last_used) % cnb->lastbuf;
	buf = cnb->cnbuf + i;

	if (! (buf->state & BUFS_KILLED)
		&& buf->name != NULL && strcmp(buf->name , name) == 0)
	{
	    last_used = i;
	    return i + 1;	/* buffer numbers start at 1 */
	}
    }
    return -1;
}

/* Return buffer file name */
    char *
cnb_filename(bufno)
    int bufno;
{
    cnbuf_T *buf;

    if ((buf = getbuf(bufno)) != NULL && ! (buf->state & BUFS_KILLED))
	return buf->name;
    return NULL;
}

/* Return TRUE if this is a valid buffer number */
    int
cnb_isvalid_buffer(bufno)
    int bufno;
{
    return (getbuf(bufno) != NULL ? TRUE : FALSE);
}

/* Return TRUE when bufno is an out of bounds buffer */
    int
cnb_outofbounds(bufno)
    int bufno;
{
    return (bufno <= 0 || bufno > cnb->lastbuf);
}

/*
 * Append a line at the last before last line of buffer bufno.
 *
 * The NetBeans Vim code handling "insert" functions (v6.2) assumes:
 *	- text must contain a new line otherwise it's a partial line and
 *	  what becomes of partial lines is ... (we don't want to know)
 *	- whatever the offset, the text is inserted as a new line before
 *	  the existing line containing this offset
 *	- Vim crashes when refering to the offset after the last character
 *
 * Another problem stems from the fact that the line_insert() function we
 * must use to update the linked list of lines ignores a new line as the last
 * character of a line (it does that because it is needed when listening to
 * "insert" events).
 */
    void
cnb_append(bufno, line, obs)
    int bufno;	/* buffer number */
    char *line; /* line to append */
    struct obstack *obs;
{
    int offset  = 0;
    char *res;
    cnbuf_T *buf;
    cnbl_T *l;
    char *ptr;
    char *quoted;

    if (line == NULL || edit_file(bufno, obs) != OK)
	return;

    if (bufno == *cnb->pvar_buf && cnb->modified)
    {
	/* not needed any more
	 * this buffer is set "bufhidden=hide" by autocommands
	if (cnb->debug)
	    fprintf(stderr, "cannot append to modified variables buffer, you must save it\n");
	return;
	*/
    }

    if ((buf = getbuf(bufno)) == NULL)
	return;

    /* strip any new line */
    if ((ptr = strchr(line, '\n')) != NULL)
	*ptr = NUL;

    /* add the terminating new line */
    OBSTACK_STRCAT(obs, line);
    obstack_strcat0(obs, "\n");
    res = (char *)obstack_finish(obs);

    /* quote it */
    if ((quoted = quote(res, obs)) == NULL)
	return;

    /* get offset of start of last line */
    for (l = buf->txt; l != NULL; )
    {
	if (l->next == NULL)
	    break;

	offset += l->len;
	offset++;		/* count the new line */
	l = l->next;
    }

    sprintf(cnb_buf, "%d ", offset);
    obstack_strcat(obs, cnb_buf);
    OBSTACK_STRCAT0(obs, quoted);
    res = (char *)obstack_finish(obs);

    /* insert line in linked list */
    if (l != NULL && l->len > 0)
    {
	if (offset == 0)
	{
	    line_insert(bufno, l->len + 1, "\n");
	    line_insert(bufno, l->len + 1, line);
	}
	else
	{
	    line_insert(bufno, offset, line);
	    line_insert(bufno, offset + strlen(line), "\n");
	}
    }
    else
    {
	line_insert(bufno, offset, line);
	line_insert(bufno, offset + strlen(line) + 1, "\n");
    }

    /* restore state */
    if (bufno == *cnb->pvar_buf)
	cnb->modified = FALSE;

    if (cnb->debug)
	line_debug(bufno);

    /* send "insert" function */
    send_function(bufno, "insert", res);
}

/* Clear a buffer of its content */
    void
cnb_clear(bufno, obs)
    int bufno;	    /* buffer number */
    struct obstack *obs;
{
    int count = 0;
    char *res;
    cnbuf_T *buf;
    cnbl_T *l;
    cnbl_T *next;

    if ((buf = getbuf(bufno)) == NULL || ! (buf->state & BUFS_INITDONE))
	return;

    /* count the file length */
    for (l = buf->txt; l != NULL; l = l->next)
    {
	count += l->len;
	count++;		/* count the new line */
    }

    /* the last line (when there is one) is empty and is not accounted for by vim */
    --count;
    if (count < 0)
	count = 0;

    /* free all lines */
    for (l= buf->txt; l != NULL; l = next)
    {
	next = l->next;
	xfree(l->line);
	xfree(l);
    }
    buf->txt = NULL;

    /* tell vim to remove all contents */
    if (count != 0) {
	sprintf(cnb_buf, "%d %d", 0, count);
	res = obstack_strsave(obs, cnb_buf);
	send_function(bufno, "remove", res);
    }

    /* set back cursor to last known position */
    if (bufno == *cnb->pvar_buf && getbuf(cnb->cur_buf) != NULL)
    {
	sprintf(cnb_buf, "%d/0", cnb->cur_line);

	res = obstack_strsave(obs, cnb_buf);
	send_cmd(cnb->cur_buf, "setDot", res);
	send_cmd(cnb->cur_buf, "setVisible", "T"); /* need this to update Vim cmd line */
    }
}

/* Replace with line in bufno at line lnum. */
    void
cnb_replace(bufno, line, lnum, obs)
    int bufno;	/* buffer number */
    char *line; /* line to append */
    int lnum;	/* line number */
    struct obstack *obs;
{
    int offset      = 0;
    char *oldline;
    char *twonl;
    char *res;
    cnbl_T *l;
    char *quoted;
    cnbuf_T *buf;
    char *ptr;
    int len;

    if (line == NULL || cnb == NULL || ! (cnb->state & NBS_READY))
	return;

    if (bufno == *cnb->pvar_buf && cnb->modified)
    {
	/* not needed any more
	 * this buffer is set "bufhidden=hide" by autocommands
	if (cnb->debug)
	    fprintf(stderr, "cannot replace line in modified variables buffer, you must save it\n");
	return;
	*/
    }

    if ((buf = getbuf(bufno)) == NULL || ! (buf->state & BUFS_INITDONE))
	return;

    /* strip any new line */
    if ((ptr = strchr(line, '\n')) != NULL)
	*ptr = NUL;

    /* add two terminating new lines */
    OBSTACK_STRCAT(obs, line);
    obstack_strcat0(obs, "\n\n");
    res = (char *)obstack_finish(obs);

    /* quote it */
    if ((quoted = quote(res, obs)) == NULL)
	return;

    /* get offset of start of line lnum */
    for (l = buf->txt; l != NULL && --lnum > 0; )
    {
	if (l->next == NULL)
	    break;

	offset += l->len;
	offset++;		/* count the new line */
	l = l->next;
    }

    if (l == NULL || lnum != 0)
    {
	if (cnb->debug)
	    fprintf(stderr, "invalid lnum in cnb_replace()\n");
	return;
    }
    len = l->len;

    sprintf(cnb_buf, "%d ", offset);
    obstack_strcat(obs, cnb_buf);
    OBSTACK_STRCAT0(obs, quoted);
    res = (char *)obstack_finish(obs);

    sprintf(cnb_buf, "%d %d", offset, len);
    oldline = obstack_strsave(obs, cnb_buf);

    sprintf(cnb_buf, "%d 2", offset + (int) strlen(line) + 1);
    twonl = obstack_strsave(obs, cnb_buf);

    /* replace line in linked list */
    if (len != 0)
	line_remove(bufno, offset, len);
    line_insert(bufno, offset, line);

    /* restore state */
    if (bufno == *cnb->pvar_buf)
	cnb->modified = FALSE;

    if (cnb->debug)
	line_debug(bufno);

    /* remove old line */
    if (len != 0)
	send_function(bufno, "remove", oldline);

    /* add new line */
    send_function(bufno, "insert", res);

    /* remove last two new line */
    send_function(bufno, "remove", twonl);

    /* set back cursor to last known position */
    if (bufno == *cnb->pvar_buf && getbuf(cnb->cur_buf) != NULL)
    {
	sprintf(cnb_buf, "%d/0", cnb->cur_line);

	res = obstack_strsave(obs, cnb_buf);
	send_cmd(cnb->cur_buf, "setDot", res);
	send_cmd(cnb->cur_buf, "setVisible", "T"); /* need this to update Vim cmd line */
    }
}

/*
 * Search for an object in the variables buffer.
 * When found, the address referenced by plnum contains the object line number
 * in the buffer.
 * Return the (non allocated) object line or NULL when not found.
 */
    char *
cnb_search_obj(object, plnum)
    char *object;	/* object name to search for */
    int *plnum;		/* found at line number */
{
    int lnum = 0;
    cnbuf_T *buf;
    cnbl_T *l;
    char *ptr;

    if (object == NULL || *object == NUL || cnb == NULL || ! (cnb->state & NBS_READY))
	return NULL;

    if ((buf = getbuf(*cnb->pvar_buf)) == NULL || ! (buf->state & BUFS_INITDONE))
	return NULL;

    /* get offset of start of line lnum */
    for (l = buf->txt; l != NULL; l = l->next)
    {
	lnum++;

	/* an object line matches the pattern: "^\s*object:"*/
	if ((ptr = l->line) != NULL)
	{
	    while (isspace(*ptr))
		ptr++;

	    if (strstr(ptr, object) == ptr && *(ptr + strlen(object)) == ':')
	    {
		*plnum = lnum;
		return l->line;	    /* found it */
	    }
	}
    }

    return NULL;
}

/*
 * Edit a file if not already done.
 * The buffer must have already been created.
 * Return FAIL when error.
 */
    static int
edit_file(bufno, obs)
    int bufno;	    /* buffer number */
    struct obstack *obs;
{
    cnbuf_T *buf;
    char *quoted;

    if (cnb == NULL || ! (cnb->state & NBS_READY))
	return FAIL;

    if ((buf = getbuf(bufno)) != NULL)
    {
	/* initialize the buffer */
	if (! (buf->state & BUFS_INITDONE))
	{
	    buf->txt = (cnbl_T *)xcalloc(sizeof(cnbl_T));

	    if (buf->name != NULL && (quoted = quote(buf->name, obs)) != NULL)
	    {
		send_cmd(bufno, "editFile", quoted);
		send_cmd(bufno, "startDocumentListen", NULL);

		buf->state |= BUFS_INITDONE;
	    }
	    else
		return FAIL;
	}
	return OK;
    }
    return FAIL;
}

/*
 * Return the address of the link where a line is referenced for buffer bufno and
 * offset. When offset cannot be found, return address of last NULL link.
 * Set address referenced by pstart to the offset of the first character in this line.
 * Return NULL when bufno refers to an invalid buffer.
 */
    static cnbl_T **
line_get(bufno, offset, pstart)
    int bufno;	    /* buffer number */
    int offset;
    int *pstart;
{
    int count = 0;
    cnbuf_T *buf;
    cnbl_T **pline;

    if ((buf = getbuf(bufno)) == NULL)
	return NULL;

    for (pline = &(buf->txt); *pline != NULL; pline = &((*pline)->next))
    {
	if (offset >= count && offset < count + (*pline)->len + 1)
	    break;

	count += (*pline)->len;
	count++;		/* count the new line */
    }

    *pstart = count;
    return pline;
}

/*
 * Insert txt at offset.
 * assumes new lines are standalone characters in txt
 */
    static void
line_insert(bufno, offset, txt)
    int bufno;	/* buffer number */
    int offset;	/* where to insert */
    char *txt;  /* text to insert, possibly a standalone '\n' */
{
    cnbl_T **pline;
    cnbl_T *l;
    char *end;
    char *p;
    int start;
    int len;
    int first;

    if (txt == NULL)
	return;

    if ((pline = line_get(bufno, offset, &start)) == NULL)
    {
	if (cnb->debug)
	    fprintf(stderr, "invalid buffer in line_insert()\n");
	return;
    }

    /*
     * Insert a new line
     */
    if (strchr(txt, '\n') != NULL)
    {
	if (strlen(txt) != 1)
	{
	    if (cnb->debug)
		fprintf(stderr, "error: a non standalone new line in line_insert()\n");
	    return;
	}

	/* insert a new line at end of buffer or at a start of line */
	if (*pline == NULL || offset == start)
	{
	    l = (cnbl_T *)xcalloc(sizeof(cnbl_T));

	    /* link it to the list */
	    l->next = *pline;
	    *pline = l;
	}
	else
	{
	    l = *pline;	    /* assert l != NULL */

	    l->len = (l->line != NULL ? strlen(l->line) : 0);
	    if ((first = offset - start) <= 0 || first > l->len)
	    {
		if (cnb->debug)
		    fprintf(stderr, "offset out of bounds in line_insert()\n");
		return;
	    }

	    /* new line at end of line */
	    if (offset == start + l->len)   /* first == l->len */
	    {
		if (l->next == NULL)
		{
		    /* ignore new line at last position in buffer */
		    return;
		}

		l = (cnbl_T *)xcalloc(sizeof(cnbl_T));

		/* link it to the list */
		l->next = (*pline)->next;
		(*pline)->next = l;
	    }
	    /* new line in the middle of a line */
	    else /* assert (l->len - first) > 0 */
	    {
		/* remove (l->len - first) last characters */
		end = clewn_strsave(l->line + first);
		line_remove(bufno, offset, (l->len - first));

		/* insert a new line at end */
		line_insert(bufno, offset, "\n");

		/* insert back (l->len - first) last characters */
		line_insert(bufno, offset + 1, end);

		xfree(end);
	    }
	}
    }

    /*
     * Insert txt
     */
    else if (*txt != NUL)
    {
	/* inserting at end */
	if ((l = *pline) == NULL)
	{
	    l = (cnbl_T *)xcalloc(sizeof(cnbl_T));
	    *pline = l;
	}

	len = strlen(txt);
	l->len = (l->line != NULL ? strlen(l->line) : 0);

	if (l->line == NULL)
	{
	    l->line = clewn_strsave(txt);
	    l->len = len;
	}

	else if ((first = offset - start) < 0 || first > l->len)
	{
	    if (cnb->debug)
		fprintf(stderr, "offset out of bounds in line_insert()\n");
	    return;
	}

	else
	{
	    p = (char *)xmalloc(l->len + len + 1);
	    strncpy(p, l->line, first);
	    strcpy(p + first, txt);
	    strcpy(p + first + len, l->line + first);

	    xfree(l->line);
	    l->line = p;
	    l->len += len;
	}
    }

    if (bufno == *cnb->pvar_buf)
	cnb->modified = TRUE;
}

/* Remove nb characters at offset. */
    static void
line_remove(bufno, offset, nb)
    int bufno;	    /* buffer number */
    int offset;	    /* where to insert */
    int nb;	    /* number of characters to remove */
{
    cnbl_T **pline;
    cnbl_T *l;
    char *p;
    int start;
    int first;
    int last;

    if (nb <= 0)
	return;

    if ((pline = line_get(bufno, offset, &start)) == NULL)
    {
	if (cnb->debug)
	    fprintf(stderr, "invalid buffer in line_remove()\n");
	return;
    }

    if ((l = *pline) == NULL)
    {
	if (cnb->debug)
	    fprintf(stderr, "cannot remove characters after buffer end in line_remove()\n");
	return;
    }

    l->len = (l->line != NULL ? strlen(l->line) : 0);
    if ((first = offset - start) < 0 || first > l->len)
    {
	if (cnb->debug)
	    fprintf(stderr, "offset out of bounds in line_remove()\n");
	return;
    }

    if ((last = l->len - first - nb) < - 1)
    {
	if (cnb->debug)
	    fprintf(stderr, "attempt to remove too many chars in line_remove()\n");
	return;
    }

    if (last == -1)		    /* remove the new line */
    {
	/* remove the whole line */
	if (first == 0)
	{
	    xfree(l->line);
	    *pline = l->next;
	    xfree(l);
	}
	else
	{
	    if (cnb->debug)
		fprintf(stderr, "cannot remove new line in non empty line in line_remove()\n");
	    return;
	}
    }
    else if (nb == l->len)	/* an empty line */
    {
	xfree(l->line);
	l->line = NULL;
	l->len = 0;
    }
    else
    {
	p = (char *)xmalloc(first + last + 1);
	strncpy(p, l->line, first);
	strcpy(p + first, l->line + first + nb);

	xfree(l->line);
	l->line = p;
	l->len = first + last;
    }

    if (bufno == *cnb->pvar_buf)
	cnb->modified = TRUE;
}

/* Print buffer to "documentListen.debug". */
    static void
line_debug(bufno)
    int bufno;	    /* buffer number */
{
    cnbuf_T *buf;
    cnbl_T *l;
    FILE *fd;

    if ((buf = getbuf(bufno)) == NULL
	    || (fd = fopen("documentListen.debug", "w+")) == NULL)
	return;

    for (l = buf->txt; l != NULL; l = l->next)
    {
	if (l->line != NULL)
	    fprintf(fd, l->line);
	fprintf(fd, "\n");
    }
    fclose(fd);
}

