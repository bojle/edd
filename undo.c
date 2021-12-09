#include "undo.h"
#include "ll.h"
#include "parse.h"
#include "aux.h"
#include "err.h"
#include "ed.h"
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

static node_t *nb_at(nb_t *nb, int i) {
	return nb->nb[i];
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

/*
 * change is a delete followed by an insert.
 * un_change is a delete of what was inserted and
 * insert of what was deleted
 */
static void un_change() {
	nb_t tmp_buf = { .sz = 0, .nmemb = 0, .initialized = 0 };
	node_t *current;

	/* Delete what was inserted */
	nb_push(&tmp_buf, &brake);
	while ((current = nb_pop(&gbl_append_buf)) != &brake) {
		ll_detach_node(current);
		nb_push(&tmp_buf, current);
	}

	/* Insert what was deleted */
	re_append();

	/* Push contents of tmp_buf to delete_buf */
	for (int i = 0; i < tmp_buf.nmemb; ++i) {
		push_to_delete_buf(nb_at(&tmp_buf, i));
	}
}

static void un_move() {
	node_t *move_to_subsequent = nb_pop(&gbl_append_buf);
	node_t *move_to = nb_pop(&gbl_append_buf);
	node_t *to_next = nb_pop(&gbl_append_buf);
	node_t *to = nb_pop(&gbl_append_buf);
	node_t *from = nb_pop(&gbl_append_buf);
	node_t *from_prev = nb_pop(&gbl_append_buf);

	/* Pop the brakes off */
	nb_pop(&gbl_append_buf);

	ll_attach_nodes(from_prev, from);
	ll_attach_nodes(to, to_next);
	ll_attach_nodes(move_to, move_to_subsequent);
}

static void re_move() {
	// TODO
}


static void un_edit() {
	node_t *current = nb_pop(&gbl_delete_buf);
	nb_pop(&gbl_delete_buf);
	char *filename = ll_s(current);
	ed_edit(NULL, NULL, filename);
	undo_pop(&gbl_undo_buf);
	ll_free_node(current);
}


/********************************************************************\
|* Function pointer type for functions of this type: void foo(void) *|
\********************************************************************/

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
	fp_assign(fptr_table_undo, 'c', un_change);
	fp_assign(fptr_table_undo, 'm', un_move);
	fp_assign(fptr_table_undo, 'e', un_edit);
	/* Redo */
	fp_assign(fptr_table_redo, 'a', re_append);
	fp_assign(fptr_table_redo, 'i', re_append);
	fp_assign(fptr_table_redo, 'd', un_append);
	fp_assign(fptr_table_redo, 'c', un_change);
	fp_assign(fptr_table_redo, 'e', un_edit);
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

