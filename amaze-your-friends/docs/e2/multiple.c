#include "e.h"

/*
 * There were several names on the command line, so we just strcat them
 * into the 'arg' array. Check to see that the length of all the args
 * will not be greater than "size" or else we will overflow arg.
 *
 * This is actually insane - we put the args into a char array to pass to
 * do_vi, and then it promptly sticks them back into a char ** arrangement.
 * Ugh.
 *
 * The total argument length must be at most size - 1 characters, including
 * spaces. arg needs to have a trailing '\0' so that do_vi() wont break.
 *
 */

void
multiple(number, args, size)
int number;
char **args;
int size;
{
    register i;
    register total = 0;
    char temp_arg[ARG_CHARS];

    temp_arg[0] = '\0';
    while (--number){
        if ((total += strlen(*(args + 1))) >= size){

            /*
             * If you are running e and you find that this condition occurs,
             * the solution is to simply increase the value of the #define
             * line for ARG_CHARS in e.h. It shouldn't happen under normal
             * circumstances.
             *
             */

            ok_fprintf(stderr,
                "\007Argument list too long, truncated after '%s'.\n", *args);
            sleep(2);   /* Give them some chance to see what happened. */
            break;
        }

        arg[0] = '\0';
        strcat(arg, *++args);
        (void) spell_help(1);
        strcat(temp_arg, arg);
        strcat(temp_arg, " ");

        if (number > 1){
            strcat(arg, " ");

            /* 
             * Add one to total for the space. There's no need to check for
             * overflow here as we know there is another argument since
             * number > 1 still. Thus if this overflows arg, then it is going
             * to be caught anyway in the test at the top of the while loop.
             *
             */

            total++;                
        }
    }

    strcpy(arg, temp_arg);

    /*
     * Now, if there is a history file and we can find an identical line
     * then reconstruct with that line at the bottom.
     *
     */

    if (hist_count != -1){
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
        /* There was no history file so try to give them one for next time. */
        new_vi();
    }
    return;
}
