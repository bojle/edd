#ifndef PARSE_H
#define PARSE_H

/*
 * parse and eval are the two major functions in parse.h
 * parse() does not fully parse an expression. since, each 
 * command accepts different, often unique arguments, the 
 * unparsed expression are relegated to be parsed by the 
 * functions that implement these commands
 */


typedef struct parse_t parse_t;

/* parse */

/* parse an expression and return a parse_t object pointer */
parse_t *parse(char *exp);
/* 
 * Parse the address part of a expression, return the first character at
 * the end of an address
 */
char *parse_address(parse_t *pt, char *addr);
int isaddresschar(char *a);
char *skipspaces(char *s);

/* eval */
void eval(parse_t *pt);
void fptr_init();

#endif
