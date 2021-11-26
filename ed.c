#include <stdio.h>
#include <stdlib.h>
#include "ll.h"
#include "ed.h"
#include "io.h"

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
