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
	longjmp(*buf, 1); 
}


void err_fatal(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
	exit(EXIT_FAILURE);
}	
