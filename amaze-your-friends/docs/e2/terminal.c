#include "e.h"

/*
 * terminal()
 *
 * Handles the terminal. Must be first called as terminal(TERM_RECORD)
 * which remembers the initial terminal charcteristics and sets up the
 * "erase" variable. Thereafter can be called either as
 *
 * terminal(TERM_SET)  --  to turn on CBREAK and ECHO off.
 * terminal(TERM_RESET)  --  to set the terminal to its original state.
 *
 * This code is definitely a mess. If you don't have Sysv defined it does 
 * things which are known to work for BSD, SUN, ULTRIX and DYNIX.
 *
 */
void
terminal(what)
int what;
{
#ifdef Sysv
    static struct termio initial_blk;
    static struct termio set_blk;
#else
    static struct sgttyb initial_blk;
    static struct sgttyb set_blk;
#endif /* Sysv */



    switch(what){

        case TERM_RECORD:{
#ifdef Sysv
            if (ioctl(0, TCGETA, (char *)&initial_blk) == -1){
                e_error("Could not ioctl stdin.");
            }

#ifdef STRUCT_ASST
            /* Copy the structure in one hit. */
            set_blk = initial_blk;
#else
            /* Copy the structure field by field. */
            set_blk.c_iflag = initial_blk.c_iflag;
            set_blk.c_oflag = initial_blk.c_oflag;
            set_blk.c_cflag = initial_blk.c_cflag;
            set_blk.c_line = initial_blk.c_line;

            for (i = 0; i < NCC; i++){
                set_blk.c_cc[i] = initial_blk.c_cc[i];
            }
#endif /* STRUCT_ASST */

            /* And now set up the set_blk. */
            set_blk.c_lflag = (initial_blk.c_lflag &= ~(ICANON|ECHO|ECHONL));
            set_blk.c_lflag &= ICANON;
            erase = set_blk.c_cc[VERASE];
            set_blk.c_cc[VMIN] = 1;
            set_blk.c_cc[VTIME] = 0;
#else
            if (ioctl(0, TIOCGETP, (char *)&initial_blk) == -1){
                e_error("Could not ioctl stdin.");
            }

#ifdef STRUCT_ASST
            /* Copy the structure in one hit. */
            set_blk = initial_blk;
#else
            /* Copy the structure field by field. */
            set_blk.sg_ispeed = initial_blk.sg_ispeed;
            set_blk.sg_ospeed = initial_blk.sg_ospeed;
            set_blk.sg_erase = initial_blk.sg_erase;
            set_blk.sg_kill = initial_blk.sg_kill;
            set_blk.sg_flags = initial_blk.sg_flags;
#endif /* STRUCT_ASST */

            /* And now set up the set_blk. */
            erase = set_blk.sg_erase;

            /* Go into CBREAK mode or stay that way if we are already. */
            set_blk.sg_flags |= CBREAK;

            /* Turn off echo. */
            set_blk.sg_flags &= ~ECHO;
#endif /* Sysv */
            break;
        }

        case TERM_SET:{
#ifdef Sysv
            if (ioctl(0, TCSETA, (char *)&set_blk) == -1){
                e_error("Could not ioctl stdin.");
            }
#else
            if (ioctl(0, TIOCSETP, (char *)&set_blk) == -1){
                e_error("Could not ioctl stdin.");
            };
#endif /* Sysv */
            break;
        }

        case TERM_RESET:{
#ifdef Sysv
            if (ioctl(0, TCSETA, (char *)&initial_blk) == -1){
                e_error("Could not ioctl stdin.");
            }
#else
            if (ioctl(0, TIOCSETP, (char *)&initial_blk) == -1){
                e_error("Could not ioctl stdin.");
            }
#endif /* Sysv */
            break;
        }

        default:{
            /* Look! - no ifdefs here. */
            e_error("terminal() called with unknown parameter (%d).", what);
        }
    }
    return;
}

