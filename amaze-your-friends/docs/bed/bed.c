/* Written by Stephen J. Muir, Computing Dept., Lancaster University */

/* version 1.0
 * release date 4th June 1985
 *
 * patch date 15th June 1985
 * changed to run on non-BSD systems
 * added 'v' command
 */

# include <sys/types.h>
# include <sys/stat.h>
# ifdef BSD
# include <sys/file.h>
# endif BSD
# include <stdio.h>
# include <signal.h>

extern char	*rindex (), *mktemp (), *getenv (), *getcom (), *getpar ();

char	*version = "Bed version 1.1\n";

char	*filename, *editor, *ap, abuf [32],
	*tempdata = "DbedXXXXXX", *temptext = "TbedXXXXXX";

short	ascii, dirty;

int	bits, base, width, i, count, reclen, ofd, dfd;

int	(*outfunc) (), (*infunc) ();

unsigned W32	buf	[512];

time_t	t_mtime;

FILE	*tfd;

struct stat	status;

syserr (name)
char	*name;
	{ perror (name);
	  unlink (tempdata);
	  exit (1);
	}

isdigit (c)
	char	c;
	{ return ('0' <= c && c <= '9');
	}

hexin (c)
	char	c;
	{ if ('0' <= c && c <= '9')
		return (c - '0');
	  c |= 040;
	  if ('a' <= c && c <= 'f')
		return (10 + c - 'a');
	  return (-1);
	}

char
hexout (num)
	unsigned W32	num;
	{ if (num <= 9)
		return ('0' + num);
	  return ('a' + num - 10);
	}

in8 ()
	{ unsigned W8	u, *cp = (unsigned W8 *)(&buf [0]);
	  char		c;
	  do
	  { while ((i = fgetc (tfd)) != EOF &&
		   ((c = i & 0377) == ' ' || c == '\n')
		  )
	    if (i != EOF && hexin (c) == -1)
		return (1);
	    u = hexin (c);
	    while ((i = fgetc (tfd)) != EOF && hexin (c = i & 0377) != -1)
		u = u * base + hexin (c);
	    if (i != EOF)
	    { if (c != ' ' && c != '\n')
		return (1);
	      *cp++ = u;
	    }
	    if (cp == ((unsigned W8 *)(&buf [0])) + reclen)
	    { if (write (dfd, (char *)buf, reclen) != reclen)
		return (-1);
	      cp = (unsigned W8 *)(&buf [0]);
	    }
	  }
	  while (i != EOF);
	  if ((i = cp - (unsigned W8 *)(&buf [0])) &&
	      write (dfd, (char *)buf, i) != i
	     )
		return (-1);
	  return (0);
	}

out8 ()
	{ unsigned W8	u = 0377, *cp;
	  do
		++width;
	  while (u /= base);
	  while ((i = count = read (dfd,
				    (char *)(cp = (unsigned W8 *)buf),
				    reclen
				   )
		 ) > 0
		)
	  { while (i)
	    { if (i-- != count && fputc (' ', tfd) == EOF)
		return (-1);
	      u = *cp++;
	      ap = &abuf [width];
	      do
	      { *--ap = hexout ((unsigned W32)(u % base));
		u /= base;
	      }
	      while (ap != &abuf [0]);
	      if (fwrite (abuf, sizeof (W8), width, tfd) == 0)
		return (-1);
	    }
	    if (fputc ('\n', tfd) == EOF)
		return (-1);
	  }
	  if (fflush (tfd) == EOF)
		return (-1);
	  return (0);
	}

in16 ()
	{ unsigned W16	u, *cp = (unsigned W16 *)(&buf [0]);
	  char		c;
	  do
	  { while ((i = fgetc (tfd)) != EOF &&
		   ((c = i & 0377) == ' ' || c == '\n')
		  )
	    if (i != EOF && hexin (c) == -1)
		return (1);
	    u = hexin (c);
	    while ((i = fgetc (tfd)) != EOF && hexin (c = i & 0377) != -1)
		u = u * base + hexin (c);
	    if (i != EOF)
	    { if (c != ' ' && c != '\n')
		return (1);
	      *cp++ = u;
	    }
	    if (cp == ((unsigned W16 *)(&buf [0])) + (reclen >> 1))
	    { if (write (dfd, (char *)buf, reclen) != reclen)
		return (-1);
	      cp = (unsigned W16 *)(&buf [0]);
	    }
	  }
	  while (i != EOF);
	  if ((i = cp - (unsigned W16 *)(&buf [0])) &&
	      write (dfd, (char *)buf, i << 1) != i << 1
	     )
		return (-1);
	  return (0);
	}

