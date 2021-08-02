#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>

/* Sorted by long option name! */
static const struct option optab[] = {
	{ "long",		no_argument,		NULL,	'l' },
	{ "file",		required_argument,	NULL,	'f' },
};


/* main --- parse options */

int
main(int argc, char **argv)
{
	/*
	 * The + on the front tells GNU getopt not to rearrange argv.
	 */
	const char *optlist = "+lf:";
	int c;
	bool do_long;
	char *filename = NULL;

	/* we do error messages ourselves on invalid options */
	opterr = false;
	/* option processing. ready, set, go! */
	while ((c = getopt_long(argc, argv, optlist, optab, NULL)) != EOF) {
		switch (c) {
		case 'l':
			do_long = true;
			break;

		case 'f':
			filename = optarg;
			break;

		case 0:
			/*
			 * getopt_long found an option that sets a variable
			 * instead of returning a letter. Do nothing, just
			 * cycle around for the next one.
			 */
			break;

		case '?':
		default:
			fprintf(stderr, "%s: invalid option -- %c\n",
				argv[0], optopt);
			break;
		}
	}

	/* .... */

	return EXIT_SUCCESS;
}
