#ifndef AUX_H
#define AUX_H
/* Auxillary functions */

#include <regex.h>
#include "ll.h"

char *skipspaces(char *s);
char *strrep(char *str, regex_t *rep, char *with, _Bool matchall);
char *regerror_aux(int errcode, regex_t *reg);

/* Dynamic Strings */

/*
 * A wrapper over the traditional malloc/free handling of 
 * dynamic strings.
 * 
 * Used mainly by ed_ functions that need to remember what the
 * arguments to the last call of that function were.
 * For eg: ed_shell, when called as '!!' will execute the last
 * shell command it executed - this command (string) must be stored
 * somewhere.
 */
typedef struct ds_t ds_t;
ds_t *ds_make();
void ds_append(ds_t *ds, char c);
void ds_set(ds_t *obj, char *s);
char *ds_get_s(ds_t *obj);
size_t ds_get_sz(ds_t *obj);
void ds_free(ds_t *obj);
char ds_pop(ds_t *ds);
char ds_false_push(ds_t *ds);
int ds_nmembs(ds_t *ds);
void ds_clear(ds_t *ds);

typedef struct yb_t yb_t;
void yb_append(yb_t *yb, char *s);
char *yb_at(yb_t *yb, int i);
yb_t *yb_make();
void yb_free(yb_t *yb);
void yb_clear(yb_t *yb);
size_t yb_nmembs(yb_t *yb);
// TODO: remove this
void yb_print(yb_t *yb);


typedef struct re_t re_t;
re_t *re_make();
void re_free(re_t *re);
ds_t *re_get_subst(re_t *re);
int re_has_subst(re_t *re);

char *next_unescaped_delimiter(char *exp, char delimiter);
char *re_replace(re_t *re, char *line, char *subst);
char *next_unescaped_delimiter(char *exp, char delimiter);
void parse_tail(re_t *re, char *tail);
void parse_tail_alt(re_t *re, char *tail);
void parse_subst(re_t *re, char *line, char *exp);
void parse_regex(re_t *re, char *exp);
char *strsubs(re_t *re, char *line, char *exp);

char *parse_global_command(regex_t *reg, char *exp);

void read_command_list(yb_t *yb, char *cmd);
void execute_command_list(yb_t *yb, node_t *from);
#endif
