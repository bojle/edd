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

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Too few arguments\n");
		exit(EXIT_FAILURE);
	}
	ll_init();
	fptr_init();
	atexit(ll_free);
	argv[1] = "man.txt";
	ed_edit_force(NULL, NULL, argv[1]);
	repl();

	free(get_command_buf());
	ll_free();
}
