/* rectangular cut and paste: rcut.c
 * 
 * usage: rcut [-x | -k | -P | -p]
 *        where "-x" cuts rectangular section from input (the default)
 *              "-k" copies rectangular section from input
 *              "-P" pastes rectangular section into input ahead of marker 
 *              "-p" pastes rectangular section into input after marker
 *
 * operation:
 * 
 * reads lines from stdin and writes them to stdout with modifications:
 *
 * "-x" - reads lines from stdin and writes them to stdout file after
 * first cutting out columns indicated by the "C" markers on line 1 and
 * writing this cut text to the "TMP" file.
 *
 * "-k" - same as "-x" except columns are copied not cut.
 *
 * "-P" - reads lines from stdin and the "TMP" file and pastes the "TMP" text
 * into the input lines before position indicated by the "C" marker on line 1.
 *
 * "-p" - same as "-P" except text is pasted after the marker "C".
 *
 * vi implementation:
 *
 * rcut is designed to be used in vi for rectangular cut and paste:
 *
 * 1) To mark the section to be cut or copied use "mb" to mark the upper left
 *    (right) of the rectangle and the cursor position to mark the lower
 *    right (left) of the rectangle.  The same column may be used in which
 *    case a single column will be affected.
 *
 * 2) To cut out the section press "=x" in command mode. Key definition:
 *
 *    map =x meo256i `ejr^:m'b-`bkr^!'ercut -x
 *
 * 3) To copy rectangular section press "=k". Key definition:
 *
 *    map =k meo256i `ejr^:m'b-`bkr^!'ercut -k
 *
 * 4) To paste the previously cut text, place the cursor at the location at
 *    which the text is to be inserted and press "=P". Key definition:
 *
 *    map =P moO256i `okr^!Grcut -P
 *
 * 5) To paste the section after the cursor position use "=p". Key definition:
 *
 *    map =p moO256i `okr^!Grcut -p
 *
 * Note that the above key defs are suitable for being loaded directly into
 *   a .exrc file; i.e. they contain the correct control characters.  To print
 *   this file use "cat -v" to get the ascii equivalent.
 *
 * Note that the character '^' above (in r^) must be same as "C" below.
 *
 * Note also that rcut as written can handle lines up to: MAX_LENGTH/2
 *
 * Unresolved issues: Truncate trailing blanks after paste operation?
 *
 * author:  Robert Colon frc@dadd.ti.com
 * organization: Design Automation Division, Texas Instruments Inc.
 * date:    17Dec90
 * updated: 15Apr93 09:45:59
 *
 */

#include <stdio.h>
#include <string.h>
#define MAX_LENGTH 8192
#define HOME "HOME"
#define TMP "/.rcut"
#define C '^'
#define FALSE 0
#define TRUE  1

char what[]="@(#) rectangular cut and paste for the vi text editor - frc";

char name[MAX_LENGTH];
int kflag=FALSE;           /* "-k" argument present */
int Pflag=FALSE;           /* "-P" argument present */
int pflag=FALSE;           /* "-p" argument present */
int line1=TRUE;            /* indicates that line 1 is being processed */
int lcut=TRUE;             /* indicates more lines available from cut buffer */
int linp=TRUE;             /* indicates more lines available from stdin */
FILE *tmp;                 /* buffer file pointer */
char tmpf[MAX_LENGTH];     /* buffer file name */

main(argc,argv)

int argc;
char *argv[];     

{
  char *getenv();
  if (argc > 1) if (strcmp(argv[1],"-P")==0) Pflag=TRUE;
                else if (strcmp(argv[1],"-p")==0) pflag=TRUE;
                else if (strcmp(argv[1],"-k")==0) kflag=TRUE;
                else if (strcmp(argv[1],"-x")==0) ;
                else {
                  printf("Error: unrecognized argument: %s\n",argv[1]);
                  exit(1);
                }

  /* set buffer file name */

  strcpy(tmpf,getenv(HOME));
  strcat(tmpf,TMP);

  /* read in and process stdin */

  while (gets(name)) process();  

  /* paste remaining lines in cut buffer at end of stdout */

  if (pflag || Pflag) {
    linp=FALSE;
    while(lcut) {
      name[0]='\0';
      process();
    }
  }
  exit(0);
}

process()
{
  static int c1,c2; /* first and second positions of position character C */
  int c2x;          /* provisional end of string to be cut */
  int l;            /* length of strings */
  int n;            /* general index */
  static char lblank[MAX_LENGTH]; /* blank padding for output cut */
  static char cut[MAX_LENGTH];    /* characters for/from tmp */

  if (line1) {
    if ( ! strchr(name,C)) {
      if ( pflag || Pflag ) {
        printf("rcut: no column for paste indicated.\n");
        printf("      First line must have %c character in desired column.",C);
      }
      else {
        printf("rcut: no columns indicated for cutting or copying.\n");
        printf("      Indicate columns on first line with %c character(s).",C);
      }
      exit(1);
    }

    c1 = strchr(name,C) - name;   /* first */
    c2 = strrchr(name,C) - name;  /* last */

    if (pflag) c1++; /* "-p" -> insert text one column over */

    if ( pflag || Pflag ) tmp = fopen(tmpf,"r");
    else tmp = fopen(tmpf,"w");

    if ( ! tmp ) {
      printf("Error in opening buffer: %s\n",tmpf);
      exit(1);
    }

    /* create padding for output cut for lines shorter than c1 */

    for (n = 0; n < c2-c1+1; n++) lblank[n]=' ';
    lblank[n] = '\n';
    lblank[n+1] = '\0';
    line1=FALSE;
  }
  else {
    if ( pflag || Pflag ) { /* paste in rectangular section */
      if(lcut) {
        if ( ! fgets(cut,MAX_LENGTH,tmp) ) {
          lcut=FALSE;
          if(linp) puts(name);
          return 1;
        }
        cut[strlen(cut)-1] = '\0'; /* remove '\n' */
        if ( strlen(cut) > MAX_LENGTH/2 ) {
          printf("rcut: line in paste buffer %s > %d characters\n",tmpf,
            MAX_LENGTH/2);
          exit(1);
        }
        if ( (l=strlen(name)) > c1 ) 
          strcat(cut,&name[c1]);  /* append rest of input to cut */
        else { /* pad input string with blanks to bring up to correct length */
          for (n = l; n < c1; n++) name[n] = ' ';
          name[c1] = '\0';
        }
        strcpy(&name[c1],cut);  /* paste cut into input */
        puts(name);

        /* if trailing blanks are to be deleted, do it here */
      }
      else puts(name);
    }
    else { /* cut or copy out rectangular section */
      if ( (l=strlen(name)) > c1 ) {
        c2x = (l < c2+1) ? l-1:c2; /* minimum */
        strncpy(cut,&name[c1],c2x-c1+1);
        for ( n = c2x-c1+1; n < c2-c1+1; n++ ) cut[n] = ' ';
        cut[c2-c1+1] = '\n'; /* unlike puts, fputs does not add \n */
        cut[c2-c1+2] = '\0';
        fputs(cut,tmp);
  
        if(! kflag ) { /* cut or copy depending on kflag */
          if (l>c2+1) strcpy(&name[c1],&name[c2+1]);
          else name[c1] = '\0';
        }
      }
      else fputs(lblank,tmp);
      puts(name);
    }
  }
}
