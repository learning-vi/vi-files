#include "e.h"

/*
 * check_hist()
 *
 * Make sure the history ok. This is called by things which are intending
 * toperform some history-relatd stuff after calling read_hist().
 *
 */
void
check_hist()
{
    if (hist_count == -1){
        e_error("Couldn't find %s history '%s'.", myname, ehist);
    }

    if (hist_count == 0){
        e_error("No history for '%s'.", cwd);
    }
}
