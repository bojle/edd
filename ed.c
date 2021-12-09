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
#include "undo.h"

#define ED_PROMPT_SIZE 64
static char gbl_prompt[ED_PROMPT_SIZE] = ":";

#define ED_DEFAULT_FILENAME_SIZE 4096
static char gbl_default_filename[ED_DEFAULT_FILENAME_SIZE] = "man.txt";

static _Bool gbl_saved = 1;

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
	strncpy(gbl_default_filename, s, size+1);
	if (gbl_default_filename[size - 1] == '\n') {
		gbl_default_filename[size - 1] = '\0';
	}
}

char *get_default_filename() {
	if (gbl_default_filename[0] == '\0') {
		return NULL;
	}
	return gbl_default_filename;
}


static ds_t *gbl_command_buf;
static yb_t *gbl_yank_buf;
static re_t *gbl_re;
static yb_t *gbl_global_cmd_buf;

void set_command_buf(char *cmd) {
	if (cmd == ds_get_s(gbl_command_buf)) {
		return;
	}
	ds_set(gbl_command_buf, cmd);
}

char *get_command_buf() {
	return ds_get_s(gbl_command_buf);
}

void gbl_buffers_init() {
	gbl_command_buf = ds_make();
	gbl_yank_buf = yb_make();
	gbl_re = re_make();
	gbl_global_cmd_buf = yb_make();
}

void gbl_buffers_free() {
	ds_free(gbl_command_buf);
	free(gbl_command_buf);

	yb_free(gbl_yank_buf);
	free(gbl_yank_buf);

	re_free(gbl_re);
	free(gbl_re);

	yb_free(gbl_global_cmd_buf);
	free(gbl_global_cmd_buf);
}

/* Yank Buffer Functions */
void yank_buf_push(char *s) {
	yb_append(gbl_yank_buf, s);
}

char *yank_buf_get(int at) {
	return yb_at(gbl_yank_buf, at);
}

void yank_buf_clear() {
	yb_clear(gbl_yank_buf);
}

/* Mark functions */

#define FIRST_MARK '!'
#define LAST_MARK '~'
#define MARK_LIM (LAST_MARK - FIRST_MARK)
node_t *gbl_marks[MARK_LIM];

int set_mark(node_t *node, int at) {
	if (at < FIRST_MARK || at > LAST_MARK) {
		err_normal(&to_repl, "Cannot set mark to '%c'." 
				" Marks must be one of the characters between %c and %c"
				" in the ASCII table (see `man ascii`)\n", at, FIRST_MARK,
				LAST_MARK);
	}
	int i = at - FIRST_MARK;
	gbl_marks[i] = node;
	return i;
}

node_t *get_mark(int at) {
	if (at < FIRST_MARK || at > LAST_MARK) {
		err_normal(&to_repl, "Invalid Mark: %c", at);
	}
	return gbl_marks[at - FIRST_MARK];
}

void clear_mark(int at) {
	gbl_marks[at - FIRST_MARK] = NULL;
}


/* ed_ functions */

static size_t append_aux(node_t *from) {
	from = (from == global_tail() ? ll_last_node() : from);
	char *line = NULL;
	size_t bytes = 0;
	size_t lines = 0;
	size_t linecap;
	push_to_append_buf(&brake);
	while ((bytes = io_read_line(&line, &linecap, stdin, NULL)) > 0) {
		if (line[0] == '.')
			break;
		from = ll_add_next(from, line);
		push_to_append_buf(from);
		lines++;
		bytes += bytes;
	}
	free(line);
	return lines;
}


void ed_append(node_t *from, node_t *to, char *rest) {
	push_to_undo_buf('a');
	size_t lines = append_aux(from);
	io_write_line(stdout, "%ld line%s appended\n", lines, (lines==1)?"":"s");
	gbl_saved = 0;
}

void ed_insert(node_t *from, node_t *to, char *rest) {
	push_to_undo_buf('i');
	from = (from == global_tail() ? ll_last_node() : from);
	size_t lines = append_aux(ll_prev(from, 1));
	io_write_line(stdout, "%ld line%s inserted\n", lines, (lines==1)?"":"s");
	gbl_saved = 0;
}

void ed_print(node_t *from, node_t *to, char *rest) {
	if (parse_defaults) {
		io_write_line(stdout, "%s", ll_s(global_current()));
		return;
	}

	to = (to == global_tail() ? to : ll_next(to, 1));
	from = (from == global_tail() ? ll_prev(from, 1) : from);

	while (from != to) {
		io_write_line(stdout, "%s", ll_s(from));
		from = ll_next(from, 1);
	}
}

