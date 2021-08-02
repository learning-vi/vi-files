# include <stdio.h>
# include <stdlib.h>

int main(int argc, char *argv[])
  {
  /*
   * arg 1: starting value
   * arg 2: second value
   * arg 3: number of entries to print
   *
  */

  if (argc - 1 != 3)
    {
    printf ("Three command line args: (you used %d)\n", argc-1);
    printf ("usage: value 1, value 2, number of entries\n");
    return (1);
    }



  /* count = how many to print */
  int count = atoi(argv[3]);

  /* index = which to print */
  long int index;

  /* first and second passed in on command line */
  long int first, second;

  /* these get calculated */
  long int current, nMinusOne, nMinusTwo;

  first  = atoi(argv[1]);
  second = atoi(argv[2]);
  printf("%i fibonacci numbers with starting values: %li, %li\n", count, first, 
      second);
  printf("=======================================\n");

  /* print the first 2 from the starter values */
  printf("%i %04li\n", 1, first);
  printf("%i %04li  ratio (golden?) %.3f\n", 2, second, (double) second/first);

  nMinusTwo = first;
  nMinusOne = second;

  for (index=1; index<=count; index++)
    {
    current = nMinusTwo + nMinusOne;
    printf("%li %04li  ratio (golden?) %.3f\n",
            index,
            current,
            (double) current/nMinusOne);
    nMinusTwo =  nMinusOne;
    nMinusOne = current;
    }
  }
