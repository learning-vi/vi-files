#include "e.h"
#include "dir_find.h"

/*
 * dir_find()
 *
 * This takes the environment variable which is #defined as PATH and 
 * extracts the directory names from it. They may be separated by 
 * arbitrary numbers of delimiter characters (currently "\n", "\t", " " 
 * and ":"). Each directory is then checked to see if it contains the 
 * desired filename (with a call to check). Spelling corrections are 
 * not attempted.
 *
 * This could have been done more simply using strtok() but I didn't know
 * about that then... You'll have to bear with me.
 *
 * 'this_dir' will point at the start of the directory name that is to be
 * processed. 'cp' will be advanced to the next delimiter which will be zeroed
 * and then 'cp' will be again advanced until it reaches a non-delimiter. This
 * marks the start of the next name and at the bottom of the loop 'this_dir'
 * is set to be 'cp' and we begin again.
 *
 * skip_delim() and friends are macros that live in dir_find.h
 *
 */
void
dir_find()
{
    char *p;
    char path[E_PATH_LEN];
    char *this_dir;
    char *cp;

    /*
     * Get the environment variable, check its length and cp it to a safe spot.
     *
     */
    p = getenv(E_PATH);
    if (!p) return;

    if (strlen(p) >= E_PATH_LEN){
        e_error("Length of '%s' variable exceeds %d.", E_PATH, MAXPATHLEN);
    }

    strcpy(path, p);

    /*
     * Begin at the beginning...
     *
     */
    cp = path;
    skip_delim(cp);

    if (!*cp){
        /* 
         * There was nothing there but delimiters! 
         *
         */
        return;
    }

    this_dir = cp;

    while (*this_dir){

        /* 
         * Move "cp" along to the first delimiter. 
         *
         */
        skip_to_next_delim(cp);

        /*
         * If it's not already '\0' then zero it and move on. Otherwise we
         * have reached the end of the string.
         *
         */
        if (*cp){
            *cp = '\0';
            cp++;
        }

        /* 
         * Move "cp" along over delimiters unitl the next directory name. 
         *
         */
        skip_delim(cp);

        /* 
         * Check the directory "this_dir" for the filename "arg". 
         * If it's there, offer it to them.
         *
         */
        if (dir_check(arg, this_dir)){

            char prompt[MAXPATHLEN + 128];
            ok_sprintf(prompt, "%s/%s [y]? ", this_dir, arg);

            /* 
             * Get and process the reply. 
             *
             */
            switch (char_in(prompt)){

                case 'N':{
                    /*
                     * They don't want it and they don't want more help.
                     *
                     */
                    ok_fprintf(stderr, "No!\n");
                    return;
                }

                case 'n':{
                    /*
                     * They don't want it but continue to search for another.
                     *
                     */
                    ok_fprintf(stderr, "no\n");
                    break;
                }

                case 'q':
                case 'Q':{
                    /*
                     * Get out.
                     *
                     */
                    ok_fprintf(stderr, "quit\n");
                    abandon();
                    exit(0);
                }

                default :{
                    /*
                     * They want it. Set up the filename in 'arg'.
                     *
                     */
                    char tmp[MAXPATHLEN];

                    ok_fprintf(stderr, "yes\n");
                    ok_sprintf(tmp, "%s/%s", this_dir, arg);
                    arg[0] = '\0';
                    strcat(arg, tmp);
                    return;
                }
            }
        }
        this_dir = cp;
    }

    return;
}
