/*
 * e.h 
 * version 1.3
 *
 * Terry Jones, Department of Computer Science, University of Waterloo
 *              Waterloo Ontario Canada N2L 3G1
 * {ihnp4,allegra,decvax,utzoo,utcsri,clyde}!watmath!watdragon!tcjones
 * tcjones@dragon.waterloo.{cdn,edu} tcjones@WATER.bitnet
 * tcjones%watdragon@waterloo.csnet 
 *
 */


#ifdef Bsd
#   include <stdio.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <signal.h>
#   include <sys/param.h>
#   include <pwd.h>
#   include <ctype.h>
#   include <sysexits.h>
#   include <sys/file.h>
#   include <sys/dir.h>
#   include <strings.h>
#   include <sys/ioctl.h>
#   define VI "/usr/ucb/vi"
    uid_t getuid();
    uid_t getgid();
#endif /* Bsd */


#ifdef Sun
#   include <stdio.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <signal.h>
#   include <sys/param.h>
#   include <pwd.h>
#   include <ctype.h>
#   include <sysexits.h>
#   include <sys/file.h>
#   include <sys/dir.h>
#   include <strings.h>
#   include <sys/time.h>
#   include <sys/vnode.h>
#   include <ufs/inode.h>
#   include <sgtty.h>
#   define VI "/usr/ucb/vi"
    extern char *sprintf();
#endif /* Sun */


#ifdef Sysv
#   include <stdio.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <signal.h>
#   include <sys/param.h>
#   include <pwd.h>
#   include <ctype.h>
#   include <sys/file.h>
#   include <termio.h>
#   include <string.h>
#   include <fcntl.h>
#   include <dirent.h>
#   include <sys/dir.h>
#   define index strchr
#   define rindex strrchr
#   define direct dirent
#   define MAXPATHLEN MAXNAMLEN
#   define ok_sprintf sprintf
#   define VI "/usr/bin/vi"
    extern struct passwd *getpwuid();
#endif /* Sysv */


#ifdef Ultrix
#   include <stdio.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <signal.h>
#   include <sys/param.h>
#   include <pwd.h>
#   include <ctype.h>
#   include <sysexits.h>
#   include <sys/file.h>
#   include <sys/dir.h>
#   include <strings.h>
#   include <sys/ioctl.h>
#   define VI "/usr/ucb/vi"
#endif /* Ultrix */


#ifdef Dynix
#   include <stdio.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <signal.h>
#   include <sys/param.h>
#   include <pwd.h>
#   include <ctype.h>
#   include <sys/dir.h>
#   include <sys/ioctl.h>
#   include <strings.h>
#   define VI "/usr/ucb/vi"
#endif /* Dynix */

#ifdef waterloo
#   include <stdlib.h>
#endif

/*
 * Other checks...
 *
 */

#ifndef IREAD
#   define IREAD    0400
#endif

#ifndef EX_IOERR
#   define EX_IOERR 1
#endif


/*
 * Things that aren't portability concerns.
 *
 */

#define DEFAULT_HIST    ".e"
#define HIST_LINES      9
#define HIST_CHARS      1024
#define E_PATH          "VIPATH"
#define E_HIST          "VIHIST"
#define E_INHERIT       "VIINHERIT"
#define E_SAFE_INHERIT  "VISAFEINHERIT"
#define E_PATH_LEN      2048
#define ARG_CHARS       4096
#define MAX_ARGS        100
#define O_READ          00004
#define G_READ          0004
#define TERM_RECORD     0
#define TERM_SET        1
#define TERM_RESET      2
#define VERSION         "1.3"

/*
 * STRUCT_ASST should be defined if your cc can handle structural assignments. 
 *
 * This is only used in the function terminal(). Leave STRUCT_ASST
 * defined and if it doesn't break you're ok. If it does, undefine it and
 * you'll definitely be ok (but things will run slower - even though you
 * wont notice it.) Words words words. Ho hum.
 *
 */

#define STRUCT_ASST


int char_in();
int check();
int clean_up();
int dir_check();
int match();
int read_hist();
int safety_first();
int sp_dist();
int spell_help();
void abandon();
void ask_hist();
void catch_signals();
void check_hist();
void dir_find();
void do_vi();
void e();
void e_error();
void find();
void find_hist();
void find_match();
void get_temp();
void inheritance();
void insert_cmd();
void multiple();
void new_vi();
void normal();
void nth_hist();
void ok_fprintf();
void ok_sprintf();
void reconstruct();
void terminal();


extern char *getenv();
extern char *mktemp();
extern char *sbrk();
#ifndef Sysv
    extern char *getwd();
    void ok_sprintf();
#endif

extern FILE *hist_fp;
extern FILE *tmp_fp;
extern char *hist[];
extern char *home;
extern char *myname;
extern char *saved_line;
extern char arg[];
extern char cwd[];
extern char ehist[];
extern char erase;
extern char tmp_file[];
extern int emode;
extern int hist_count;
extern int inherit;
extern int safe_inherit;
extern int uid;


/* 
 * Walk over white space. 
 *
 */
#define skip_white(c) while (*(c) == ' ' || *(c) == '\t') (c)++;

/* 
 * Walk over non-white characters. 
 *
 */
#define skip_to_white(c) while (*(c) && *(c) != ' ' && *(c) != '\t') (c)++;

/* 
 * Run down a string and zap the newline if we find one. 
 *
 */
#define zap_nl(c) while (*(c) && *(c) != '\n') (c)++; *(c) = '\0';