void ed_print_n(node_t *from, node_t *to, char *rest) {
	if (parse_defaults) {
		io_write_line(stdout, "%s", ll_s(global_current()));
		return;
	}
	to = (to == global_tail() ? to : ll_next(to, 1));
	from = (from == global_tail() ? ll_prev(from, 1) : from);

	size_t n = ll_node_index(from);
	while (from != to) {
		io_write_line(stdout, "%ld\t%s", n, ll_s(from));
		from = ll_next(from, 1);
		n++;
	}
}

static size_t delete_aux(node_t *from, node_t *to) {
	to = (to == global_tail() ? to : ll_next(to, 1));
	from = (from == global_tail() ? ll_prev(from, 1) : from);
	
	size_t lines = 0;
	push_to_delete_buf(&brake);
	while (from != to) {
		push_to_delete_buf(from);
		from = ll_remove_shallow(from);
		lines++;
	}	
	return lines;
}

void ed_delete(node_t *from, node_t *to, char *rest) {
	push_to_undo_buf('d');
	delete_aux(from, to);
	gbl_saved = 0;
}

void ed_change(node_t *from, node_t *to, char *rest) {
	push_to_undo_buf('c');
	size_t lines = delete_aux(from, to);
	append_aux(ll_prev(from, 1));
	io_write_line(stdout, "%ld line%s changed\n", lines, (lines==1)?"":"s");
	gbl_saved = 0;
}


