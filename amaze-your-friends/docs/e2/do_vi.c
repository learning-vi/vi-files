#include "e.h"

/* 
 * do_vi()
 *
 * Split the arguments (if any) in 'thing' up to be passed to execvp,
 * and exec vi on them. The arguments in 'thing' are space separated.
 * Just walk down the 'thing' array, zero the spaces and set pointers 
 * to the arguments.
 *
 */

void
do_vi(thing)
char *thing;
{
    char *args[MAX_ARGS];
    register int i = 1;
	int go_home = 0;

    args[0] = "vi";
	skip_white(thing);

	while (*thing){
		register char *tmp = thing;
		skip_to_white(tmp);
		if (*tmp) *tmp++ = '\0';
		skip_white(tmp);

		if (i > 1 || (*thing != '-' && *thing != '+')){
			if (safety_first(thing)){
				/*
				 * Have to go home for this one.
				 * Grab some space to put the new filename into.
				 * The filename is to be cwd/thing.
				 *
				 */
				char *space = sbrk(MAXPATHLEN + 1);
				go_home = 1;
				if (space == (char *)-1){
					e_error("Could not sbrk.");
				}
				sprintf(space, "%s/%s", cwd, thing);
				thing = space;
			}
		}
		args[i++] = thing;
		thing = tmp;
	}

    args[i] = NULL;

	if (go_home && chdir(home) == -1){
		e_error("Could not chdir to '%s'.", home);
	}
        
    if (execvp(VI, args) == -1){
        e_error("Could not execvp '%s'", VI);
    }
}


int
safety_first(s)
char *s;
{
	register char *slash;
	struct stat sbuf;
	char away_dir[MAXPATHLEN];
	int away;
	int own;

	/*
	 * If they are not concerned with inheritance, just return.
	 *
	 */
	if (!safe_inherit && !inherit){
		return 0;
	}

	/*
	 * See if there is a .exrc file in the directory in which the file we
	 * are going to be editing lives.
	 */
	slash = rindex(s, '/');

	if (slash){
		/*
		 * The file we are editing lies (probably) in a directory that is 
		 * not the one we are in. Check it by inode numbers though.
		 *
		 */
		char tmp[MAXPATHLEN];
		ino_t here_ino;

		*slash = '\0';
		ok_sprintf(away_dir, "%s", s);
		*slash = '/';
		ok_sprintf(tmp, "%s/.exrc", away_dir);

		if (stat(tmp, &sbuf) == -1){
			/*
			 * There is no .exrc file.
			 *
			 */
			return 0;
		}
		
		/*
		 * There is a .exrc file.
		 *
		 */
		if (uid == sbuf.st_uid){
			/*
			 * And we own it.
			 *
			 */
			away = 1;
			own = 1;
		}
		else{
			/*
			 * We don't own the .exrc file. 
			 *
			 */
			own = 0;
		}

		/*
		 * Decide whether or not the file we are going to edit is in
		 * this directory. We may have been deceived by the presence of the
		 * '/' since the filename could be ../this_dir etc etc.
		 *
		 */

		if (stat(cwd, &sbuf) == -1){
			e_error("Cannot stat '%s'.", cwd);
		}
		here_ino = sbuf.st_ino;

		if (stat(away_dir, &sbuf) == -1){
			e_error("Cannot stat '%s'.", away_dir);
		}

		if (sbuf.st_ino == here_ino){
			 away = 0;
		}
		else{
			away = 1;
		}
	}
	else{
		/*
		 * The file is in this directory.
		 *
		 */

		/*
		 * We know we are away, there was no '/' in the filename.
		 * The possibility of the filename being a link is excluded I
		 * think, because earlier on we checked for regular files.
		 * I'll check this again. In any case no harm could come as
		 * at worst we would attempt to vi a directory.
		 *
		 */
		away = 0;

		if (stat(".exrc", &sbuf) == -1){
			/*
			 * No .exrc file.
			 *
			 */
			return 0;
		}

		if (uid == sbuf.st_uid){
			own = 1;
		}
		else{
			own = 0;
		}
	}

	/*
	 * We have now set up 'away' and 'own'.
	 *
	 * away = 1  =>  The file we are editing is not in our current directory.
	 * away = 0  =>  The file we are editing is in our current directory.
	 *
	 * own = 1   =>  We own the .exrc file.
	 * own = 0   =>  We do not own the .exrc file.
	 *
	 *
	 */

	/*
	 * It's in this directory and we own it. So just return and pretend
	 * to be a normal vi :-)
	 *
	 */
	if (!away && own){
		return 0;
	}

	/*
	 * It's in this directory and we don't own it and we don't want to inherit
	 * it. 
	 *
	 * Return 1 to indicate that the filename that needs to be edited 
	 * should now be preceeded by 'cwd/'. And that we need to chdir to home
	 * before we exec vi.
	 *
	 */
	if (!away && !own && safe_inherit){
		return 1;
	}

	/*
	 * The other option is that it's in this directory and we don't own
	 * it and either 1) inherit is set or 2) inherit is not set AND neither
	 * is safe_inherit. In either case we are going to inherit the thing.
	 *
	 */

	 /*
	  * Now to the cases where the file we are going to edit is not in
	  * this directory.
	  *
	  */

	/*
	 * It's not in this directory and we own it and we want to inherit. 
	 * So chdir over there before returning.
	 *
	 */
	if (away && own && inherit){
		if (chdir(away_dir) == -1){
			e_error("Could not chdir to '%s'.", away_dir);
		}
		return 0;
	}

	/*
	 * It's not in this directory and we don't own it and we want to inherit
	 * even if it's not safe. So chdir over there before returning.
	 *
	 */
	if (away && !own && inherit && !safe_inherit){
		if (chdir(away_dir) == -1){
			e_error("Could not chdir to '%s'.", away_dir);
		}
		return 0;
	}

	/*
	 * And I think that's about it. Time will tell.
	 *
	 */
	
	return 0;
}
