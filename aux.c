#include "aux.h"
#include <ctype.h>

char *skipspaces(char *s) {
	if (! isspace(*s))
		return s;
	while (isspace(*s))
		s++;
	return s;
}