void ed_move(node_t *from, node_t *to, char *rest) {
	push_to_undo_buf('m');
	from = (from == global_head() ? ll_first_node() : from);

	parse_t *pt = pt_make();
	parse_address(pt, rest);
	node_t *move_to = pt_from(pt);
	free(pt);

	move_to = (move_to == global_tail() ? ll_last_node(): move_to);
	node_t *move_to_subsequent = ll_next(move_to, 1);

	push_to_append_buf(&brake);
	push_to_append_buf(ll_prev(from, 1));
	push_to_append_buf(from);
	push_to_append_buf(to);
	push_to_append_buf(ll_next(to, 1));
	push_to_append_buf(move_to);
	push_to_append_buf(move_to_subsequent);
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
	if (parse_defaults) {
		ll_set_current_node(from = ll_next(from, 1));
	}
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
	set_command_buf(rest);
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

void edit_aux(char *rest) {
	if (*rest == '\n' || *rest == '\0') {
		err_normal(&to_repl, "%s\n", "Error: No arguments");
	}
	rest = parse_filename(rest);

	FILE *fp;
	_Bool frompipe = 0;
	if (*rest == '!') {
		rest++;
		rest = skipspaces(rest);
		if (*rest == '!') {
			rest = get_command_buf();
		}
		fp = shopen(rest, "r");
		set_command_buf(rest);
		frompipe = 1;
	}
	else {
		set_default_filename(rest);
		fp = fileopen(get_default_filename(), "r");
	}
	/* Remove existing nodes */
	node_t *node = ll_first_node();
	while (node != global_tail()) {
		node = ll_remove_node(node);
	}
	/* Load new nodes */
	io_load_file(fp);
	frompipe == 1 ? pclose(fp) : fclose(fp);
	free(rest);
}

void ed_edit(node_t *from, node_t *to, char *rest) {
	push_to_undo_buf('e');
	push_to_delete_buf(&brake);
	push_to_delete_buf(ll_make_node(NULL, get_default_filename(), NULL));

	if (!gbl_saved) {
		err_normal(&to_repl, "%s", 
				"No write since last change. Use 'E' to override or save changes.\n");
	}
	edit_aux(rest);
	gbl_saved = 1;
}

void ed_edit_force(node_t *from, node_t *to, char *rest) {
	gbl_saved = 1;
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

void ed_quit(node_t *from, node_t *to, char *rest) {
	if (!gbl_saved) {
		err_normal(&to_repl, "%s", 
				"No write since last change." 
				" Use 'Q' to quit without saving or save changes.\n");
	}

	ll_free();
	gbl_buffers_free();
	exit(EXIT_SUCCESS);
}

void ed_quit_force(node_t *from, node_t *to, char *rest) {
	gbl_saved = 1;
	ed_quit(from, to, rest);
}

void ed_read(node_t *from, node_t *to, char *rest) {
	FILE *fp;
	_Bool frompipe = 0;
	if (*rest == '!') {
		rest++;
		rest = skipspaces(rest);
		fp = shopen(rest, "r");
		frompipe = 1;
	}
	else {
		fp = fileopen(rest, "r");
	}

	char *line = NULL;
	size_t linecap;

	while (io_read_line(&line, &linecap, fp, NULL) > 0) {
		from = ll_add_next(from, line);
	}
	free(line);
	frompipe == 1 ? pclose(fp) : fclose(fp);
}


void ed_mark(node_t *from, node_t *to, char *rest) {
	set_mark(from, *rest);
}

void ed_write(node_t *from, node_t *to, char *rest) {
	FILE *fp;
	_Bool quit = 0;
	_Bool frompipe = 0;
	if (*rest == '!') {
		fp = shopen(skipspaces(++rest), "w");
		frompipe = 1;
	}
	else { 
		if (*rest == 'q') {
			quit = 1;
			rest = skipspaces(++rest);
		}
		if (!isalnum(*rest))  {
			if (get_default_filename() == NULL) {
				err_normal(&to_repl, "No default filename set. "
					"Set a default filename or provide a file in the command\n");
			}
			rest = get_default_filename();
		}
		if (get_default_filename() == NULL) {
			set_default_filename(rest);
		}
		fp = fileopen(rest, "w");
	}
	

	if (parse_defaults) {
		from = ll_first_node();
		to = ll_last_node();
	}

	to = (to == global_tail() ? to : ll_next(to, 1));
	while (from != to) {
		fprintf(fp, "%s", ll_s(from));
		from = ll_next(from, 1);
	}
	gbl_saved = 1;
	frompipe == 1 ? pclose(fp) : fclose(fp);
	if (quit) {
		ed_quit(NULL, NULL, NULL);
	}
}

void ed_write_append(node_t *from, node_t *to, char *rest) {
	FILE *fp;
	_Bool quit = 0;
	_Bool frompipe = 0;
	if (*rest == 'q') {
		quit = 1;
		rest = skipspaces(++rest);
	}
	if (!isalnum(*rest))  {
		if (get_default_filename() == NULL) {
			err_normal(&to_repl, "No default filename set. "
				"Set a default filename or provide a file in the command\n");
		}
		rest = get_default_filename();
	}
	if (get_default_filename() == NULL) {
		set_default_filename(rest);
	}
	fp = fileopen(rest, "a");
	if (parse_defaults) {
		from = ll_first_node();
		to = ll_last_node();
	}

	to = (to == global_tail() ? to : ll_next(to, 1));
	while (from != to) {
		fprintf(fp, "%s", ll_s(from));
		from = ll_next(from, 1);
	}
	gbl_saved = 1;
	frompipe == 1 ? pclose(fp) : fclose(fp);
	if (quit) {
		ed_quit(NULL, NULL, NULL);
	}
	
}

void ed_equals(node_t *from, node_t *to, char *rest) {
	if (parse_defaults) {
		from = ll_last_node();
	}
	from = (from == global_tail() ? ll_last_node() : from);
	io_write_line(stdout, "%d\n", ll_node_index(from));
}

void ed_comment(node_t *from, node_t *to, char *rest) {
}

void ed_semicolon(node_t *from, node_t *to, char *rest) {
	ll_set_current_node(from);
}

void ed_transfer(node_t *from, node_t *to, char *rest) {
	from = (from == global_head() ? ll_first_node() : from);
	to = (to == global_tail() ? ll_last_node() : ll_next(to, 1));

	parse_t *pt = pt_make();
	parse_address(pt, rest);
	node_t *move_to = pt_from(pt);
	free(pt);

	move_to = (move_to == global_tail() ? ll_last_node(): move_to);
	//node_t *move_to_subsequent = ll_next(move_to, 1);

	while (from != to) {
		move_to = ll_add_next(move_to, ll_s(from));
		from = ll_next(from, 1);
	}

	gbl_saved = 0;
}

void ed_yank(node_t *from, node_t *to, char *rest) {
	from = (from == global_head() ? ll_first_node() : from);
	to = (to == global_tail() ? ll_last_node() : ll_next(to, 1));

	yank_buf_clear();
	while (from != to) {
		yank_buf_push(ll_s(from));
		from = ll_next(from, 1);
	}
}


void ed_paste(node_t *from, node_t *to, char *rest) {
	for (int i = 0; i < (int)yb_nmembs(gbl_yank_buf); ++i) {
		from = ll_add_next(from, yank_buf_get(i));
	}
}

/*
 * 'rest' should be of the form: 		
 * 		[delimiter][RE][delimiter][SUBS][delimiter][TAIL]
 * 								OR
 *      A count suffix N or any permutation of characters r,g,p.
 *
 * 		Replace from->s to a calloc'ated string with all characters 
 * 		that match [RE], replaced with [SUBS].
 *
 * 		See manual for more.
 *
 * re_replace() or any other function in the chain shall not deallocate anything,
 * deallocation is relegated to the user of this function.
 */

int gbl_total_substitutions = 0;

void ed_subs(node_t *from, node_t *to, char *rest) {
	if (parse_defaults) {
		from = ll_first_node();
		to = ll_last_node();
	}
	from = (from == global_head() ? ll_first_node() : from);
	to = (to == global_tail() ? to : ll_next(to, 1));

	char *subst;
	char *tail;
	if (*rest == '\n' || isdigit(*rest) || *rest == 'r' || *rest == 'g' || *rest == 'p') {
		if (re_has_subst(gbl_re) == 0) {
			err_normal(&to_repl, "%s\n", "No previous substitutions");
		}
		subst = ds_get_s(re_get_subst(gbl_re));
		tail = rest;
		parse_tail_alt(gbl_re, tail);
		goto end;
	}

	char delimiter = *rest++;
	char *regex = rest;
	subst = next_unescaped_delimiter(regex, delimiter);
	tail = next_unescaped_delimiter(subst, delimiter);

	parse_regex(gbl_re, regex);
	parse_tail(gbl_re, tail);
end:
	while (from != to) {
		ll_set_s(from, re_replace(gbl_re, ll_s(from), subst));
		from = ll_next(from, 1);
	}
	gbl_saved = 0;
}


void ed_global(node_t *from, node_t *to, char *rest) {
	regex_t reg;
	rest = parse_global_command(&reg, rest);

	if (parse_defaults) {
		from = ll_first_node();
		to = ll_last_node();
	}
	from = (from == global_head() ? ll_first_node() : from);
	to = (to == global_tail() ? to : ll_next(to, 1));

	node_t *node;
	read_command_list(gbl_global_cmd_buf, rest);
	while (from != to) {
		node = ll_reg_next(from, &reg);	
		if (node == NULL) {
			break;
		}
		execute_command_list(gbl_global_cmd_buf, node);
		from = ll_next(node, 1);
	}
	gbl_saved = 0;
}

void ed_global_interact(node_t *from, node_t *to, char *rest) {
	regex_t reg;
	rest = parse_global_command(&reg, rest);

	if (parse_defaults) {
		from = ll_first_node();
		to = ll_last_node();
	}
	from = (from == global_head() ? ll_first_node() : from);
	to = (to == global_tail() ? to : ll_next(to, 1));

	node_t *node;
	rest[strlen(rest) - 2] = '\\';

	while (from != to) {
		node = ll_reg_next(from, &reg);	
		if (node == NULL) {
			break;
		}
		io_write_line(stdout, "%s", ll_s(node));
		read_command_list(gbl_global_cmd_buf, rest);
		execute_command_list(gbl_global_cmd_buf, node);
		from = ll_next(node, 1);
	}
	gbl_saved = 0;
}

void ed_global_invert(node_t *from, node_t *to, char *rest) {
	regex_t reg;
	rest = parse_global_command(&reg, rest);

	if (parse_defaults) {
		from = ll_first_node();
		to = ll_last_node();
	}
	from = (from == global_head() ? ll_first_node() : from);
	to = (to == global_tail() ? to : ll_next(to, 1));

	node_t *node;
	read_command_list(gbl_global_cmd_buf, rest);
	while (from != to) {
		node = ll_reg_next_invert(from, &reg);	
		if (node == NULL) {
			break;
		}
		execute_command_list(gbl_global_cmd_buf, node);
		from = ll_next(node, 1);
	}
	gbl_saved = 0;
}

void ed_global_interact_invert(node_t *from, node_t *to, char *rest) {
	regex_t reg;
	rest = parse_global_command(&reg, rest);

	if (parse_defaults) {
		from = ll_first_node();
		to = ll_last_node();
	}
	from = (from == global_head() ? ll_first_node() : from);
	to = (to == global_tail() ? to : ll_next(to, 1));

	node_t *node;
	rest[strlen(rest) - 2] = '\\';

	while (from != to) {
		node = ll_reg_next_invert(from, &reg);	
		if (node == NULL) {
			break;
		}
		io_write_line(stdout, "%s", ll_s(node));
		read_command_list(gbl_global_cmd_buf, rest);
		execute_command_list(gbl_global_cmd_buf, node);
		from = ll_next(node, 1);
	}
	gbl_saved = 0;
}

void ed_undo(node_t *from, node_t *to, char *rest) {
	undo();
}

void ed_redo(node_t *from, node_t *to, char *rest) {
	redo();
}
