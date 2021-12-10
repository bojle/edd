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
#include "undo.h"


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

	if (!parse_defaults) {
		int to_i = ll_node_index(pt->to);
		int from_i = ll_node_index(pt->from);
		if (to_i < from_i) {
			err_normal(&to_repl, "Invalid Address... did you mean %d,%d?\n", to_i, from_i);
		}
	}
	return addr;
}

node_t *pt_from(parse_t *pt) {
	return pt->from;
}

node_t *pt_to(parse_t *pt) {
	return pt->to;
}

char pt_command(parse_t *pt) {
	return pt->command;
}


parse_t *pt_make() {
	parse_t *pt = calloc(1, sizeof(*pt));
	pt->from = NULL;
	pt->to = NULL;
	pt->command = '\0';
	pt->argument = NULL;
	return pt;
}

void pt_set(parse_t *pt, node_t *from, node_t *to, char cmd, char *rest) {
	pt->from = from;
	pt->to = to;
	pt->command = cmd;
	pt->argument = rest;
}

/* 
 * Function pointer type for functions of of this prototype:
 * 		void foo(node_t *, node_t *, char *)
 */
typedef void (*fptr_t) (node_t *, node_t *, char *);

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
	fp_assign('y', ed_yank);
	fp_assign('x', ed_paste);
	fp_assign('s', ed_subs);
	fp_assign('g', ed_global);
	fp_assign('G', ed_global_interact);
	fp_assign('v', ed_global_invert);
	fp_assign('V', ed_global_interact_invert);
	fp_assign('u', ed_undo);
	fp_assign('U', ed_redo);
}
	
char *gbl_commands = "apndcmPif!eEjqQrkwW=#;tyxsgGvVuU\n";

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
}
