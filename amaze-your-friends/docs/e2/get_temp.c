#include "e.h"

/* 
 * get_temp()
 *
 * Get ourselves a temporary file for the new history. 
 *
 */
void
get_temp()
{
    if (mktemp(tmp_file) != tmp_file){
        e_error("Could not mktemp.");
    }

    if ((tmp_fp = fopen(tmp_file, "w")) == NULL){
        e_error("Could not open temporary file '%s'.", tmp_file);
    }
    return;
}
