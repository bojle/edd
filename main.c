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
	int optindex = parse_args(argc, argv);
	ll_init();
	fptr_init();
	un_fptr_init();
	gbl_buffers_init();

	/* The FILE argument was provided */
	if (optindex < argc) {
		/* FILE is a shell command, prepare the string */
		if (argv[optindex][0] == '!') {
			set_command_buf(argv[optindex]);
			for (int i = optindex+1; i < argc; ++i) {
				append_command_buf(" ");
				append_command_buf(argv[i]);
			}
			edit_aux(get_command_buf());
		}
		else {
			edit_aux(argv[optindex]);
		}
	}
	repl();
}
