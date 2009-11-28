# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([clewn],[1.15],[xdegaye@users.sourceforge.net])
AC_CONFIG_SRCDIR([clewn.h])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR(./scripts)
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug], [turn on debugging]),
[case "${enableval}" in
  yes) debug=true ;;
  no)  debug=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac],[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)

# vim runtime files directory
if test -z "$vimdir" ; then
    vimdir=/usr/local/share/vim/vimfiles
fi
AC_ARG_VAR(vimdir, [vim runtime files directory [/usr/local/share/vim/vimfiles]])

# Checks for programs.
AC_PROG_CC

# Checks for curses.
AC_CHECK_LIB(curses, initscr,,
	AC_MSG_ERROR(need curses library))

# Checks for readline.
AC_CHECK_HEADERS(readline/readline.h readline/history.h,,
	AC_MSG_ERROR(need readline include files))
AC_CHECK_LIB(readline, rl_initialize,,
	AC_MSG_ERROR(need readline library))

# Checks for readline rl_ding function.
AC_MSG_CHECKING(for rl_ding)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <readline/readline.h>
]], [[rl_ding()]])],[AC_MSG_RESULT(yes); AC_DEFINE(HAVE_RL_DING, 1, Define if we have the rl_ding function.)],[AC_MSG_RESULT(no)])

# Checks for readline rl_replace_line function.
AC_MSG_CHECKING(for rl_replace_line)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <readline/readline.h>
]], [[rl_replace_line("", 0)]])],[AC_MSG_RESULT(yes); AC_DEFINE(HAVE_RL_REPLACE_LINE, 1,
	    Define if we have the rl_replace_line function.)],[AC_MSG_RESULT(no)])

# Checks for readline rl_command_func_t typedef.
AC_MSG_CHECKING(for rl_command_func_t typedef)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <readline/readline.h>
]], [[rl_command_func_t undo]])],[AC_MSG_RESULT(yes); AC_DEFINE(HAVE_RL_COMMAND_FUNC_T, 1,
	    Define if we have the rl_command_func_t typedef.)],[AC_MSG_RESULT(no)])

# Checks for sockets.
AC_CHECK_FUNC(socket,,
	AC_MSG_ERROR(need sockets))
AC_CHECK_HEADERS(sys/socket.h netinet/in.h netdb.h arpa/inet.h,,
	AC_MSG_ERROR(missing socket include files))
# On Solaris we need the socket and nsl library.
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, gethostbyname)

# Checks for regexp.
AC_MSG_CHECKING(wether regexp can be used)
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <ctype.h>
	    #include <sys/types.h>
	    #include <regex.h>
	    main() { regex_t regexp; regmatch_t match[4];
		if (regcomp(&regexp,
		    "no match|^test\\\\+ (yes|no) *([^ ^(]+)(\\\\([0-9]+\\\\))*$",
		    REG_EXTENDED) == 0
			&& regexec(&regexp, "test+ yes func(34)", 4, match, 0) == 0
			&& match[3].rm_so != -1
			&& match[3].rm_eo != -1)
		    exit(0);
		exit(1); }]])],
		[AC_MSG_RESULT(yes)],
		[AC_MSG_ERROR(cannot use regexp)],
		[AC_MSG_RESULT(cross-compiling: assume we can)])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(libintl.h fcntl.h malloc.h strings.h sys/ioctl.h sys/time.h termio.h unistd.h)
