#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "aux.h"
#include "parse.h"
#include "ll.h"
#include "ed.h"
#include "io.h"
#include "err.h"

#define ED_PROMPT_SIZE 64
static char gbl_prompt[ED_PROMPT_SIZE] = ":";

#define ED_DEFAULT_FILENAME_SIZE 4096
static char gbl_default_filename[ED_DEFAULT_FILENAME_SIZE];

static char *ed_shell_command_buf;
static size_t ed_shell_command_buf_sz;

static _Bool gbl_saved = 0;

void set_prompt(char *s) {
	size_t sz = strlen(s);
	if (sz >= ED_PROMPT_SIZE) {
		err_normal(&to_repl, 
				"ERR: Prompt string should not be more than %d characters\n", 
				ED_PROMPT_SIZE-1);
	}
	strncpy(gbl_prompt, s, sz); 
	if (gbl_prompt[sz - 1] == '\n') {
		gbl_prompt[sz - 1] = '\0';
	}
}

char *get_prompt() {
	return gbl_prompt;
}

void set_default_filename(char *s) {
	size_t size = strlen(s);
	if (size >= ED_DEFAULT_FILENAME_SIZE) {
		err_normal(&to_repl, 
				"ERR: Filename should not be more than %d characters\n", 
				ED_DEFAULT_FILENAME_SIZE-1);
	}
	strncpy(gbl_default_filename, s, size);
	if (gbl_default_filename[size - 1] == '\n') {
		gbl_default_filename[size - 1] = '\0';
	}
}

char *get_default_filename() {
	return gbl_default_filename;
}

void set_command_buf(char *cmd) {
	if (cmd == ed_shell_command_buf) {
		return;
	}

	size_t sz = strlen(cmd);
	if (sz > ed_shell_command_buf_sz) {
		ed_shell_command_buf = realloc(ed_shell_command_buf, sz);
		ed_shell_command_buf_sz = sz;
	}
	strncpy(ed_shell_command_buf, cmd, sz - 1);
	ed_shell_command_buf[sz - 1] = '\0';
}

char *get_command_buf() {
	return ed_shell_command_buf;
}

void ed_append(node_t *from, node_t *to, char *rest) {
	from = (from == global_tail() ? ll_last_node() : from);
	char *line = NULL;
	size_t bytes = 0;
	size_t lines = 0;
	size_t linecap;
	while ((bytes = io_read_line(&line, &linecap, stdin, NULL)) > 0) {
		if (line[0] == '.')
			break;
		from = ll_add_next(from, line);
		lines++;
		bytes += bytes;
	}
	printf("%ld line%s appended\n", lines, (lines==1)?"":"s");
	free(line);
	gbl_saved = 0;
}

void ed_insert(node_t *from, node_t *to, char *rest) {
	from = (from == global_tail() ? ll_last_node() : from);
	char *line = NULL;
	size_t bytes = 0;
	size_t lines = 0;
	size_t linecap;
	/* This line makes ed_insert different from ed_append */
	from = ll_prev(from, 1);
	while ((bytes = io_read_line(&line, &linecap, stdin, NULL)) > 0) {
		if (line[0] == '.')
			break;
		from = ll_add_next(from, line);
		lines++;
		bytes += bytes;
	}
	printf("%ld line%s inserted\n", lines, (lines==1)?"":"s");
	free(line);
	gbl_saved = 0;
}

void ed_print(node_t *from, node_t *to, char *rest) {
	to = (to == global_tail() ? to : ll_next(to, 1));
	from = (from == global_tail() ? ll_prev(from, 1) : from);
	while (from != to) {
		io_write_line(stdout, "%s", ll_s(from));
		from = ll_next(from, 1);
	}
}

void ed_print_n(node_t *from, node_t *to, char *rest) {
	to = (to == global_tail() ? to : ll_next(to, 1));
	from = (from == global_tail() ? ll_prev(from, 1) : from);
	size_t n = 1;
	while (from != to) {
		io_write_line(stdout, "%ld\t%s", n, ll_s(from));
		from = ll_next(from, 1);
		n++;
	}
}

void ed_delete(node_t *from, node_t *to, char *rest) {
	to = (to == global_tail() ? to : ll_next(to, 1));
	from = (from == global_tail() ? ll_prev(from, 1) : from);
	
	while (from != to) {
		from = ll_remove_node(from);
	}	
	gbl_saved = 0;
}

void ed_change(node_t *from, node_t *to, char *rest) {
	ed_delete(from, to, rest);
	ed_append(global_current(), NULL, NULL);
	gbl_saved = 0;
}

void ed_move(node_t *from, node_t *to, char *rest) {
	from = (from == global_head() ? ll_first_node() : from);

	parse_t *pt = pt_make();
	parse_address(pt, rest);
	node_t *move_to = pt_from(pt);
	free(pt);

	move_to = (move_to == global_tail() ? ll_last_node(): move_to);
	node_t *move_to_subsequent = ll_next(move_to, 1);

	/* 
	 * from->prev <-> to->next
	 * move_to <-> from 
	 * to <-> move_to_subsequent
	 */	

	ll_attach_nodes(ll_prev(from, 1), ll_next(to, 1));
	ll_attach_nodes(move_to, from);
	ll_attach_nodes(to, move_to_subsequent);	
	gbl_saved = 0;
}

void ed_newline(node_t *from, node_t *to, char *rest) {
	from = (from == global_tail() ? ll_last_node() : from);
	ed_print(from, from, rest);
}

void ed_prompt(node_t *from, node_t *to, char *rest) {
	set_prompt(rest);
}	

void ed_shell(node_t *from, node_t *to, char *rest) {
	if (*rest == '!') {
		rest = get_command_buf();
	}
	FILE *fp = shopen(rest, "r");
	io_write_line(stdout, "%s\n", get_command_buf());

	char *line = NULL;
	size_t linecap;
	while (io_read_line(&line, &linecap, fp, NULL) > 0) {
		io_write_line(stdout, "%s", line);
	}
	io_write_line(stdout, "!\n");
	free(line);
	pclose(fp);
}

void ed_edit(node_t *from, node_t *to, char *rest) {
	if (!gbl_saved) {
		err_normal(&to_repl, "%s", 
				"No write since last change. Use 'E' to override or save changes.\n");
	}
	FILE *fp;
	if (*rest == '!') {
		rest++;
		rest = skipspaces(rest);
		if (*rest == '!') {
			rest = get_command_buf();
		}
		fp = shopen(rest, "r");
	}
	else {
		fp = fileopen(rest, "r");
	}
	node_t *node = ll_first_node();
	while (node != global_tail()) {
		node = ll_remove_node(node);
	}
	io_load_file(fp);
	gbl_saved = 1;
}

void ed_edit_force(node_t *from, node_t *to, char *rest) {
	gbl_saved = 1;
	printf("inside edit_force\n");
	ed_edit(from, to, rest);
}


void ed_file(node_t *from, node_t *to, char *rest) {
	printf("inside ed_file\n");
	if (!isalnum(*rest)) {
		io_write_line(stdout, "%s\n", get_default_filename());
		return;
	}
	set_default_filename(rest);
}

void ed_join(node_t *from, node_t *to, char *rest) {
	node_t *from_next = ll_next(from, 1);
	to = (to == global_tail() ? to : ll_next(to, 1));

	while (from_next != to) {
		ll_join_nodes(from, from_next);
		from_next = ll_next(from, 1);
	}
	gbl_saved = 0;
}

