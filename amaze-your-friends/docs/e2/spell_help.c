#include "e.h"

/*
 * spell_help()
 *
 * Read the directory and if the file they want (in 'arg') does not
 * exist then see if there is one that does which has similar spelling
 * to what they requested. Offer the change and handle the reply.
 *
 * The argument tells us how we should form the prompt if we get a match.
 * 1 = this file was one of several arguments to e and so we should say
 * "correct 'frde' to 'fred'?". If there were only one argument it would
 * be redundant to say that, they know their argument was 'frde', so we
 * just say "correct to 'frde'?". Little things make all the difference.
 *
 * Return 1 if we were able to help, 0 otherwise.
 *
 */
int
spell_help(flag)
int flag;
{
    DIR *dp; 
    DIR *opendir();
    struct direct *readdir();
    struct direct *entry;
    register int len = strlen(arg);
    struct stat buf;
    int ch;

    /* 
     * If the file already exists just return - they don't need help. 
     *
     */
    if (stat(arg, &buf) == 0){
        return 1;
    }

    /* 
     * If the current directory can't be read then return. 
     *
     */
    if ((dp = opendir(".")) == NULL){
        return 0;
    }

    for (entry = readdir(dp); entry != NULL; entry = readdir(dp)){

#ifdef Sysv
        register int dlen = strlen(entry->d_name);
#else
        register int dlen = entry->d_namlen;
#endif

        if (stat(entry->d_name, &buf) == -1){
            continue;
        }

        /* 
         * If it's not a regular file then continue. 
         *
         */
        if ((buf.st_mode&S_IFMT) != S_IFREG){
            continue;
        }

        /* 
         * If this entry has a non-zero inode number and
         *
         *      name length == sought length +/- 1 
         *
         * then it should be checked.
         *
         */

        if (entry->d_ino && dlen >= len - 1 && dlen <= len + 1){

            char prompt[MAXPATHLEN + 128];

            /* 
             * If the distance between this name and the one the user enetered
             * is too great then just continue.
             *
             */

            if (sp_dist(entry->d_name, arg) == 3) continue;


            /* 
             * Otherwise offer them this one and get the response.
             *
             */
            if (flag){
                ok_sprintf(prompt, "correct '%s' to '%s' [y]? ", arg, 
                    entry->d_name);
            }
            else{
                ok_sprintf(prompt, "correct to '%s' [y]? ", entry->d_name);
            }
            ch = char_in(prompt);

            if (ch == 'N'){
                /* 
                 * No, and they mean it. Offer no more help. 
                 *
                 */
                ok_fprintf(stderr, "No!\n");
                break;
            }

            else if (ch == 'n'){
                /* 
                 * No, but they'd like more help. 
                 *
                 */
                ok_fprintf(stderr, "no\n");
                continue;
            }

            else if (ch == 'q' || ch == 'Q' || ch == (int)erase){
                /* 
                 * Quit. 
                 *
                 */
                ok_fprintf(stderr, "quit\n");
                closedir(dp);
                abandon();
                exit(0);
            }

            else{
                /* 
                 * Yes. 
                 *
                 */
                ok_fprintf(stderr, "yes\n");
                closedir(dp);
                strcpy(arg, entry->d_name);
                return 1;
            }
        }
    }
    closedir(dp);
    return 0;
}
