#include "io.h"
#include "ll.h"
#include <stdio.h>

ssize_t io_read_line(char **line, size_t *linecap, FILE *fp, char *prompt) {
	if (!prompt) {
		printf("%s", prompt);
	}

	ssize_t bytes_read = 0;

	if ((bytes_read = getline(line, linecap, fp)) < 0) {
		// TODO: err check
		return -1;
	}
	return bytes_read;
}


int io_write_line(FILE *fp, char *line) {
	// TODO: err check. if fp has write persmissions in append mode
	int bytes_read = fprintf(fp, "%s", line);
	fflush(fp);
	return bytes_read;
}


void io_load_file(char *filename) {
	FILE *fp = fileopen(filename, "r");

	char *line = NULL;
	size_t linecap;
	while (io_read_line(&line, &linecap, fp, NULL) > 0) {
		ll_add_next(global_current(), line);
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

FILE *fileopen(char *filename, char *mode) {
	FILE *fp = fopen(filename, mode);
	// TODO: err chekc
	return fp;
}

void print_list(node_t *node) {
	FILE *fp = fileopen("newfile.txt", "w");
	for (;;) {
		io_write_line(stdout, ll_s(node));
		if (ll_next(node, 1) == global_tail())
			return;
		node = ll_next(node, 1);
	}
}


