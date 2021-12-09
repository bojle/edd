#include "aux.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "err.h"
#include "io.h"
#include "ll.h"
#include "parse.h"

#define REPLIM 200

char *skipspaces(char *s) {
	if (!isspace(*s) || *s == '\n')
		return s;
	while (isspace(*s))
		s++;
	return s;
}

/* Dynamic strings */

typedef struct ds_t{
	char *s;
	size_t sz;
	int nmemb;
} ds_t;

ds_t *ds_make() {
	ds_t *ds = calloc(1, sizeof(*ds));
	ds->s = NULL;
	ds->sz = 0;
	ds->nmemb = 0;
	return ds;
}

void ds_set(ds_t *obj, char *s) {
	if (*s == '\0') {
		return;
	}
	size_t sz = strlen(s);
	if (sz > obj->sz) {
		obj->s = realloc(obj->s, (sz+1) * sizeof(*(obj->s)));
	}
	strncpy(obj->s, s, sz);
	obj->s[sz] = '\0';
	obj->sz = sz;
#if 0
	if (obj->s[sz - 1] == '\n') {
		obj->s[sz - 1] = '\0';
		obj->sz--;
	}
#endif
	obj->nmemb = obj->sz;
}

char *ds_get_s(ds_t *obj) {
	return obj->s;
}

size_t ds_get_sz(ds_t *obj) {
	return obj->sz;
}

char ds_at(ds_t *obj, int n) {
	return obj->s[n];
}


void ds_free(ds_t *obj) {
	free(obj->s);
}

void ds_append(ds_t *ds, char c) {
	size_t sz = (ds->sz == 0 ? 1 : ds->sz);
	if (ds->nmemb >= (int)ds->sz) {
		ds->s = realloc(ds->s, (sz * 2) * sizeof(*(ds->s)));
		ds->sz = sz * 2;
	}
	ds->s[ds->nmemb] = c;
	ds->nmemb++;
	ds->s[ds->nmemb] = '\0';
}

void ds_clear(ds_t *ds) {
	if (ds->nmemb == 0) {
		return;
	}
	ds->s[0] = '\0';
	ds->nmemb = 0;
}

void ds_cat_e(ds_t *ds, char *sp, char *ep) {
	while (sp <= ep) {
		ds_append(ds, *sp);
		sp++;
	}
}

char ds_pop(ds_t *ds) {
	if (ds->nmemb == 0) {
		return '\0';
	}
	ds->nmemb--;
	return ds->s[ds->nmemb];
}

char ds_false_push(ds_t *ds) {
	ds->nmemb++;
	return ds->s[ds->nmemb - 1];
}

int ds_nmembs(ds_t *ds) {
	return ds->nmemb;
}


/* Yank buf */

typedef struct yb_t {
	ds_t **yb;
	size_t sz;
	size_t nmemb;
	size_t initialized;
} yb_t;

/*
 * yb_t *obj;
 * obj->yb[n] points to the nth dynamic string
 */

void yb_append(yb_t *yb, char *s) {
	if (s == NULL || *s == '\0') {
		return;
	}
	size_t sz = (yb->sz == 0 ? 1 : yb->sz);
	if (yb->sz == yb->nmemb) {
		yb->yb = realloc(yb->yb, (sz * 2) * sizeof(*(yb->yb)));
		yb->sz = sz * 2;
	}
	if (yb->nmemb >= yb->initialized) {
		yb->yb[yb->nmemb] = ds_make();
		yb->initialized++;
	}
	ds_set(yb->yb[yb->nmemb], s);
	yb->nmemb++;
}

char *yb_at(yb_t *yb, int i) {
	return ds_get_s(yb->yb[i]);
}

yb_t *yb_make() {
	yb_t *yb = calloc(1, sizeof(*yb));
	yb->yb = NULL;
	yb->sz = 0;
	yb->nmemb = 0;
	yb->initialized = 0;
	return yb;
}

void yb_free(yb_t *yb) {
	if (yb->yb == NULL) {
		return;
	}
	for (size_t i = 0; i < yb->nmemb; --i)	{
		ds_free(yb->yb[i]);
	}
	free(yb->yb);
}

