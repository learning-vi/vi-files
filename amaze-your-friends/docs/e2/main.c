
/*******************************************************************************
#  E - command line preprocessor for vi.  Version 1.3 - November 1988.         #
#  ===================================================================         #
#                                                                              #
#  Terry Jones, Department of Computer Science, University of Waterloo         #
#  Waterloo, Ontario, Canada. N2L 3G1                                          #
#                                                                              #
#  {ihnp4,allegra,decvax,utzoo,utcsri,clyde}!watmath!watdragon!tcjones         #
#  tcjones@dragon.waterloo.{cdn,edu} tcjones@WATER.bitnet                      #
#  tcjones%watdragon@waterloo.csnet                                            #
*******************************************************************************/


#include "e.h"

FILE *hist_fp = NULL;        /* The original .e file                        */
FILE *tmp_fp = NULL;         /* The new .e file                             */
char *hist[HIST_LINES];      /* Pointers to history items.                  */
char *home;                  /* Home directory.                             */
char *myname;                /* argv[0]                                     */
char *saved_line = NULL;     /* In case we read one line too many later on. */
char arg[ARG_CHARS];         /* The arguments that vi will be invoked with. */
char cwd[MAXPATHLEN];        /* The directory from which we're invoked.     */
char ehist[MAXPATHLEN];      /* The name of the original .e file.           */
char erase;                  /* The terminal's erase character.             */
char tmp_file[MAXPATHLEN];   /* The name of the new .e file.                */
int emode;                   /* The protection mode of the original .e.     */
int hist_count;              /* The # of items in the history for this dir. */
int safe_inherit = 0;        /* Never inherit other people's .exrc's        */
int inherit = 0;             /* Inherit .exrc files?                        */
int uid;                     /* The user's uid.                             */


main(argc, argv)
int argc;
char **argv;
{
    /*
     * Do some preliminary things. Grab the name we were invoked with,
     * record the status of the terminal so we can restore it later if
     * we have to alter it for some reason, arrange to catch SIGINT and
     * read and split up the history for this directory.
     *
     * Then call e which handles the arguments and calls other things
     * to get the job done. e should never return.
     */

    myname = argv[0];
    terminal(TERM_RECORD);
    catch_signals();
    inheritance();
    find_hist();
    hist_count = read_hist();
    e(argc, argv);
    return 1;
}
