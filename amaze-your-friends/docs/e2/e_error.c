#include "e.h"

/*
 * e_error()
 *
 * Error message printer. The argument a should be a format string that
 * can be printed using fprintf(). We make sure the terminal is in a sane
 * condition. Then the message is printed preceded by the
 * name we were invoked with and succeeded by a newline.
 * Then open files are closed and we get out as quickly as we can.
 *
 */
/* VARARGS1 */
void
e_error(u, v, w, x, y, z)
char *u;
{
    terminal(TERM_RESET);
    ok_fprintf(stderr, "%s: ", myname);
    ok_fprintf(stderr, u, v, w, x, y, z);
    if (fputc('\n', stderr) == EOF){
        perror("fputc");
        exit(EX_IOERR);
    }
    abandon();
    if (fflush(stderr) == EOF){
        perror("fflush");
    }
    _exit(1);
}
