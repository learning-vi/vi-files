/*
 * Stolen from "The UNIX Programming Environment" by Kernighan and Pike.
 *
 * Work out the distance between the strings 's' and 't' according
 * to the rough metric that
 *
 *     Identical = 0
 *     Interchanged characters = 1
 *     Wrong character/extra character/missing character = 2
 *     Forget it = 3
 *
 */
int
sp_dist(s, t)
char *s;
char *t;
{

    while (*s++ == *t){
        if (*t++ == '\0'){
            /* identical */
            return 0;
        }
    }

    if (*--s){
        if (*t){
            if (s[1]&&t[1]&&*s == t[1]&&*t == s[1]&&!strcmp(s+2, t+2)){
                /* Interchanged chars. */
                return 1;
            }
            if (!strcmp(s+1, t+1)){
                /* Wrong char. */
                return 2;
            }
        }
        if (!strcmp(s+1, t)){
            /* Extra char in 't'. */
            return 2;
        }
    }
    if (!strcmp(s, t+1)){
        /* Extra char in 's'. */
        return 2;
    }

    /* Forget it. */
    return 3;
}
