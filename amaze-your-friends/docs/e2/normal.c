#include "e.h"

/* 
 * normal()
 *
 * A normal filename was found, put it into arg. First of all if there
 * is a history and this file is already in it (which means they could
 * have gotten to this file in other ways), then reconstruct the history
 * as though they had. Also offer spelling help.
 *
 */

void
normal(string)
char *string;
{
    ok_sprintf(arg, "%s", string);

    if (hist_count != -1){

        register int i;

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
         * It's not in the history, help with spelling then reconstruct. 
         *
         */
        if (!spell_help(0)){
            dir_find();
        }

        /* 
         * If it is in the history then reconstruct and return. 
         * (It may now be in the history even if it wasn't before - this
         * is because dir_find() or spell_help() may have done something to 
         * arg.)
         *
         */
        for (i = 0; i < hist_count; i++){
            if (!strcmp(hist[i], arg)){
                reconstruct(i);
                return;
            }
        }

        /*
         * Reconstruct and leave out the oldest if needed.
         *
         */
        if (hist_count == HIST_LINES){
            reconstruct(0);
        }
        else{
            reconstruct(-1);
        }
    }
    else{

        /* 
         * There is no history around so help with spelling and set up a 
         * history for next time.
         *
         */

        if (!spell_help(0)){
            dir_find();
        }
        new_vi();
    }
    return;
}

