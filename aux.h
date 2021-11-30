#ifndef AUX_H
#define AUX_H
/* Auxillary functions */

#include <regex.h>

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
void ds_set(ds_t *obj, char *s);
char *ds_get_s(ds_t *obj);
size_t ds_get_sz(ds_t *obj);
void ds_free(ds_t *obj);

#endif
