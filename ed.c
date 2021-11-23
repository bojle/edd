#include <stdio.h>
#include <stdlib.h>
#include "ll.h"
#include "ed.h"
#include "io.h"

void ed_append(node_t *from, node_t *to, char *rest) {
#if 0
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
#endif 
	printf("ed_apppend() has been called\n");
}

//void ed_subs(node_t *from, node_t *to, char *rest) {

	