void yb_clear(yb_t *yb) {
	yb->nmemb = 0;
}

size_t yb_nmembs(yb_t *yb) {
	return yb->nmemb;
}


/* Regex Functions */

typedef struct re_t{
	regex_t re;
	ds_t *subst;
	_Bool global;
	_Bool print;
	_Bool number;
	int N;
} re_t;

re_t *re_make() {
	re_t *re = calloc(1, sizeof(*re));
	re->global = 0;
	re->print = 0;
	re->number = 0;
	re->N = 0;
	return re;
}

void re_free(re_t *re) {
	if (re->subst != NULL) {
		ds_free(re->subst);
	}
}


ds_t *re_get_subst(re_t *re) {
	return re->subst;
}

int re_has_subst(re_t *re) {
	return !(re->subst == NULL);
}

#define toggle(a) (a = (a == 1 ? 0 : 1))
#define toggle_on(a) (a = 1)


#define NMATCH 200
regmatch_t pmatch[NMATCH];

void parse_regex(re_t *re, char *exp) {
	if (exp == NULL) {
		return;
	}
	int err;
	if ((err = regcomp(&re->re, exp, REG_EXTENDED)) != 0) {
		err_normal(&to_repl, "%s\n", regerror_aux(err, &re->re));
	}
}

void parse_subst(re_t *re, char *line, char *exp) {
	if (exp == NULL) {
		exp = ds_get_s(re->subst);
	}
	int err;
	if ((err = regexec(&re->re, line, NMATCH, pmatch, 0)) != 0) { 
		pmatch[0].rm_so = -1;
		pmatch[0].rm_eo = -1;
	}

	if (re->subst == NULL) {
		re->subst = ds_make();
	}

	ds_clear(re->subst);
	while (*exp) {
		if (*exp == '&' && *(exp-1) != '\\') {
			ds_cat_e(re->subst, line + pmatch[0].rm_so, line + pmatch[0].rm_eo - 1);
		}
		else if (isdigit(*exp) && *(exp-1) == '\\') {
			ds_cat_e(re->subst, line + pmatch[*exp - '0'].rm_so, line + pmatch[*exp - '0'].rm_eo -1);
		}
		else if (*exp == '\\') {
		}
		else {
			ds_append(re->subst, *exp);
		}
		exp++;
	}
}

void parse_tail(re_t *re, char *tail) {
	/* N, r, p, g */
	if (tail == NULL || *tail == '\0') {
		return;
	}
	if (*tail == '\n') {
		re->number = 1;
		re->N = 1;
	}

	for (; *tail; tail++) {
		if (isdigit(*tail)) {
			re->N = strtol(tail, &tail, 10);
			tail--;
			re->number = 1;
		}
		else {
			switch (*tail) {
				case 'p':
					toggle_on(re->print);
					break;
				case 'g':
					toggle_on(re->global);
					break;
			}
		}
	}
}

void parse_tail_alt(re_t *re, char *tail) {
	if (tail == NULL || *tail == '\0') {
		return;
	}
	if (*tail == '\n') {
		re->number = 1;
		re->N = 1;
	}
	for (; *tail; tail++) {
		if (isdigit(*tail)) {
			re->N = strtol(tail, &tail, 10);
			tail--;
			re->number = 1;
		}
		else {
			switch (*tail) {
				case 'r':
					// TODO
					break;
				case 'p':
					toggle(re->print);
					break;
				case 'g':
					toggle(re->global);
					break;
			}
		}
	}
}

char *next_unescaped_delimiter(char *exp, char delimiter) {
	++exp;
	if (exp == NULL || *exp == '\0') {
		return NULL;
	}
	for (; ; ++exp) {
		if (*exp == '\0' || (*exp == delimiter && *(exp -1) != '\\')) {
			break;
		}
	}
	*exp++ = '\0';
	return exp;	
}

