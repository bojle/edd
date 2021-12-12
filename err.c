#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "io.h"
#include "err.h"

void err_normal(jmp_buf *buf, const char *fmt, ...) {
	if (opt_silent) {
		goto end;
	}
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
end:
	if (buf != NULL) {
		longjmp(*buf, 1); 
	}
}


void err_fatal(const char *fmt, ...) {
	if (opt_silent) {
		goto end;
	}
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
end:
	exit(EXIT_FAILURE);
}	

char *regerror_aux(int errcode, regex_t *reg) {
	static char reg_str[128];
	regerror(errcode, reg, reg_str, 128);
	regfree(reg);
	return reg_str;
}
