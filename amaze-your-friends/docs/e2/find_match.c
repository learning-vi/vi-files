#include "e.h"


/*
 * find_match()
 *
 * Find the name in the history list that contains the 'pattern'.
 * If we find one, put it into the 'arg' variable. Otherwise
 * announce that a match couldn't be found and leave.
 *
 */
void
find_match(pattern)
char *pattern;
{
    register i;
	int match();

    /* 
     * Try for a match with each file in turn. Note that we are working
     * from most-recently-used backwards.
     *
     */

    for (i = hist_count - 1; i >= 0; i--){
        if (match(hist[i], pattern)){
            ok_sprintf(arg, "%s", hist[i]);
            reconstruct(i);
            return;
        }
    }

    /* 
     * We couldn't match so get out of here. 
     *
     */
    e_error("Unable to match with '%s'.", pattern);
}



/*
 * match()
 *
 * Boneheaded but easy pattern matcher. Just see if the 'pattern'
 * exists anywhere in the 'argument'. Boyer-Moore who?
 * In general our patterns will be so short that it wouldn't be
 * worth the effort to set up a better algorithm.
 *
 */
int
match(argument, pattern)
char *argument;
char *pattern;
{
    register length = strlen(pattern);

    while (strlen(argument) >= length){
        if (!strncmp(argument++, pattern, length)){
            return 1;
        }
    }
    return 0;
}
