#include "parse.h"
#include <stdio.h>
#include "ed.h"
#include "ll.h"
#include <stdbool.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>


/* 
 * An expression: [address][sep][command][sep][arguments]
 * [address]: 1,3 8,$ .,$ ,
 * [sep]: ' ' or ''
 * [command]: s,c,a,W,e etc.
 * [arguments]: any text after command
 *
 * Eg expression: 9,15s/dog/cat/g
 * Here, 9,15 is forms address, 's' is the command, everything else from '/' 
 * are arguments to 's' command.
 */

struct parse_t {
	node_t *from;
	node_t *to;
	char command;
	char *argument;
	char *regex;
};

static parse_t pt;

parse_t *parse(char *exp) {
	/* defaults */
	pt.from = ll_next(global_head(), 1);
	pt.to = ll_prev(global_tail(), 1);
	pt.command = '\0';
	pt.argument = NULL;

	exp = parse_address(&pt, exp);
	exp = skipspaces(exp);
	pt.command = *exp++;
	pt.argument = skipspaces(exp);
	return &pt;
}

char *skipspaces(char *s) {
	if (! isspace(*s))
		return s;
	while (isspace(*s))
		s++;
	return s;
}

int isaddresschar(char *a) {
	if (*a == '-' || *a == '+' || *a == '$' ||
		*a == '.' || *a == ',' || isdigit(*a) ||
		(isalpha(*a) && *(a-1) == '\''))
		return 1;
	return 0;
}


/* 
 * populate the 'from' and 'to' pointers of a parse_t 
 * object, return a pointer to the expression where the 
 * address ends
 */
char *parse_address(parse_t *pt, char *addr) {
	bool commapassed = false;
	for (; isaddresschar(addr); addr++) {
		long num = 1;
		char *start = NULL;
		switch (*addr) {
			case '.':
				if (commapassed) { 
					pt->to = global_current();
				}
				else { 
					pt->from = global_current();
				}
				break;
			case '$':
				if (commapassed) { 
					pt->to = global_tail();
				}
				else { 
					pt->from = global_tail();
				}
				break;
			case ',': 
				if (!isaddresschar(addr+1)) { 
					 /* ,s/dog/cat/   -- here range is from head to tail
					  * this if block checks for such cases */
					pt->from = global_head();
				}
				commapassed = true;
				break;
			case '-':
				/* A minus sign, what follows is a number */
				if (isdigit(*(addr+1))) {
					num = strtol(addr + 1, &addr, 10);
				}

				if (commapassed) {
					pt->to = ll_prev(global_current(), num);
		   		}
				else {
					pt->from = ll_prev(global_current(), num);
				}
				addr--;
				break;
			case '+':
				/* analogous to the previous block for '-' */
				if (isdigit(*(addr+1))) {
					num = strtol(addr + 1, &addr, 10);
				}

				if (commapassed) {
					pt->to = ll_next(global_current(), num);
				}
				else {
					pt->from = ll_next(global_current(), num);
				}
				addr--;
				break;
			case ';':
				pt->from = global_current();
				pt->to = global_tail();
				break;
			case '/':
				start = addr+1;
				addr++;
				while (*addr != '/' && *(addr-1) != '\\') {
					addr++;
				}
				*addr = '\0';
				pt->regex = start;
				break;
			case '\'':
				//if ((pt->from = markget(*(addr+1))) == NULL) {
				//	io_err("Mark not set %c\n", *(addr+1));
				//}
				break;
			default:
				if (isdigit(*addr)) {
					num = strtol(addr, &addr, 10);
					if (commapassed) {
						pt->to = ll_at(num);
					}
					else {
						pt->from = ll_at(num);
					}
					addr--;
				}
				else {
					// TODO: error checking here. Invalid address caracter
				}
		}
	}
	return addr;
}

void eval(parse_t *pt) {
	printf("node from: %s", ll_s(pt->from));
	printf("node to: %s", ll_s(pt->to));
	printf("command : %c\n", pt->command);
	printf("arguments : %s", pt->argument);
#if 0

	switch(pt->command) {
		case 'a':
			ed_append(pt->from);
			break;
		case 'd':
			ed_delete(pt->from, pt->to);
			break;
		case 'c':
			ed_change(pt->from, pt->to);
			break;
		case 'e':
			if (pt->rest[0] == '!') 
				/* ed_edit(filename, cmd, force) */
				ed_edit(NULL, skipspaces(pt->rest+1), false);
			else
				ed_edit(pt->rest, NULL, false);
			break;
		case 'E':
			ed_edit(pt->rest, NULL, true);
			break;
		case 'w':
			if (pt->rest[0] == '!')
				ed_save(NULL, nextword(pt->rest), 0, 0);
			else if (pt->rest[0] == 'q')
				ed_save(state.filename, NULL, 1, 0);
			else if (isalnum(pt->rest[0]))
				ed_save(pt->rest, NULL, 0, 0);
			else
				ed_save(state.filename, NULL, 0, 0);
			break;
		case 'W':
			ed_save(pt->rest, NULL, 0, 1);
			break;
		case 'p':
			ed_print(pt->from, pt->to);
			break;
		case 'n':
			ed_printn(pt->from, pt->to);
			break;
		case '!':
			ed_shell(pt->rest, true);
			break;
		case 'q':
			ed_quit(false);
			break;
		case 'Q':
			ed_quit(true);
			break;
		case 's':
			ed_subs(pt->from, pt->to, pt->regex, pt->rest);
			break;
		case 'k':
			ed_mark(pt->from, pt->rest[0]);
			break;
		case 'r':
			if (pt->rest[0] == '!')
				ed_read(NULL, nextword(pt->rest), pt->from);
			else
				ed_read(pt->rest, NULL, pt->from);
			break;
		case 'j':
			ed_join(pt->from, pt->to);
			break;
		case '=':
			ed_equals(pt->from);
			break;
		case '#':
			ed_hash(pt->from);
			break;
		/* TODO: remove this temporary print func */
		case 't': 
			ll_print(gbl_head_node);
			break;
		case '\n':
			break;
		default:
			printf("Unimplemented Command\n");
	}
#endif
}
