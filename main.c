#include <stdlib.h>
#include "main.h"
//#include "ll.h"
//#include "io.h"
//#include "parse.h"



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
#if 0
	if (argc < 2) {
		fprintf(stderr, "Too few arguments\n");
		exit(EXIT_FAILURE);
	}
	ll_init();
	atexit(ll_free);

	io_load_file(argv[1]);
	ll_free();
#endif
}
