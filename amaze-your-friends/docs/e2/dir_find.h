/* 
 * Directory delimiters for the VIPATH environment variable. 
 *
 */
#define is_delim(c) ((*c)==' '||(*c)==':'||(*c)=='\t'||(*c)=='\n')

/*
 * Walk over delimiters using the above macro.
 *
 */
#define skip_delim(c) while (is_delim((c))) (c)++;

/*
 * Walk over non-delimiters using the above macro.
 *
 */
#define skip_to_next_delim(c) while (*(c) && !is_delim((c))) (c)++;
