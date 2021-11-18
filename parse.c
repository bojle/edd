#include "parse.h"
#include <stdbool.h>
#include <ctype.h>
#include <stddef.h>

// [address][command][arguments]

parse_t *parse(parse_t *pt, char *exp) {
	exp = parse_address(pt, exp);
	pt->command = *exp;
	exp++;
	pt->argument = skipspaces(exp);
	return pt;
}

char *skipspaces(char *s) {
	if (! isspace(*s))
		return s;
	while (isspace(*s))
		s++;
	return s;
}

int isaddresschar(char *a) {
	if (*a == '-' || *a == '+' || *a == '$' ||
		*a == '.' || *a == ',' || isdigit(*a) ||
		(isalpha(*a) && *(a-1) == '\''))
		return 1;
	return 0;
}


char *parse_address(parse_t *pt, char *addr) {
	bool commapassed = false;
	for (; isaddresschar(addr); addr++) {
		long num = 1;
		char *start = NULL;
		switch (*addr) {
			case '.':
				if (commapassed) { 
					pt->to = global_current();
				}
				else { 
					pt->from = global_current();
				}
				break;
			case '$':
				if (commapassed) { 
					pt->to = global_tail();
				}
				else { 
					pt->from = global_tail();
				}
				break;
			case ',': 
				if (!isaddresschar(addr+1)) { 
					 /* ,s/dog/cat/   -- here range is from head to tail
					  * this if block checks for such cases */
					pt->from = global_head();
				}
				commapassed = true;
				break;
			case '-':
				/* A minus sign, what follows is a number */
				if (isdigit(*(addr+1))) {
					num = strtol(addr + 1, &addr, 10);
				}

				if (commapassed) {
					pt->to = ll_prev(global_current(), num);
		   		}
				else {
					pt->from = ll_prev(global_current(), num);
				}
				addr--;
				break;
			case '+':
				/* analogous to the previous block for '-' */
				if (isdigit(*(addr+1))) {
					num = strtol(addr + 1, &addr, 10);
				}

				if (commapassed) {
					pt->to = ll_next(global_current(), num);
				}
				else {
					pt->from = ll_next(global_current(), num);
				}
				addr--;
				break;
			case ';':
				pt->from = global_current();
				pt->to = global_tail();
				break;
			case '/':
				start = addr+1;
				addr++;
				while (*addr != '/' && *(addr-1) != '\\') {
					addr++;
				}
				*addr = '\0';
				pt->regex = start;
				break;
			case '\'':
				//if ((pt->from = markget(*(addr+1))) == NULL) {
				//	io_err("Mark not set %c\n", *(addr+1));
				//}
				break;
			default:
				if (isdigit(*addr)) {
					num = strtol(addr, &addr, 10);
					if (commapassed) {
						pt->to = ll_at(num);
					}
					else {
						pt->from = ll_at(num);
					}
					addr--;
				}
				else {
					// TODO: error checking here. Invalid address caracter
				}
		}
	}
	return addr;
}
	
int main() {

}
