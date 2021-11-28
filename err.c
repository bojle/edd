#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "err.h"

void err_normal(jmp_buf *buf, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
	if (buf != NULL) {
		longjmp(*buf, 1); 
	}
}


void err_fatal(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
	exit(EXIT_FAILURE);
}	

char *regerror_aux(int errcode, regex_t *reg) {
	static char reg_str[128];
	regerror(errcode, reg, reg_str, 128);
	regfree(reg);
	return reg_str;
}
