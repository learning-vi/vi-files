#include "e.h"

/*
 * dir_check()
 *
 * Checks to see if the name 'target' can be found in the directory 'dir'.
 * Make sure you are able to read it and that it is in fact a regular file.
 * Return 1 if it can, 0 if not.
 * 
 */
int 
dir_check(target, dir)
char *target;
char *dir;
{
    char filename[MAXPATHLEN];
    struct stat sbuf;

    ok_sprintf(filename, "%s/%s", dir, target);
    if (stat(filename, &sbuf) == -1){
        return 0;
    }
    /* 
     *  If it is not a directory and EITHER you own it and can
     *  read it OR you don't own it and it is readable by others, 
     *  OR you are in the group of the owner and it's group readable
     *      - then this is it.
     */

    if (((sbuf.st_mode & S_IFMT) == S_IFREG)  &&  
        ((sbuf.st_uid == (short)uid && sbuf.st_mode & S_IREAD) ||
        (sbuf.st_gid == (short)getgid() && sbuf.st_mode & G_READ) ||
        (sbuf.st_uid != (short)uid && sbuf.st_mode & O_READ))){

        return 1;
    }

    return 0;
}
