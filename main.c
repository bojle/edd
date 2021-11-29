#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include "ll.h"
#include "ed.h"
#include "parse.h"
#include "io.h"

jmp_buf to_repl;


void repl() {
	char *line = NULL;
	size_t linecap;
	setjmp(to_repl);
	while (io_read_line(&line, &linecap, stdin, get_prompt()) > 0) {
		eval(parse(line));
	}
	free(line);
}


#if 0
void print_list(node_t *node) {
	for (;;) {
		io_write_line(stdout, ll_s(node));
		if (ll_next(node, 1) == global_tail())
			return;
		node = ll_next(node, 1);
	}
}
#endif

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Too few arguments\n");
		exit(EXIT_FAILURE);
	}
	ll_init();
	fptr_init();
	atexit(ll_free);
	argv[1] = "man.txt";
	io_load_file(fileopen(argv[1], "r"));
	repl();
	ll_free();
}
