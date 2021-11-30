#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "io.h"
#include "ll.h"
#include "err.h"
#include "ed.h"
#include "aux.h"

#include <errno.h>
#include <string.h>

#define ED_FLUSH_OUTPUT 1

ssize_t io_read_line(char **line, size_t *linecap, FILE *fp, char *prompt) {
	if (prompt != NULL) {
		printf("%s", prompt);
	}
	ssize_t bytes_read = 0;
	if ((bytes_read = getline(line, linecap, fp)) < 0) {
		return -1;
	}
	return bytes_read;
}


int io_write_line(FILE *fp, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int bytes_read = vfprintf(stderr, fmt, ap);
	va_end(ap);
#if (ED_FLUSH_OUTPUT == 1)
	fflush(fp);
#endif
	return bytes_read;
}


void io_load_file(FILE *fp) {
	//FILE *fp = fileopen(filename, "r");

	char *line = NULL;
	size_t linecap;
	node_t *node = global_head();
	while (io_read_line(&line, &linecap, fp, NULL) > 0) {
		node = ll_add_next(node, line);
	}
	free(line);
	fclose(fp);
}

void io_write_file(char *filename) {
	FILE *fp = fileopen(filename, "a");

	node_t *node = ll_next(global_head(), 1);
	for (;;) {
		io_write_line(fp, ll_s(node));
		if (ll_next(node, 1) == global_tail()) {
			break;
		}
	}
	fclose(fp);
}

char *parse_filename(char *filename) {
	regex_t rt;
	int err;

	/* Replace all unescaped '%' in 'cmd' with the default filename */
	if ((err = regcomp(&rt, "[^\\]%", 0)) != 0) {
		err(&to_repl, regerror_aux(err, &rt));
	}
	filename = strrep(filename, &rt, get_default_filename(), 1);
	return filename;
}


FILE *fileopen(char *filename, char *mode) {
	FILE *fp = fopen(filename, mode);

	if (fp == NULL) {
		err(&to_repl, strerror(errno));
	}
	return fp;
}

FILE *shopen(char *cmd, char *mode) {
	FILE *fp = popen(cmd, mode);
	if (fp == NULL) {
		err(NULL, strerror(errno));
	}
	return fp;
}
