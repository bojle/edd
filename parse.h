#ifndef PARSE_H
#define PARSE_H

#include "ll.h"

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

node_t *pt_from(parse_t *pt);
parse_t *pt_make();
void pt_set(parse_t *pt, node_t *from, node_t *to, char cmd, char *rest);
char pt_command(parse_t *pt);

/* 
 * Lets functions know that the values of 
 * parse memebers are default values i.e.
 * were not modified by parse functions
 */
extern _Bool parse_defaults; 

extern char *gbl_commands;

/* eval */

#define FIRST_ASCII_CHAR '\n'
#define LAST_ASCII_CHAR 'z'
/* Size of fptr_table; accomodates all the characters that could be a command */
#define FPTR_ARRAY_SIZE (LAST_ASCII_CHAR - FIRST_ASCII_CHAR + 1) 

void eval(parse_t *pt);
void fptr_init();

#endif
