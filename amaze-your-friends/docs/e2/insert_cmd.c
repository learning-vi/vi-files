#include "e.h"

/*
 * insert_cmd()
 *
 * They want the last file in the history but want to preceed it
 * this time with a command - no problems here.
 *
 */
void
insert_cmd(command)
char *command;
{
    char *place;

    /* 
     * If there was already a command there (indicated by a '+'), then we
     * want to get rid of it. If there is a '+' but no ' ' (somewhere) after 
     * it, then the history file is in disarray and we do not try to recover.
     *
     */

    if (*hist[hist_count - 1] == '+'){
        if ((place = index(hist[hist_count - 1], ' ')) == NULL){
            e_error("History '%s' corrupted, + but no following space", ehist);
        }
        skip_white(place);
    }
    else{
        place = hist[hist_count - 1];
    }

    /* 
     * Put the new command and the filename into 'arg' 
     *
     */
    ok_sprintf(arg, "%s %s", command, place);

    /* 
     * Reconstruct, leaving out the oldest name if needed.
     * reconstruct(-1) will exclude nothing - the history is not full.
     *
     */
    if (hist_count == HIST_LINES){
        reconstruct(0);
    }
    else{
        reconstruct(-1);
    }
    return;
}
