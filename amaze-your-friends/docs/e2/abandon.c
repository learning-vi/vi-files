#include "e.h"

/*
 * abandon()
 *
 * Close the old history and the new history files, then unlink the
 * new history. This is called when we wish to leave the program but are
 * not going to call reconstruct() to finish the building of the new history
 * for us. Typically we discover one way or another that the new history wasn't
 * needed (e.g. the user quits or the file requested was the most recently
 * used and so we don't have to update the LRU list.)
 *
 */
void
abandon()
{
    if (hist_fp && fclose(hist_fp) == EOF){
        (void)fprintf(stderr, "%s: Could not fclose '%s'\n", myname, ehist);
    }

    if (tmp_fp && fclose(tmp_fp) == EOF){
        (void)fprintf(stderr, "%s: Could not fclose '%s'\n", myname, tmp_file);
    }

    if (tmp_fp && unlink(tmp_file) == -1){
        (void)fprintf(stderr, "%s: Could not unlink '%s'\n", myname, tmp_file);
    }

    hist_fp = tmp_fp = NULL;
    return;
}

