#include "e.h"


/*
 * Question the user about which file from the history is wanted.
 *
 */
void
ask_hist()
{
    register int i;
    register int option;

    /* Print the history. */
    for (i = 0; i < hist_count; i++){
        ok_fprintf(stderr, "\t[%d]: %s\n", hist_count-i-1, hist[i]);
    }

    option = char_in("select -> ");

    /* 
     * Process the option and put the appropriate file name into the 
     * arg variable.
     *
     */

    if (option == '\n'){
        /* 
         * They want the last file of the list. 
         * There's no need to reconstruct, the history is already correct.
         *
         */
        ok_fprintf(stderr, "%s\n", hist[hist_count - 1]);
        ok_sprintf(arg, "%s", hist[hist_count - 1]);
        abandon();
        return;
    }
    else if (option == (int)erase){
        /* 
         * They want to leave. 
         *
         */
        ok_fprintf(stderr, "\n");
        abandon();
        exit(1);
    }
    else if (option >= '0' && option <= '0' + hist_count - 1){
        /* 
         * They have requested a file by its history number. 
         *
         */
        register int want = hist_count - (option - '0') - 1;
        ok_fprintf(stderr, "%s\n", hist[want]);
        ok_sprintf(arg, "%s", hist[want]);
        reconstruct(want);
        return;
    }
    else{
        /* 
         * Looks like they want to name a specific file. Echo the 
         * character back to the screen. Then get the rest of the filename.
         *
         */
        ok_fprintf(stderr, "%c", option);
        arg[0] = (char)option;
        i = 1;
        while ((arg[i] = (char)getc(stdin)) != '\n'){
            i++;
        }
        arg[i] = '\0';

        /* 
         * Seeing as they typed in the name, try and help with spelling. 
         * If you can't help with spelling, see if there is a file in a
         * directory mentioned in the VIPATH list that might have been the
         * file that was meant.
         *
         */
        if (!spell_help(0)){
            dir_find();
        }

        /* 
         * If it is in the history then reconstruct and return. 
         *
         */
        for (i = 0; i < hist_count; i++){
            if (!strcmp(hist[i], arg)){
                reconstruct(i);
                return;
            }
        }

        /* 
         * Otherwise reconstruct, leaving out the oldest name if needed.
         * reconstruct(-1) will exclude nothing - the history is not full.
         *
         */
        if (hist_count == HIST_LINES){
            reconstruct(0);
        }
        else{
            reconstruct(-1);
        }
    }
    return;
}
