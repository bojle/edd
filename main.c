#include "ll.h"
#include "io.h"
#include "parse.h"

int main() {
	ll_init();
	io_load_file("man.txt");
	print_list(ll_at(0));
	ll_free();
}
