#include "e.h"

/*
 * find_hist()
 *
 * Find out where the history file is hiding. If E_HIST (currently this is
 * "VIHIST") is defined, then use that. Otherwise use a default ($HOME/.e
 * at the moment.)
 *
 * If E_HIST starts with a '/' then take it as an absolute path, otherwise
 * take it relative to the home directory.
 *
 * Set up the name of the temporary file to be in the same directory as the
 * history. This ensures 1) that we can write there and 2) that we can use
 * rename(2) when we want to make it the new history. (I'd just use /tmp but 
 * that stops me from using rename).
 *
 */
void
find_hist()
{
    char *efile;
    struct passwd *pwd;

    uid = (int)getuid();
    pwd = getpwuid(uid);

    if (!pwd){
        e_error("Could not get password file entry for uid %d.", uid);
    }

    home = pwd->pw_dir;
    efile = getenv(E_HIST);

    if (!efile){
        /*
         * E_HIST is not set.
         * Use the default location and name for the history file (that
         * is name = DEFAULT_HIST in the home directory.)
         *
         */
        ok_sprintf(ehist, "%s/%s", home, DEFAULT_HIST);
        ok_sprintf(tmp_file, "%s/.e_tempXXXXXX", home);
    }
    else{
        /*
         * It was set.
         *
         */
        if (*efile == '/'){
            /*
             * It's an absolute pathname. Copy it into ehist and tmp_file.
             * Zero the last '/' in tmp_file it to get the basename, then
             * strcat the .e_tempXXXXXX stuff. The call to rindex() cannot 
             * fail to find a '/' since by this time we know that the first 
             * character of efile (and hence ehist and tmp_file) is '/'.
             *
             */
            strcpy(ehist, efile);
            strcpy(tmp_file, efile);
            *rindex(tmp_file, '/') = '\0';
            strcat(tmp_file, "/.e_tempXXXXXX");
        }
        else{
            /*
             * Take it as being relative to the home directory.
             *
             */
            ok_sprintf(ehist, "%s/%s", home, efile);
            ok_sprintf(tmp_file, "%s/.e_tempXXXXXX", home);
        }
    }

#ifdef Sysv
    if (!getcwd(cwd, sizeof(cwd))){
#else
    if (getwd(cwd) == (char *)0){
#endif
        e_error("Could not get working directory.");
    }
    return;
}