out16 ()
	{ unsigned W16	u = 0177777, *cp;
	  if (status.st_size & 1)
	  { fprintf (stderr, "filesize is not multiple of 2\n");
	    return (1);
	  }
	  do
		++width;
	  while (u /= base);
	  while ((count = read (dfd,
				(char *)(cp = (unsigned W16 *)buf),
				reclen
			       )
		 ) > 0
		)
	  { if (count & 1)
	    { fprintf (stderr, "read error\n");
	      return (1);
	    }
	    i = (count >>= 1);
	    while (i)
	    { if (i-- != count && fputc (' ', tfd) == EOF)
		return (-1);
	      u = *cp++;
	      ap = &abuf [width];
	      do
	      { *--ap = hexout ((unsigned W32)(u % base));
		u /= base;
	      }
	      while (ap != &abuf [0]);
	      if (fwrite (abuf, sizeof (W8), width, tfd) == 0)
		return (-1);
	    }
	    if (fputc ('\n', tfd) == EOF)
		return (-1);
	  }
	  if (fflush (tfd) == EOF)
		return (-1);
	  return (0);
	}

in32 ()
	{ unsigned W32	u, *cp = &buf [0];
	  char		c;
	  do
	  { while ((i = fgetc (tfd)) != EOF &&
		   ((c = i & 0377) == ' ' || c == '\n')
		  )
	    if (i != EOF && hexin (c) == -1)
		return (1);
	    u = hexin (c);
	    while ((i = fgetc (tfd)) != EOF && hexin (c = i & 0377) != -1)
		u = u * base + hexin (c);
	    if (i != EOF)
	    { if (c != ' ' && c != '\n')
		return (1);
	      *cp++ = u;
	    }
	    if (cp == &buf [0] + (reclen >> 2))
	    { if (write (dfd, (char *)buf, reclen) != reclen)
		return (-1);
	      cp = &buf [0];
	    }
	  }
	  while (i != EOF);
	  if ((i = cp - &buf [0]) && write (dfd, (char *)buf, i << 2) != i << 2)
		return (-1);
	  return (0);
	}

out32 ()
	{ unsigned W32	u = 037777777777, *cp;
	  if (status.st_size & 3)
	  { fprintf (stderr, "filesize is not multiple of 4\n");
	    return (1);
	  }
	  do
		++width;
	  while (u /= base);
	  while ((count = read (dfd,
				(char *)(cp = (unsigned W32 *)buf),
				reclen
			       )
		 ) > 0
		)
	  { if (count & 3)
	    { fprintf (stderr, "read error\n");
	      return (1);
	    }
	    i = (count >>= 2);
	    while (i)
	    { if (i-- != count && fputc (' ', tfd) == EOF)
		return (-1);
	      u = *cp++;
	      ap = &abuf [width];
	      do
	      { *--ap = hexout (u % base);
		u /= base;
	      }
	      while (ap != &abuf [0]);
	      if (fwrite (abuf, sizeof (W8), width, tfd) == 0)
		return (-1);
	    }
	    if (fputc ('\n', tfd) == EOF)
		return (-1);
	  }
	  if (fflush (tfd) == EOF)
		return (-1);
	  return (0);
	}

outasc ()
	{ char	c, *cp;
	  while ((i = count = read (dfd, cp = (char *)buf, reclen)) > 0)
	  { while (i--)
	    { if (' ' <= (c = *cp++) && c <= '~' && c != '\\')
	      { if (fputc (c, tfd) == EOF)
			return (-1);
	      }
	      else
	      { if (fputc ('\\', tfd) == EOF)
			return (-1);
		switch (c)
		{ case '\b':
		    c = 'b';
		    break;
		  case '\t':
		    c = 't';
		    break;
		  case '\f':
		    c = 'f';
		    break;
		  case '\n':
		    c = 'n';
		    break;
		  case '\r':
		    c = 'r';
		    break;
		  case '\\':
		    break;
		  default:
		    if (fputc ('0' + ((c >> 6) & 3), tfd) == EOF ||
			fputc ('0' + ((c >> 3) & 7), tfd) == EOF
		       )
			return (-1);
		    c = '0' + (c & 7);
		    break;
		}
		if (fputc (c, tfd) == EOF)
			return (-1);
	      }
	    }
	    if (fputc ('\n', tfd) == EOF)
		return (-1);
	  }
	  if (fflush (tfd) == EOF)
		return (-1);
	  return (0);
	}

