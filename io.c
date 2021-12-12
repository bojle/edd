#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

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
	remove_trailing_newlines(filename);
	FILE *fp = fopen(filename, mode);
	return fp;
}

FILE *shopen(char *cmd, char *mode) {
	remove_trailing_newlines(cmd);
	FILE *fp = popen(cmd, mode);
	return fp;
}

/*
 * ARGPARSE
 */

static const char *options_text = "Usage: edd [OPTIONS] [FILE]\n\n"
"Options:\n"
"-h       \tPrint this help message and exit\n"
"-E       \tUse \"Extended Regular Expressions\"\n"
"-p STRING\tSet interactive prompt to STRING\n"
"-r       \tRun edd in restricted mode\n"
"-s       \tSilent error messages and diagnostics";

static const char *more_information = "Try 'edd -h' for more information";



void options() {
	io_write_line(stdout, "%s\n", options_text);
}

_Bool opt_restricted = 0;
_Bool opt_silent = 0;
_Bool opt_extended = 0;

static const char *optstring = "hEp:rs";

int parse_args(int argc, char **argv) {
#if 0
	if (argc < 2) {
		io_write_line(stderr, "Too few arguments\n%s\n", more_information);
		exit(EXIT_FAILURE);
	}
#endif
	int opt;
	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
			case 'h':
				options();
				exit(EXIT_SUCCESS);
				break;
			case 'E':
				opt_extended = 1;
				break;
			case 'p':
				set_prompt(optarg);
				break;
			case 'r':
				opt_restricted = 1;
				break;
			case 's':
				opt_silent = 1;
				break;
			case '?':
				io_write_line(stderr, "Invalid Option: %c\n%s\n", optopt, more_information);
				exit(EXIT_FAILURE);
		}
	}
	return optind;
}
