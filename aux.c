#include "aux.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define REPLIM 200

char *skipspaces(char *s) {
	if (!isspace(*s) || *s == '\n')
		return s;
	while (isspace(*s))
		s++;
	return s;
}


/* 
 * Matches `reg` in `haystack`. Returns pointer to the match
 * and updates `matchsz` to be equal to the size of the 
 * matched substring
 * returns NULL if no match
 */
char *strreg(char *haystack, regex_t *reg, int *matchsz) {
	regmatch_t matcharr[1];	
	if (regexec(reg, haystack, 1, matcharr, 0)) {
		*matchsz = 0;
		return NULL;
	}
	*matchsz = matcharr[0].rm_eo - matcharr[0].rm_so;
	return haystack + matcharr[0].rm_so;
}


/* strncat(3) but returns pointer to the end */
char *strncata(char *dest, char *src, int n) {
	while (*dest) dest++;
	while (n > 0) {
		*dest++ = *src++;
		--n;
	}
	return dest;
}

/*
 * `with` contains strings that may contain '&'s, which are placeholders
 * for mathced strings (see manual).
 * regcat() cats `dest` and `with` with '&'s in `with` replaced by `repstring`
 */
char * regcat(char *dest, char *with, char *repstring, int *substrsizes) {
	//while (*dest) dest++;

	for (int i = 0; *with; ) {
		if (*with == '&') {
			if (*(with-1) != '\\') {
				dest = strncata(dest, repstring, substrsizes[i]);
				with++;
				i++;
				continue;
			}
			else {
				dest--;
				*dest = '&';
			}
		}
		else {
			*dest = *with;
		}

		dest++;
		with++;
	}
	return dest;
}


/* 
 * Count the number of unescaped chars in a string 
 * also store the size of the string without unescaped
 * chars in sz
 */
int countchars(char *s, char n, int *sz) {
	int count = 0;
	*sz = 0;
	while (*s) {
		if (*s == n) { 
			if (*(s-1) != '\\') 
				count++;
		}
		else {
			*sz += 1;
		}
		s++;
	}
	return count;
}


/* Return the total sum of sizes of each substring replaced */
int rep_substr_sz(char *substr, int *substrsizes, int totalreps) {
	int size = 0;
	
	int remsize = 0;
	int namps = countchars(substr, '&', &remsize);
	for (int i = 0; i < totalreps; ++i) {
		size += (namps * substrsizes[i]) + remsize;
	}
	return size;
}



/*
 * Replace `rep` with `with` in `str`. 
 * `matchall`, if true will replace all matches in str.
 * Returns an allocated string, must be freed by the user.
 */
char *strrep(char *str, regex_t *rep, char *with, _Bool matchall) {
	/* replaces in two passes over `str`
	 * first pass: mark what has to be replaced
	 * second pass: replace
	 */

	int strsz = strlen(str);
	char *strstart = str;
	char *strend = str + strsz;

	/* Store pointers to substrings that will be replaced */
	char *reparr[REPLIM];
	/* Number of replacements */
	int totalreps;

	/* Store the size of each substring that will be replaced */
	int substrsizes[REPLIM];
	/* Sum of the above array */
	int repsum = 0;

	
	/* Pass 1 */
	for (int i = 0,repsz = 0; str < strend; ++i) {
		if ((str = reparr[i] = strreg(str, rep, &repsz)) == NULL) {
			totalreps = i;
			break;
		}
		str += repsz;
		totalreps = i;
		substrsizes[i] = repsz;
		repsum += substrsizes[i];

		if (!matchall)
			break;
	}

	if (totalreps == 0)
		return strstart;

	int repsubstrsz = rep_substr_sz(with, substrsizes, totalreps);

	/* retn will be the replaced string and retnsz its size */
	int retnsz = strsz + (repsubstrsz - repsum);
	char *retn; 
	if (!(retn = calloc(retnsz, sizeof(*retn)))) {
		
	}

	char *sretn = retn;

	/* pass 2 */
	str = strstart;
	for (int i = 0; str < strend; ) {
		if (str == reparr[i]) {
			retn = regcat(retn, with, reparr[i], substrsizes);
			str += substrsizes[i];
			++i;
			continue;
		}
		*retn= *str;
		retn++;
		str++;
	}
	free(strstart);
	return sretn;
}

/* Dynamic strings */

typedef struct ds_t{
	char *s;
	size_t sz;
} ds_t;

ds_t *ds_make() {
	return calloc(1, sizeof(ds_t));
}

void ds_set(ds_t *obj, char *s) {
	size_t sz = strlen(s);
	/* If theres a need for reallocation */
	if (sz > obj->sz) {
		obj->s = realloc(obj->s, (sz+1) * sizeof(*(obj->s)));
	}
	strncpy(obj->s, s, sz);
	obj->s[sz] = '\0';
	obj->sz = sz;
	if (obj->s[sz - 1] == '\n') {
		obj->s[sz - 1] = '\0';
		obj->sz--;
	}
}

char *ds_get_s(ds_t *obj) {
	return obj->s;
}

size_t ds_get_sz(ds_t *obj) {
	return obj->sz;
}

void ds_free(ds_t *obj) {
	free(obj->s);
}
