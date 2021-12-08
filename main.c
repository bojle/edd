#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include "ll.h"
#include "ed.h"
#include "parse.h"
#include "undo.h"
#include "io.h"

jmp_buf to_repl;

static char *repl_line = NULL;

static void free_repl_line() {
	free(repl_line);
}

void repl() {
	size_t linecap;
	atexit(free_repl_line);
	setjmp(to_repl);
	while (io_read_line(&repl_line, &linecap, stdin, get_prompt()) > 0) {
		eval(parse(repl_line));
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Too few arguments\n");
		exit(EXIT_FAILURE);
	}
	ll_init();
	fptr_init();
	un_fptr_init();
	gbl_buffers_init();

	argv[1] = "man.txt";
	ed_edit_force(NULL, NULL, argv[1]);
	repl();
}
