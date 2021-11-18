#ifndef PARSE_H
#define PARSE_H

#include "ll.h"

typedef struct parse_t {
	node_t *from;
	node_t *to;
	char command;
	char *argument;
	char *regex;
}parse_t;

/* parse an expression and return a parse_t object */
parse_t *parse(parse_t *pt, char *exp);
/* 
 * populate the 'from' and 'to' pointers of a parse_t 
 * object, return a pointer to the expression where the 
 * address ends
 */
char *parse_address(parse_t *pt, char *addr);
int isaddresschar(char *a);
char *skipspaces(char *s);

void eval(parse_t *pt);

#endif