inasc ()
	{ char	c, newc;
	  int	ret = 0;
	  FILE	*mydfd;
	  if ((mydfd = fdopen (dfd, "r+")) == NULL)
		return (-1);
	  while ((i = fgetc (tfd)) != EOF)
	  { c = i & 0377;
	    if (c == '\n')
		continue;
	    if (' ' <= c && c <= '~' && c != '\\')
	    { if (fputc (c, mydfd) == EOF)
		goto sysfail;
	      continue;
	    }
	    if (c != '\\' || (i = fgetc (tfd)) == EOF)
		goto fail;
	    switch (c = i & 0377)
	    { case 'b':
		c = '\b';
		break;
	      case 't':
		c = '\t';
		break;
	      case 'f':
		c = '\f';
		break;
	      case 'n':
		c = '\n';
		break;
	      case 'r':
		c = '\r';
		break;
	      case '\\':
		break;
	      default:
		if (c < '0' || c > '3')
			goto fail;
		newc = (c - '0') << 6;
		if ((i = fgetc (tfd)) == EOF || (c = i & 0377) < '0' || c > '7')
			goto fail;
		newc |= (c - '0') << 3;
		if ((i = fgetc (tfd)) == EOF || (c = i & 0377) < '0' || c > '7')
			goto fail;
		c = newc | (c - '0');
		break;
	    }
	    fputc (c, mydfd);
	    if (ferror (mydfd))
		goto sysfail;
	  }
	  goto out;
sysfail:  --ret;
	  goto out;
fail:	  ++ret;
out:	  dfd = dup (dfd);
	  if (fclose (mydfd) == EOF)
		return (-1);
	  return (ret);
	}

copyorig ()
	{
# ifdef BSD
	  lseek (ofd, 0, L_SET);
	  lseek (dfd, 0, L_SET);
	  ftruncate (dfd, 0);
# else BSD
	  lseek (ofd, 0, 0);
	  lseek (dfd, 0, 0);
	  close (creat (tempdata, 0));
# endif BSD
	  while ((count = read (ofd, (char *)buf, sizeof (buf))) > 0)
		if (write (dfd, (char *)buf, count) != count)
			syserr (tempdata);
	  if (count < 0)
		syserr (filename);
	  dirty = 0;
	}

edit ()
	{ int	pid;
	  if (base < 2 || base > 16)
	  { printf ("Radix must be between 2 and 16 inclusive.\n");
	    return;
	  }
	  switch (bits)
	  { case 8:
	      outfunc = out8;
	      infunc = in8;
	      break;
	    case 16:
	      outfunc = out16;
	      infunc = in16;
	      break;
	    case 32:
	      outfunc = out32;
	      infunc = in32;
	      break;
	    default:
	      printf ("This program can only manage 8, 16 or 32 bit formats\n");
	      return;
	  }
	  if (ascii)
	  { outfunc = outasc;
	    infunc = inasc;
	  }
	  else if (reclen % (bits >> 3))
	  { printf ("Record length is not a multiple of %d.\n", bits >> 3);
	    return;
	  }
	  if (reclen < 1)
	  { printf ("Record length is too small.\n");
	    return;
	  }
	  if ((tfd = fopen (temptext, "w+")) == NULL)
		syserr (temptext);
# ifdef BSD
	  lseek (dfd, 0, L_SET);
# else BSD
	  lseek (dfd, 0, 0);
# endif BSD
	  width = 0;
	  fstat (dfd, &status);
	  printf ("Preparing for edit.\n");
	  if ((i = (*outfunc) ()) < 0)
		perror (temptext);
	  if (i)
		goto out;
	  fstat (fileno (tfd), &status);
	  t_mtime = status.st_mtime;
	  while ((pid = fork ()) == -1);
	  if (pid == 0)
	  { execlp (editor, editor, temptext, 0);
	    perror (editor);
	    exit (1);
	  }
	  wait (0);
	  fstat (fileno (tfd), &status);
	  if (status.st_mtime != t_mtime)
	  { fseek (tfd, 0, 0);
# ifdef BSD
	    lseek (dfd, 0, L_SET);
	    ftruncate (dfd, 0);
# else BSD
	    lseek (dfd, 0, 0);
	    close (creat (tempdata, 0));
# endif BSD
	    printf ("Copying back changes.\n");
	    if ((i = (*infunc) ()) < 0)
	    { perror (tempdata);
	      goto out;
	    }
	    if (i)
	    { printf ("%s: bad format - original file restored\n", temptext);
	      copyorig ();
	    }
	    else
		++dirty;
	  }
out:	  fclose (tfd);
	  unlink (temptext);
	}

