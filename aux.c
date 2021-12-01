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
	ds_t *ds = calloc(1, sizeof(ds_t));
	ds->s = NULL;
	ds->sz = 0;
	return ds;
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
