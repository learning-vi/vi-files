#include "e.h"

/*
 * char_in()
 *
 * Display the string in 'prompt', go into cbreak and get a single character
 * and then reset the terminal. Return the character.
 *
 */
int
char_in(prompt)
char *prompt;
{
    int c;

    ok_fprintf(stderr, "%s", prompt);
    terminal(TERM_SET);
    c = getc(stdin);
    terminal(TERM_RESET);
    return c;
}

/*
 * ok_printf()
 *
 * Call fprintf and check the return status. I did this to satisfy lint.
 * It makes things slower with the extra function call overhead though.
 * Sigh.
 *
 */
/* VARARGS2 */
void
ok_fprintf(stream, format, u, v, w, x, y, z)
FILE *stream;
char *format;
{
    if (fprintf(stream, format, u, v, w, x, y, z) == EOF){
        perror("fprintf");
        exit(EX_IOERR);
    }
}


#ifndef Sysv
/*
 * ok_srintf()
 *
 * Call sprintf and check the return status. I did this to satisfy lint.
 * It makes things slower with the extra function call overhead though.
 * Sigh.
 *
 */
/* VARARGS2 */
void
ok_sprintf(dest, format, u, v, w, x, y, z)
char *dest;
char *format;
{
    if (sprintf(dest, format, u, v, w, x, y, z) == (char *)EOF){
        perror("sprintf");
        exit(EX_IOERR);
    }
}
#endif
