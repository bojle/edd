#include "undo.h"
#include "ll.h"
#include "parse.h"
#include "aux.h"
#include "err.h"
#include <stdlib.h>

/* UNDO */

/* Buffers */


typedef struct node_buf {
	node_t **nb;
	size_t sz;
	size_t nmemb;
	size_t initialized;
} nb_t;

static nb_t gbl_append_buf;
static nb_t gbl_delete_buf;

static void nb_push(nb_t *nb, node_t *node) {
	if (node == NULL) {
		return;
	}
	size_t sz = (nb->sz == 0 ? 1 : nb->sz);
	if (nb->sz == nb->nmemb) {
		nb->nb = realloc(nb->nb, (sz * 2) * sizeof(*(nb->nb)));
		nb->sz = sz * 2;
	}
	if (nb->nmemb >= nb->initialized) {
		nb->initialized++;
	}
	nb->nb[nb->nmemb] = node;
	nb->nmemb++;
}

static node_t *nb_top(nb_t *nb) {
	return nb->nb[nb->nmemb - 1];
}

static node_t *nb_pop(nb_t *nb) {
	nb->nmemb--;
	return nb->nb[nb->nmemb];
}

static void nb_free(nb_t *nb) {
	free(nb->nb);
}

/* Undo buffers */

typedef struct undo_t {
	ds_t *buf;
	size_t undo_count;
} undo_t;

static undo_t gbl_undo_buf;

void undo_push(undo_t *u, char c) {
	ds_append(u->buf, c);
}

char undo_pop(undo_t *u) {
	return ds_pop(u->buf);
}


/* un_ functions */

static void un_append() {
	node_t *current;
	push_to_delete_buf(&brake);
	while ((current = nb_pop(&gbl_append_buf)) != &brake) {
		ll_detach_node(current);
		push_to_delete_buf(current);
	}
}

static void re_append() {
	node_t *current;
	push_to_append_buf(&brake);
	while ((current = nb_pop(&gbl_delete_buf)) != &brake) {
		ll_attach_nodes(current, ll_next(current, 1));
		ll_attach_nodes(ll_prev(current, 1), current);
		push_to_append_buf(current);
	}
}


/* Function pointer type for functions of this type: void foo(void) */
typedef void (*fptr_t) (void);

static fptr_t fptr_table_undo[FPTR_ARRAY_SIZE];
static fptr_t fptr_table_redo[FPTR_ARRAY_SIZE];

static int fp_hash(char c) {
	return c - FIRST_ASCII_CHAR;
}

static void fp_assign(fptr_t *table, char c, fptr_t fn) {
	table[fp_hash(c)] = fn;
}

void un_fptr_init() {
	gbl_undo_buf.buf = ds_make();
	/* Undo */
	fp_assign(fptr_table_undo, 'a', un_append);
	fp_assign(fptr_table_undo, 'i', un_append);
	fp_assign(fptr_table_undo, 'd', re_append);
	/* Redo */
	fp_assign(fptr_table_redo, 'a', re_append);
	fp_assign(fptr_table_redo, 'i', re_append);
	fp_assign(fptr_table_redo, 'd', un_append);
}

void push_to_append_buf(node_t *node) {
	nb_push(&gbl_append_buf, node);
}

void push_to_delete_buf(node_t *node) {
	nb_push(&gbl_delete_buf, node);
}

void push_to_undo_buf(char c) {
	undo_push(&gbl_undo_buf, c);
}

void undo() {
	char c = undo_pop(&gbl_undo_buf);
	if (c == '\0') {
		err_normal(&to_repl, "%s\n", "Already at the latest change.");
	}
	fptr_table_undo[fp_hash(c)]();
	gbl_undo_buf.undo_count++;
}

/* redo = undo + 1 */
void redo() {
	if (gbl_undo_buf.undo_count == 0) {
		err_normal(&to_repl, "%s\n", "Nothing to redo.");
	}
	char c = ds_false_push(gbl_undo_buf.buf);
	if (c == '\0') {
		err_normal(&to_repl, "%s\n", "Already at the latest change.");
	}
	fptr_table_redo[fp_hash(c)]();
	gbl_undo_buf.undo_count--;
}
