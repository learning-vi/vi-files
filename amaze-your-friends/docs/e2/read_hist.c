#include "e.h"

/*
 * read_hist()
 *
 * Do some initialization type things such as finding out if the user has a
 * uid, password entry, home directory etc. Then find out where the history
 * is kept and check to see if it's there. If it's not just indicate this by
 * returning -1 and let other functions deal with this problem as they see
 * fit.
 *
 * If the history exists then read it. The format of the history file is
 *

/absolute/path/name/for/some/directory
\tfilename_1
\tfilename_2
\tfilename_3
\tfilename_4
/absolute/path/of/some/other/directory
\tsome_filename_or_other
\tanother_filename

 *
 * Where \t is of course a single TAB.
 * The files for a directory are kept in least recently used order. So in
 * the above example, 'filename_1' was used before 'filename_2' and so on.
 * There may be a maximum of HIST_LINES file names associated with each 
 * directory.
 *
 * If the history is used and needs to be updated, then we will have to create
 * a new file that looks pretty much like the current one. Unfortunately we
 * will have to read and write the whole of the old file.
 *
 * Seeing as we are going to have to write the new history file, it doesn't
 * make much sense to try and find the relevant directory in a clever manner
 * (e.g. by keeping the history sorted by directory). The point is that
 * we HAVE to read and write the whole file once so it doesn't really matter
 * where we encounter the directory we want (if at all).
 *
 *
 * We return the number of history items found after setting the pointers in
 * hist[] to point to them. hist[0] points to the oldest filename, hist[1]
 * to the next oldest etc etc... hist[hist_count - 1] to the most recently
 * used.
 *
 */

int
read_hist()
{
    static char line[MAXPATHLEN]; /* Static, as saved_line may point at it. */
    int h_index = 0;              /* # of history items found so far.       */
    register int cwd_len;         /* # of chars in the current dir name.    */
    struct stat sbuf;

    cwd_len = strlen(cwd);

    if (stat(ehist, &sbuf) == -1){
        return -1;
    }

    emode = (int)sbuf.st_mode;

    hist_fp = fopen(ehist, "r");
    if (!hist_fp){
        e_error("Could not open history '%s'.", ehist);
    }

    /*
     * Open a new history file that will renamed to the old one once
     * we update. (If we update.)
     *
     */

    get_temp();

	/*
	 * Make sure that the first line of the history starts with a '/'
	 * This way we know we are not dealing with an old-style (pre version 1.3)
	 * history file. The error message here is unsatisfactory - it should make
	 * a better attempt to find out what's wrong and point them to the 
	 * script for converting to the new format etc etc. Fortunately I'm the
	 * only person in the world who uses e and so it's not a problem.
	 *
	 */

    if (!fgets(line, MAXPATHLEN, hist_fp)){
		e_error("Could not read first line of '%s'.\n", ehist);
	}

	if (line[0] != '/'){
		e_error("The history (%s) is munged. It is not in version %s format.",
			ehist, VERSION);
	}

    ok_fprintf(tmp_fp, "%s", line);

    /*
     * Get lines and write them to the new history until you see one that 
     * matches the directory we are in.
     *
     */
    while (fgets(line, MAXPATHLEN, hist_fp)){

        register char *tmp = line;
        int room_left = HIST_LINES * MAXPATHLEN;
        char *room;

        /* 
         * If it's not a path starting with a '/' then continue. 
         * We have a filename. Keep going until the next directory entry.
         *
         */
        if (line[0] != '/'){
            ok_fprintf(tmp_fp, "%s", line);
            continue;
        }

        zap_nl(tmp);

        /*
         * Test to see if this is the directory we want. tmp - line is the
         * length of the name of the found directory. Do a length test first
		 * up to avoid a strcmp if possible.
         *
         */
        if (tmp - line != cwd_len || strcmp(line, cwd)){
            ok_fprintf(tmp_fp, "%s\n", line);
            continue;
        }

        /* 
         * So we have found the directory we were looking for. Allocate a
         * hunk of space to read all the filenames. 
         */
        room = sbrk(room_left);
        if (room == (char *)-1){
            e_error("Could not sbrk().");
        }

        /*
         * Read the filenames. If we reach a line that starts with a '/'
         * then we are onto the next directory in the history and we save
         * a pointer to this line to emit it later.
         *
         */
        while (fgets(room, room_left, hist_fp)){
            char *cp = hist[h_index] = room + 1;
            zap_nl(cp);
            room_left -= (cp - room);

            /*
             * In theory we should never run out of space, but in case we do
             * we might as well try to grab some more rather than just dying.
             *
             */
            if (room_left <= MAXPATHLEN){
                room_left = HIST_LINES * MAXPATHLEN;
                room = sbrk(room_left);
                if (room == (char *)-1){
                    e_error("Could not sbrk().");
                }
            }
            
            if (*room == '/'){
                saved_line = room; /* Read one too many! */
                return h_index;
            }

            room = cp + 1;
            h_index++;

            /*
             * Check to see if we have read HIST_LINES filenames. If we have,
             * make sure that the next line is in fact a directory (by
             * verifying that it starts with a '/'.
             *
             */
            if (h_index == HIST_LINES){
                if (fgets(line, MAXPATHLEN, hist_fp)){
                    if (line[0] != '/'){
                        e_error("History contains too many entries!");
                    }
                    else{
                        tmp = line;
                        zap_nl(tmp);
                        saved_line = line;
                    }
                }
                return HIST_LINES;
            }
        }
        return h_index;
    }
    return h_index;
}
