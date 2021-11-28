#ifndef AUX_H
#define AUX_H
/* Auxillary functions */

#include <regex.h>

char *skipspaces(char *s);
char *strrep(char *str, regex_t *rep, char *with, _Bool matchall);
char *regerror_aux(int errcode, regex_t *reg);

#endif
