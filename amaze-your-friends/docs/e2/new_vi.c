#include "e.h"

/* 
 * Attempt to make a new history file.
 *
 */
void
new_vi()
{
    FILE *vh;

    /* 
     * If we can't make it, get out. 
     *
     */
    if ((vh = fopen(ehist, "w")) == NULL){
        e_error("Could not open new history.");
    }

    /* 
     * Put in the directory and 'arg' that we will be vi'ing in a second. 
     *
     */
    ok_fprintf(vh, "%s\n\t%s\n", cwd, arg);

    /* 
     * Close the history. 
     *
     */
    if (fclose(vh) == EOF){
        e_error("Could not close '%s'.", ehist);
    }
    return;
}
