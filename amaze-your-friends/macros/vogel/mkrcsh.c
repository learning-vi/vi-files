#ifndef	lint
static char    *mkrcsh_c_rcsid =
"$Id: mkrcsh.c,v 1.8 1991/08/16 17:43:51 vogel Exp $";

static char    *mkrcsh_c_source =
"$Source: /d/cdc/vogel/util/RCS/mkrcsh.c,v $";

#endif

/*
 *	mkrcsh:		accepts a filename, and creates a string containing
 *			RCS header information for the file:
 *
 *				#ifndef lint
 *				static	char	*	name_x_rcsid =
 *					"$ Id$";
 *
 *				static	char	*	name_x_source =
 *					"$ Source$";
 *				#endif
 *
 *			is created, where "name" is the name of the source
 *			file, and "x" is the extension.
 *
 *			A function value of 0 is returned if everything is
 *			OK, 1 otherwise.
 *
 *		****	THERE IS ALSO A MAIN PROGRAM BY THIS NAME!  The binary
 *			has the name "mkrcsh", and the source is "tst.mkrcsh".
 *
 *		Usage:	int mkrcsh (file, header)
 *			char * file;
 *			char * header;
 *
 *		Where:	"file"	 is the file name.
 *			"header" holds the desired header string.
 *
 *		Ex:	mkrcsh ("remark.c", string) stores
 *
 *			#ifndef lint
 *			static	char	*	remark_c_rcsid =
 *				"$ Id$";
 *
 *			static	char	*	remark_c_source =
 *				"$ Source$";
 *			#endif
 *
 *			in the array "string".  Actually, there is no space
 *			between "$" and "I" in the array - it was put there
 *			to prevent RCS expansion when THIS file is checked in.
 */

#include	<stdio.h>
#include	<strings.h>
#include	<sys/param.h>

int 
mkrcsh (file, header)
char           *file;
char           *header;
{

/*
 *	Variables.
 */

	char           *slash;
	char           *t;

	char            temp[MAXPATHLEN];

/*
 *	Check arguments.  If either one is NULL, return with status 1.
 */

	if (file == (char *) NULL || header == (char *) NULL)
	{
		fprintf (stderr, "mkrcsh:  one of the arguments is null\n");
		return (1);
	}

/*
 *	Use only the basename for the variable name.  Make sure that any
 *	special characters are turned into underscores.
 */

	if (slash = rindex (file, '/'))
		*slash++ = '\0';
	else
		slash = file;

	(void) strcpy (temp, slash);

	for (t = temp; *t; ++t)
		if (index ("@$-+;:=.,", *t))
			*t = '_';

/*
 *	Write the header string.
 */

	(void) strcpy (header, "#ifndef\tlint\nstatic\tchar\t*\t");
	(void) strcat (header, temp);
	(void) strcat (header, "_rcsid\t=\n\t\"$I");
	(void) strcat (header, "d$\";\n\n");

	(void) strcat (header, "static\tchar\t*\t");
	(void) strcat (header, temp);
	(void) strcat (header, "_source\t=\n\t\"$Sour");
	(void) strcat (header, "ce$\";\n#endif\n\n");

	return (0);
}