/*ARGSUSED*/
main (argc, argv, envp)
	char	*argv [], *envp [];
	{ char	*command;
	  if (!isatty (0))
	  { fprintf (stderr, "Standard input is not a tty.\n");
	    exit (1);
	  }
	  while (--argc && **++argv == '-' && *(*argv + 1))
	  { ++*argv;
	    while (**argv)
	    { switch (*(*argv)++)
	      { case 'c':
		case 'a':
		  if (ascii++)
			goto usage;
		  break;
		case 'r':
		  if (base)
			goto usage;
		  while (isdigit (**argv))
			base = base * 10 + *(*argv)++ - '0';
		  break;
		case 'b':
		  if (bits)
			goto usage;
		  while (isdigit (**argv))
			bits = bits * 10 + *(*argv)++ - '0';
		  break;
		case 'l':
		  if (reclen)
			goto usage;
		  while (isdigit (**argv))
			reclen = reclen * 10 + *(*argv)++ - '0';
		  break;
		default:
usage:		  fprintf (stderr,
			   "usage: bed [-c] [-r#] [-b#] [-l#] file\n"
			  );
		  exit (1);
	      }
	    }
	  }
	  if (bits == 0)
		bits = 16;
	  if (reclen == 0)
		reclen = 16;
	  if (base == 0)
		base = 8;
	  if (argc != 1)
		goto usage;
	  if (filename = rindex (*argv, '/'))
	  { *filename = '\0';
	    if (*(ap = *argv) == '\0')
		ap = "/";
	    if (chdir (ap) == -1)
		syserr (ap);
	    *filename++ = '/';
	  }
	  else
		filename = *argv;
# ifdef BSD
	  if ((ofd = open (filename, O_RDWR, 0)) == -1)
# else BSD
	  if ((ofd = open (filename, 2)) == -1)
# endif BSD
		syserr (*argv);
# ifdef BSD
	  if (flock (ofd, LOCK_EX | LOCK_NB) == -1)
	  { fprintf (stderr, "%s: waiting for lock to be released\n", *argv);
	    flock (ofd, LOCK_EX);
	  }
# else BSD
	  /* too bad locking isn't present on most systems - oh well */
# endif BSD
	  fstat (ofd, &status);
	  if ((status.st_mode & S_IFMT) != S_IFREG)
	  { printf ("%s: not regular file\n", *argv);
	    exit (1);
	  }
	  signal (SIGINT, SIG_IGN);
	  umask (0);
	  mktemp (tempdata);
	  mktemp (temptext);
# ifdef BSD
	  if ((dfd = open (tempdata, O_RDWR | O_CREAT, status.st_mode)) == -1)
# else BSD
	  if ((dfd = creat (tempdata, status.st_mode)) == -1 ||
	      close (dfd) == -1 ||
	      (dfd = open (tempdata, 2)) == -1
	     )
# endif BSD
		syserr (tempdata);
	  if ((editor = getenv ("VISUAL")) == 0 &&
	      (editor = getenv ("EDITOR")) == 0
	     )
		editor = EDITOR;
	  copyorig ();
	  for (;;)
	  { while ((command = getcom ("command: ")) == 0);
	    switch (*command)
	    { case 'w':
		if (!dirty &&
		    !quest ("File has not been modified; are you sure? ")
		   )
			break;
# ifdef BSD
		if (rename (tempdata, filename) == -1)
# else BSD
		if (unlink (filename) == -1 ||
		    link (tempdata, filename) == -1 ||
		    unlink (tempdata) == -1
		   )
# endif BSD
		{ perror ("rename()");
		  fprintf (stderr, "new file is in \"%s\"\n", tempdata);
		  exit (1);
		}
		exit (0);
	      case 'q':
		if (dirty && !quest ("File has been modified; are you sure? "))
			break;
		unlink (tempdata);
		exit (0);
	      case 'e':
		edit ();
		break;
	      case 'c':
	      case 'a':
		printf ("%sow in ascii mode.\n",
			(ascii = !ascii) ? "N" : "Not n"
		       );
		break;
	      case 'r':
		printf ("Radix is now %d.\n", base = atoi (getpar ("radix? ")));
		break;
	      case 'b':
		printf ("Word size is now %d bits.\n",
			bits = atoi (getpar ("bits? "))
		       );
		break;
	      case 'l':
		printf ("Record length is now %d bytes.\n",
			reclen = atoi (getpar ("record length? "))
		       );
		break;
	      case 'v':
		printf (version);
		break;
	      case 'h':
	      case '?':
		printf ("The following commands are available:\n");
		printf ("h - print out help\n");
		printf ("? - print out help\n");
		printf ("a - ascii toggle\n");
		printf ("c - ascii toggle\n");
		printf ("r - set radix\n");
		printf ("b - set number of bits in word\n");
		printf ("l - set record length\n");
		printf ("e - edit\n");
		printf ("w - write out file and quit\n");
		printf ("q - quit\n");
		printf ("v - print version\n");
		printf ("radix = %d, bits = %d,", base, bits);
		printf (" length of record = %d bytes%s.\n",
			reclen,
			ascii ? ", ascii mode" : ""
		       );
		break;
	      default:
		printf ("Invalid command -- type 'h' for help.\n");
		break;
	    }
	  }
	}
