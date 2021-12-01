#include "parse.h"
#include <stdio.h>
#include "ed.h"
#include "ll.h"
#include "err.h"
#include <stdbool.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "aux.h"


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
};

static parse_t pt;

_Bool parse_defaults = 0;

parse_t *parse(char *exp) {
	/* defaults */
	pt.from = global_current();
	pt.to = ll_last_node();
	pt.command = '\0';
	pt.argument = NULL;
	parse_defaults = 1;

	exp = parse_address(&pt, exp);
	exp = skipspaces(exp);
	pt.command = *exp++;
	pt.argument = skipspaces(exp);
	return &pt;
}

int isaddresschar(char *a) {
	if (*a == '-' || *a == '+' || *a == '$' ||
		*a == '.' || *a == ',' || isdigit(*a) ||
		(isalpha(*a) && *(a-1) == '\'') || *a == '/' || *a == '\'')
		return 1;
	return 0;
}


/* 
 * populate the 'from' and 'to' pointers of a parse_t 
 * object, return a pointer to the expression where the 
 * address ends
 *
 * Addresses -> correspondence
 * n (where n is an integer) -> nth line of the file
 * $ -> gbl_tail_node
 * . -> gbl_current_node
 * , -> 1st node to last node
 *
 */
char *parse_address(parse_t *pt, char *addr) {
	bool commapassed = false;
	regex_t reg;
	int digits_encountered = 0;
	for (; isaddresschar(addr); addr++) {
		long num = 1;
		char *start = NULL;
		parse_defaults = 0;
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
					pt->to = ll_last_node();
				}
				if (!isaddresschar(addr-1)) {
					pt->from = ll_first_node();
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
				pt->to = ll_last_node();
				break;
			case '/':
				start = addr+1;
				addr++;
				while (*addr != '/' && *(addr-1) != '\\') {
					addr++;
				}
				*addr = '\0';
				if ((num = regcomp(&reg, start, REG_EXTENDED)) != 0) {
					err(&to_repl, regerror_aux(num, &reg));
				}
				pt->from = pt->to = ll_reg_next(global_head(), &reg);
				regfree(&reg);
				break;
			case '\'':
				if (get_mark(*(addr+1)) == NULL) {
					err_normal(&to_repl, "No mark set at: %c", *(addr+1));
				}
				pt->from = get_mark(*(addr+1));
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
					digits_encountered++;
				}
				else {
					// TODO: error checking here. Invalid address caracter
				}
		}
	}
	if (digits_encountered == 1) {
		if (!commapassed) {
			pt->to = pt->from;
		}
	}



	return addr;
}

node_t *pt_from(parse_t *pt) {
	return pt->from;
}

parse_t *pt_make() {
	return calloc(1, sizeof(parse_t));
}


/* 
 * Function pointer type for functions of of this prototype:
 * 		void foo(node_t *, node_t *, char *)
 */
typedef void (*fptr_t) (node_t *, node_t *, char *);

#define FIRST_ASCII_CHAR '!'
#define LAST_ASCII_CHAR 'z'
/* Size of fptr_table; accomodates all the characters that could be a command */
#define FPTR_ARRAY_SIZE (LAST_ASCII_CHAR - FIRST_ASCII_CHAR + 1) 
static fptr_t fptr_table[FPTR_ARRAY_SIZE];

static int fp_hash(char c) {
	return c - FIRST_ASCII_CHAR;
}

static void fp_assign(char c, fptr_t fn) {
	fptr_table[fp_hash(c)] = fn;
}

void fptr_init() {
	fp_assign('a', ed_append);
	fp_assign('p', ed_print);
	fp_assign('n', ed_print_n);
	fp_assign('d', ed_delete);
	fp_assign('c', ed_change);
	fp_assign('m', ed_move);
	fp_assign('\n', ed_newline);
	fp_assign('P', ed_prompt);
	fp_assign('i', ed_insert);
	fp_assign('f', ed_file);
	fp_assign('!', ed_shell);
	fp_assign('e', ed_edit);
	fp_assign('E', ed_edit_force);
	fp_assign('j', ed_join);
	fp_assign('q', ed_quit);
	fp_assign('Q', ed_quit_force);
	fp_assign('r', ed_read);
	fp_assign('k', ed_mark);
	fp_assign('w', ed_write);
	fp_assign('W', ed_write_append);
	fp_assign('=', ed_equals);
	fp_assign('#', ed_comment);
	fp_assign(';', ed_semicolon);
	fp_assign('t', ed_transfer);
}
	
static char *gbl_commands = "apndcmPif!eEjqQrkwW=#;t\n";

void eval(parse_t *pt) {
	printf("node from: %s", ll_s(pt->from));
	printf("node to: %s", ll_s(pt->to));
	printf("command : %c\n", pt->command);
	printf("arguments : %s\n", pt->argument);
	printf("size: %ld\n", ll_node_size(pt->from));

	if (strchr(gbl_commands, pt->command) == NULL) {
		err_normal(&to_repl, "%s: %c\n", "Invalid Command", pt->command);
	}
	fptr_table[fp_hash(pt->command)](pt->from, pt->to, pt->argument);
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
