#ifndef PARSE_H
#define PARSE_H

/*
 * parse and eval are the two major functions in parse.h
 * parse() does not fully parse an expression. since, each 
 * command accepts different, often unique arguments, the 
 * unparsed expression are relegated to be parsed by the 
 * functions that implement these commands
 */


#include "ll.h"

typedef struct parse_t parse_t;

/* parse */
parse_t *parse(parse_t *pt, char *exp);
char *parse_address(parse_t *pt, char *addr);
int isaddresschar(char *a);
char *skipspaces(char *s);


/* eval */
void eval(parse_t *pt);
#define TOTAL_COMMANDS ('z' - 'a' + 1)
int fp_hash(char c);

#endif
