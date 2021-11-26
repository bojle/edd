#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "aux.h"
#include "parse.h"
#include "ll.h"
#include "ed.h"
#include "io.h"

static char gbl_prompt[64] = ":";

void set_prompt(char *s) {
	strncpy(gbl_prompt, s, strlen(s)-1); // -1 to not include the trailing newline
}

char *get_prompt() {
	return gbl_prompt;
}

void ed_append(node_t *from, node_t *to, char *rest) {
	char *line = NULL;
	size_t bytes = 0;
	size_t lines = 0;
	size_t linecap;
	from = ll_prev(from, 1);
	while ((bytes = io_read_line(&line, &linecap, stdin, NULL)) > 0) {
		if (line[0] == '.')
			break;
		from = ll_add_next(from, line);
		lines++;
		bytes += bytes;
	}
	printf("%ld line%s appended\n", lines, (lines==1)?"":"s");
	free(line);
}

void ed_print(node_t *from, node_t *to, char *rest) {
	to = (to == global_tail() ? to : ll_next(to, 1));
	from = (from == global_tail() ? ll_prev(from, 1) : from);
	while (from != to) {
		io_write_line(stdout, "%s", ll_s(from));
		from = ll_next(from, 1);
	}
}

void ed_print_n(node_t *from, node_t *to, char *rest) {
	to = (to == global_tail() ? to : ll_next(to, 1));
	from = (from == global_tail() ? ll_prev(from, 1) : from);
	size_t n = 1;
	while (from != to) {
		io_write_line(stdout, "%ld\t%s", n, ll_s(from));
		from = ll_next(from, 1);
		n++;
	}
}

void ed_delete(node_t *from, node_t *to, char *rest) {
	to = (to == global_tail() ? to : ll_next(to, 1));
	from = (from == global_tail() ? ll_prev(from, 1) : from);
	
	while (from != to) {
		from = ll_remove_node(from);
	}	
}

void ed_change(node_t *from, node_t *to, char *rest) {
	ed_delete(from, to, rest);
	ed_append(global_current(), NULL, NULL);
}

void ed_move(node_t *from, node_t *to, char *rest) {
	from = (from == global_head() ? ll_first_node() : from);

	parse_t *pt = pt_make();
	parse_address(pt, rest);
	node_t *move_to = pt_from(pt);
	free(pt);

	move_to = (move_to == global_tail() ? ll_last_node(): move_to);
	node_t *move_to_subsequent = ll_next(move_to, 1);

	/* 
	 * from->prev <-> to->next
	 * move_to <-> from 
	 * to <-> move_to_subsequent
	 */	

	ll_attach_nodes(ll_prev(from, 1), ll_next(to, 1));
	ll_attach_nodes(move_to, from);
	ll_attach_nodes(to, move_to_subsequent);	
}

void ed_newline(node_t *from, node_t *to, char *rest) {
	from = (from == global_tail() ? ll_last_node() : from);
	ed_print(from, from, rest);
}

void ed_prompt(node_t *from, node_t *to, char *rest) {
	set_prompt(rest);
}	