AC_CHECK_HEADERS(string.h stddef.h stdlib.h unistd.h)
AC_CHECK_HEADERS(sys/select.h poll.h sys/poll.h)
AC_CHECK_HEADERS(stropts.h sys/stream.h sys/ptem.h termios.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_MODE_T
AC_FUNC_OBSTACK
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_PROG_GCC_TRADITIONAL
AC_HEADER_MAJOR
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(getcwd gethostbyname gettimeofday getwd mkdir rmdir select socket strerror strtol)
AC_CHECK_FUNCS(fchdir inet_ntoa memset nanosleep regcomp setenv strchr strrchr usleep getpseudotty mtrace)

# Check for memmove() before bcopy(), makes memmove() be used when both are
# present, fixes problem with incompatibility between Solaris 2.4 and 2.5.
AC_MSG_CHECKING(whether memmove/bcopy/memcpy handle overlaps)
[bcopy_test_prog='
main() {
  char buf[10];
  strcpy(buf, "abcdefghi");
  clewn_memmove(buf, buf + 2, 3);
  if (strncmp(buf, "ababcf", 6))
    exit(1);
  strcpy(buf, "abcdefghi");
  clewn_memmove(buf + 2, buf, 3);
  if (strncmp(buf, "cdedef", 6))
    exit(1);
  exit(0); /* libc version works properly.  */
}']

AC_RUN_IFELSE([AC_LANG_SOURCE([[#define clewn_memmove(s,d,l) memmove(d,s,l) $bcopy_test_prog]])],
    [AC_DEFINE(USEMEMMOVE, 1, Define if we can use memmove for overlapping copies) AC_MSG_RESULT(memmove does)],
    [AC_RUN_IFELSE([AC_LANG_SOURCE([[#define clewn_memmove(s,d,l) bcopy(s,d,l) $bcopy_test_prog]])],
	[AC_DEFINE(USEBCOPY, 1, Define if we can use bcopy for overlapping copies) AC_MSG_RESULT(bcopy does)],
	[AC_RUN_IFELSE([AC_LANG_SOURCE([[#define clewn_memmove(s,d,l) memcpy(d,s,l) $bcopy_test_prog]])],
	    [AC_DEFINE(USEMEMCPY, 1, Define if we can use memcpy for overlapping copies) AC_MSG_RESULT(memcpy does)],
	    [AC_MSG_ERROR(no)],
	    [AC_MSG_ERROR(failed to compile test program)])],
	[AC_MSG_ERROR(failed to compile test program)])],
    [AC_MSG_RESULT(cross-compiling: assume we can)])

# On some SCO machines sys/select redefines struct timeval
AC_MSG_CHECKING([whether sys/select.h and sys/time.h may both be included])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>]], [[]])],[AC_MSG_RESULT(yes)
    AC_DEFINE(SYS_SELECT_WITH_SYS_TIME, 1,
	Define if sys/select.h and sys/time.h may both be included.)],[AC_MSG_RESULT(no)])

# Check if strings.h and string.h can both be included when defined.
AC_MSG_CHECKING([if strings.h can be included after string.h])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(_AIX) && !defined(_AIX51) && !defined(_NO_PROTO)
# define _NO_PROTO	/* like in os_unix.h, causes conflict for AIX (Winn) */
			/* but don't do it on AIX 5.1 (Uribarri) */
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#if defined(HAVE_STRINGS_H)
# include <strings.h>
#endif
    ]], [[int i; i = 0;]])],[AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no); AC_DEFINE(NO_STRINGS_WITH_STRING_H, 1,
	Define if strings.h and string.h cannot both be included when defined.)])

# Checks for pty.c (copied from screen) ==========================
AC_MSG_CHECKING(for /dev/ptc)
if test -r /dev/ptc; then
  AC_DEFINE(HAVE_DEV_PTC, 1, Define if we have /dev/ptc)
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi

# Checking for SystemV pseudo ttys
AC_MSG_CHECKING(for SVR4 ptys)
if test -c /dev/ptmx ; then
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[ptsname(0);grantpt(0);unlockpt(0);]])],[AC_MSG_RESULT(yes); AC_DEFINE(HAVE_SVR4_PTYS, 1,
	    Define if we have SystemV pseudo ttys.)],[AC_MSG_RESULT(no)])
else
  AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING(for ptyranges)
if test -d /dev/ptym ; then
  pdir='/dev/ptym'
else
  pdir='/dev'
fi
# SCO uses ptyp%d
AC_EGREP_CPP(yes,
[#ifdef M_UNIX
   yes;
#endif
	], ptys=`echo /dev/ptyp??`, ptys=`echo $pdir/pty??`)
# if test -c /dev/ptyp19; then
# ptys=`echo /dev/ptyp??`
# else
# ptys=`echo $pdir/pty??`
# fi
if test "$ptys" != "$pdir/pty??" ; then
  p0=`echo $ptys | tr ' ' '\012' | sed -e 's/^.*\(.\).$/\1/g' | sort -u | tr -d '\012'`
  p1=`echo $ptys | tr ' ' '\012' | sed -e 's/^.*\(.\)$/\1/g'  | sort -u | tr -d '\012'`
  AC_DEFINE_UNQUOTED(PTYRANGE0, "$p0", Define pty ranges.)
  AC_DEFINE_UNQUOTED(PTYRANGE1, "$p1", Define pty ranges.)
  AC_MSG_RESULT([$p0 / $p1])
else
  AC_MSG_RESULT([don't know])
fi

# Special options used with gcc (taken from gnupg configure.ac).
if test "$GCC" = yes; then
    # Note that it is okay to use CFLAGS here because this are just
    # warning options and the user should have a chance of overriding
    #them.
    if test "$USE_MAINTAINER_MODE" = "yes"; then
	CFLAGS="$CFLAGS -Wall -Wextra -Wcast-align -Wshadow -Wstrict-prototypes"
    else
	CFLAGS="$CFLAGS -Wall"
    fi

    # This is handy for debugging so the compiler doesn't rearrange
    # things and eliminate variables.
    AC_ARG_ENABLE(optimization,
       AC_HELP_STRING([--disable-optimization],
		      [disable compiler optimization]),
		      [if test $enableval = no ; then
			 CFLAGS=`echo $CFLAGS | sed 's/-O[[0-9]]//'`
		       fi])
fi

AC_CONFIG_FILES([Makefile runtime/Makefile])
AC_OUTPUT
