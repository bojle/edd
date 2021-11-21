#ifndef ERR_H
#define ERR_H

#include <setjmp.h>

extern jmp_buf to_repl;

/*
 * jmpbuf - type jmp_buf, for non-local jump
 * reason - type char *. strerror(errno) can be used here
 */
#define err(jmpbuf, reason) (err_normal(jmpbuf, "%s: In function '%s'\nLine %d: %s",\
			__FILE__, __func__, __LINE__, reason))

#define die(reason) (err_fatal("%s: In function '%s'\nLine %d: %s", __FILE__, __func__,\
			__LINE__, reason))

void err_normal(jmp_buf *buf, const char *fmt, ...);
void err_fatal(const char *fmt, ...);

#endif
