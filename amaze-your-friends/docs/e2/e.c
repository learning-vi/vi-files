#include "e.h"

void
e(c, v)
int c;
char **v;
{
    /*
     * Process the command line. This gets a little messy as there are so
     * many ways e can be invoked. They are listed below and there is an
     * example provided in each of the switch cases to illustrate the 
     * particular one we are trying to handle.
     *
     * The idea in most cases is to get the arguments that will be passed
     * to vi into a character array (arg), and pass it to do_vi(). do_vi()
     * splits up the arguments and execs vi. Occasionally it is simpler and
     * do_vi() can be called more directly.
     *
     *
     * Command Line Options.
     * =====================
     *
     * No arguments.
     *
     *     (1) "e"
     *
     * One argument.
     *
     *     (2) "e -"
     *     (3) "e -#"                # is the number of some history item.
     *     (4) "e -r"
     *     (5) "e -pat"              pat is a search pattern.
     *     (6) "e +100"
     *     (7) "e ."
     *     (8) "e <filename > "
     *
     * Multiple arguments.
     *
     *     (9) "e fred harry joe"   Also handles "e -t tag", "e -r file" etc.
     *
     */


    switch (c){
        case 1: {

            /* 
             * Command line option (1).
             * Example: "e"
             *
             * Just go and vi the last file that was e'ed.
             *
             */

            check_hist();
            abandon();
            do_vi(hist[hist_count - 1]);
            break;
        }
        
        case 2:{
            switch ((*++v)[0]){

                case '-':{

                    if ((c = (*v)[1]) == '\0'){

                        /* 
                         * Command line option (2).
                         * Example: "e -"
                         *
                         * This is a select from history, ask what they want.
                         *
                         */

                        check_hist();
                        ask_hist();
                        do_vi(arg);
                    }
                    else if (isdigit(c)){

                        /* 
                         * Command line option (3).
                         * Example: "e -3"
                         *
                         * Get the nth last file from the history and vi it.
                         *
                         */

                        check_hist();
                        nth_hist(c-'0');
                        do_vi(arg);
                    }
                    else if (c == 'r' && (*v)[2] == '\0'){

                        /* 
                         * Command line option (4).
                         * Example: "e -r"
                         *
                         * A recover, just pass it to vi and don't interfere.
                         *
                         */

                        do_vi(*v);
                    }
                    else{

                        /* 
                         * Command line option (5).
                         * Example: "e -pat"
                         *
                         * This is a pattern - try to match it.
                         *
                         */

                        check_hist();
                        find_match(++*v);
                        do_vi(arg);
                    }
                    break;
                }

                case '+':{

                    /* 
                     * Command line option (6).
                     * Example: "e +100"
                     *
                     * A command, put it before the last file name.
                     *
                     */

                    check_hist();
                    insert_cmd(*v);
                    do_vi(arg);
                    break;
                }

                case '.':{

                    /* 
                     * Command line option (7).
                     * Example: "e ."
                     * Example: "e .login"  (falls through to option (8)).
                     *
                     * Just give a history list if there is only a dot.
                     * Otherwise fall through as it must be a filename.
                     *
                     */

                    if ((*v)[1] == '\0'){
                        register i;

                        check_hist();
                        for (i = 0; i < hist_count; i++){
                            ok_fprintf(stderr, "\t[%d]: %s\n",
                                hist_count - i - 1, hist[i]);
                        }
                        abandon();
                    }
                    /* 
                     * The switch falls through in the case where there is a
                     * filename that starts with a period.
                     *
                     */

                }
                /* FALLTHROUGH */

                default :{

                    /* 
                     * Command line option (8).
                     * Example: "e fred"
                     * Example: "e .login"  (fell through from option (8)).
                     *
                     * Looks like it's just a plain old file name. vi it!
                     *
                     */

                    normal(*v);
                    do_vi(arg);
                    break;
                }
            }
            break;
        }

        default:{

            /* 
             * Command line option (9).
             * Example: "e fred harry joe"
             *
             * A bunch of arguments, fix the history & vi them all as normal.
             *
             */

            multiple(c, v, ARG_CHARS);
            do_vi(arg);
            break;
        }
    }
}
