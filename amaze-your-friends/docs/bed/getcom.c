# include <stdio.h>

# define LINESIZE 82

char	*parptr, comline [LINESIZE];

comerr ()
	{ fprintf (stderr, "getcom(): bad input\n");
	}

char	*
getcom (prompt)
	char	*prompt;
	{ register char	*old, *new;
	  register short	count;
	  parptr = (char *)0;
	  if (prompt != (char *)0)
		fprintf (stderr, prompt);
	  if ((count = read (0, comline, LINESIZE - 1)) < 0)
	  { perror ("getcom()");
	    return ((char *)0);
	  }
	  if (count == 0)
		return ((char *)0);
	  if (comline [--count] != '\n')
	  { comerr ();
	    return ((char *)0);
	  }
	  comline [count] = '\0';
	  if (count == 0)
		return (comline);
	  for (old = new = &comline [0]; *old != '\0'; ++old)
	  { if (*old == '!')
		return (old);
	    if (*old != ' ' && *old != '\t')
		break;
	  }
	  while (*old != '\0')
	  { while (*old != '\0' && *old != ' ' && *old != '\t')
		*new++ = *old++;
	    while (*old == ' ' || *old == '\t')
		++old;
	    *new++ = '\0';
	    if (parptr == (char *)0)
		parptr = new;
	  }
	  *new = '\0';
	  if (*parptr == '\0')
		parptr = (char *)0;
	  if (old != &comline [count])
	  { comerr ();
	    return ((char *)0);
	  }
	  return (comline);
	}

char	*
getpar (prompt)
	char	*prompt;
	{ register char	*old;
	  register short	count;
	  if (parptr != (char *)0)
	  { old = parptr;
	    while (*parptr++ != '\0');
	    if (*parptr == '\0')
		parptr = (char *)0;
	    return (old);
	  }
	  if (prompt != (char *)0)
		fprintf (stderr, prompt);
	  if ((count = read (0, comline, LINESIZE - 1)) < 0)
	  { perror ("getpar()");
	    return ((char *)0);
	  }
	  if (count == 0)
		return ((char *)0);
	  if (comline [--count] != '\n')
	  { fprintf (stderr, "getpar(): bad input\n");
	    return ((char *)0);
	  }
	  comline [count] = '\0';
	  return (comline);
	}
