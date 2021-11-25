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

	if (from == global_head()) {
		from = ll_next(global_head(), 1);
	}
	to = ll_next(to, 1);
	while (from != to) {
		io_write_line(stdout, "%s", ll_s(from));
		from = ll_next(from, 1);
	}
}