char *re_replace(re_t *re, char *line, char *subst) {
	ds_t ds;
	ds.s = NULL;
	ds.sz = 0;
	ds.nmemb = 0;

	_Bool number = re->number;
	int num = re->N;
	// line is a line is a line
	
	parse_subst(re, line, subst);
	if (number) {
		num--;
	}
	char *i = line;
	while (*i) {
		if (i == line + pmatch[0].rm_so) {
			if (number && num > 0) {
				ds_cat_e(&ds, line + pmatch[0].rm_so, line + pmatch[0].rm_eo - 1);
				i = line + pmatch[0].rm_eo;
				line = i;
				parse_subst(re, i, subst);
				num--;
			}
			else {
				char *s = ds_get_s(re->subst);
				int sz = re->subst->nmemb;
				ds_cat_e(&ds, s, s + sz -1);
				i = line + pmatch[0].rm_eo;
				line = i;
				if (re->global) {
					parse_subst(re, i, subst);
				}
				if (number && num <= 0) {
					line += strlen(i);
				}
			}
		}
		else if (*i == '\\') {
			++i;
		}
		else {
			ds_append(&ds, *i);
			i++;
		}
	}
	if (re->print) {
		io_write_line(stdout, "%s", ds.s);
	}
	return ds.s;
}

/* Command list:
 * EG:
 * 		a\
 * 		append line 1\
 * 		append line 2\
 * 		append line 3\
 * 		p
 *
 * 		m $\
 * 		d
 */

void yb_print(yb_t *yb) {
	for (int i = 0; i < (int)yb->nmemb; ++i) {
		printf("%s\n", yb_at(yb, i));
	}
	printf("\n");
}

/* Load the regex in 'exp' in 'reg'; return the start of command-list */
char *parse_global_command(regex_t *reg, char *exp) {
	char delimiter = *exp++;
	char *regex = exp;
	exp = next_unescaped_delimiter(regex, delimiter);
	exp = skipspaces(exp);

	int err;
	if ((err = regcomp(reg, regex, REG_EXTENDED)) != 0) {
		err(&to_repl, regerror_aux(err, reg));
	}
	return exp;
}

void read_command_list(yb_t *yb, char *cmd) {
	size_t len = 0;
	if (*cmd == '&') {
		return;
	}
	yb_clear(yb);
	len = strlen(cmd);
	if (cmd[len - 2] == '\\') {
		cmd[len - 2] = '\n';
		cmd[len - 1] = '\0';
	}
	yb_append(yb, cmd);
	if (cmd[len-1] == '\n' && cmd[len - 2] != '\\') {
		return;
	}

	char *line = NULL;
	size_t linecap;

	while (1) {
		if ((len = io_read_line(&line, &linecap, stdin, NULL)) <= 0) {
			err_normal(&to_repl, "%s\n", "No characters in the input stream");
		}
		if (line[len - 2] == '\\') {
			line[len - 2] = '\n';
			line[len - 1] = '\0';
		}
		yb_append(yb, line);
		if (line[len - 1] == '\n' && line[len - 2] != '\\') {
			break;
		}
	}
	free(line);
}

void execute_command_list(yb_t *yb, node_t *from) {
	char current_cmd;
	char *cmd;

	for (int i = 0; i < (int)yb->nmemb; ++i) {
		cmd = yb_at(yb, i);
		current_cmd = *cmd;
		if (*cmd == 'a' || *cmd == 'i' || *cmd == 'c') {
			cmd = skipspaces(++cmd);
			if (isalnum(*cmd)) {
				err_normal(&to_repl, "%s\n", "Invalid command suffix");
			}
			else {
				if (current_cmd == 'i') {
					from = ll_prev(from, 1);
				}
				else if (current_cmd == 'c') {
					from = ll_remove_node(from);
					from = ll_prev(from, 1);
				}

				for (i += 1; yb_at(yb, i)[0] != '.' && i < (int)yb->nmemb; ++i) {
					ll_add_next(from, yb_at(yb, i));
					from = ll_next(from, 1);
				}
			}
		}
		else if (strchr(gbl_commands, *cmd) != NULL) {
			parse_t *pt = pt_make();
			cmd++;
			pt_set(pt, from, from, current_cmd, skipspaces(cmd));
			parse_defaults = 0;
			eval(pt);
			free(pt);
		}
		else {
			err_normal(&to_repl, "%s\n", "Invalid Command");
		}
	}
}
