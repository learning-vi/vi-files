#include "e.h"


/* 
 * reconstruct()
 *
 * Finish off the building of the new history file. Emit the directory name
 * then the history, except for hist[except] which is the new most recently 
 * used entry and so we do it last.
 *
 * After this, emit the saved line if there was one and then the rest of
 * the original history.
 *
 * Then close the two files and rename the old history to be the new one.
 * Make sure they new .e file has the same perms as the old.
 *
 */
void
reconstruct(except)
int except;
{

    register int i;
    register int c;

    /* The name of the directory goes first. */
    ok_fprintf(tmp_fp, "%s\n", cwd);

    /* Put in the lines we still want. i.e. those excluding 'except'. */
    for (i = 0; i < hist_count; i++){
        if (i != except){
            ok_fprintf(tmp_fp, "\t%s\n", hist[i]);
        }
    }

    /* Put in the new entry. */
    ok_fprintf(tmp_fp, "\t%s\n", arg);

    /* The saved line, if there was one. */
    if (saved_line){
        ok_fprintf(tmp_fp, "%s\n", saved_line);
    }

    /* The rest of the old history file. */
    while ((c = getc(hist_fp)) != EOF){
        if (putc(c, tmp_fp) == EOF){
            perror("putc");
            exit(EX_IOERR);
        }
    }

    /* Clean up etc. */
    if (fclose(hist_fp) == EOF){
        e_error("Could not close '%s'.", ehist);
    }
    if (fclose(tmp_fp) == EOF){
        e_error("Could not close '%s'.", tmp_file);
    }
#ifdef Sysv
    if (unlink(ehist) == -1){
        e_error("Could not unlink '%s'\nNew history in '%s'.", ehist, tmp_file);
    }

    if (link(tmp_file, ehist) == -1){
        e_error("Could not link '%s' to '%s'.\n", tmp_file, ehist);
    }

    if (unlink(tmp_file) == -1){
        e_error("Could not unlink '%s'.\n", tmp_file);
    }
#else
    if (rename(tmp_file, ehist) == -1){
        e_error("Could not rename '%s' to '%s'.", tmp_file, ehist);
    }
#endif
    if (chmod(ehist, emode) == -1){
        e_error("Could not chmod '%s'", ehist);
    }
    return;
}
