#include "e.h"

/*
 * Get the nth last filename from hlist. Then rebuild the history for
 * the directory, putting the requested filename last (most recently used).
 *
 */
void
nth_hist(n)
int n;
{
    if (n >= hist_count){
        if (hist_count > 1){
            e_error("Only %d history items exist.", hist_count);
        }
        else{
            e_error("Only one history item exists.");
        }
    }
    ok_sprintf(arg, "%s", hist[hist_count - n - 1]);
    reconstruct(hist_count - n - 1);
    return;
}
