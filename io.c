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
	char *line = NULL;
	size_t linecap;
	node_t *node = global_head();
	while (io_read_line(&line, &linecap, fp, NULL) > 0) {
		node = ll_add_next(node, line);
	}
	free(line);
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
	re_t *re = re_make();
	parse_regex(re, "[^\\]%");
	filename = re_replace(re, filename, get_default_filename());
	free(re);
	return filename;
}


FILE *fileopen(char *filename, char *mode) {
	size_t sz = strlen(filename);
	if (filename[sz - 1] == '\n') {
		filename[sz - 1] = '\0';
	}
	FILE *fp = fopen(filename, mode);

	if (fp == NULL) {
		err_normal(&to_repl, "%s: %s\n", strerror(errno), filename);
	}
	return fp;
}

FILE *shopen(char *cmd, char *mode) {
	size_t sz = strlen(cmd);
	if (cmd[sz - 1] == '\n') {
		cmd[sz - 1] = '\0';
	}
	FILE *fp = popen(cmd, mode);
	if (fp == NULL) {
		err(NULL, strerror(errno));
	}
	return fp;
}
