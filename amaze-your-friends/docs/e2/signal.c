#include "e.h"

/*
 * clean_up()
 *
 * This is where we come when we get a SIGINT.
 *
 */
int
clean_up()
{
    /*
     * Just get out after making sure things are tidy.
     *
     */
    e_error("\nInterrupt.");
}


/*
 * catch_signals()
 *
 * Arrange for SIGINT to be dealt with by clean_up().
 *
 */
void
catch_signals()
{
    if (signal(SIGINT, SIG_IGN) != SIG_IGN){
        if (signal(SIGINT, clean_up) == (int (*)())-1){
            e_error("Signal fails!");
        }
    }
    return;
}
