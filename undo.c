#include "undo.h"
#include <stdio.h>
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
static nb_t gbl_redo_append;
static nb_t gbl_redo_delete;

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
	nb_push(&gbl_redo_delete, &brake);
	while ((current = nb_pop(&gbl_append_buf)) != &brake) {
		ll_detach_node(current);
		nb_push(&gbl_redo_delete, current);
	}
}

static void re_append() {
	node_t *current;
	push_to_append_buf(&brake);
	while ((current = nb_pop(&gbl_redo_delete)) != &brake) {
		ll_attach_nodes(current, ll_next(current, 1));
		ll_attach_nodes(ll_prev(current, 1), current);
		push_to_append_buf(current);
	}
}

static void un_delete() {
	node_t *current;
	nb_push(&gbl_redo_append, &brake);
	while ((current = nb_pop(&gbl_delete_buf)) != &brake) {
		ll_attach_nodes(current, ll_next(current, 1));
		ll_attach_nodes(ll_prev(current, 1), current);
		nb_push(&gbl_redo_append, current);
	}
}

/* redo_append to delete */
static void re_delete() {
	node_t *current;
	push_to_delete_buf(&brake);
	while ((current = nb_pop(&gbl_redo_append)) != &brake) {
		ll_detach_node(current);
		push_to_delete_buf(current);
	}
}

/*
 * change is a delete followed by an insert.
 * un_change is a delete of what was inserted and
 * insert of what was deleted
 */
static void un_change() {
#if 0
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
	for (int i = 0; i < (int)tmp_buf.nmemb; ++i) {
		push_to_delete_buf(nb_at(&tmp_buf, i));
	}
#endif
	un_append();
	un_delete();
}

static void re_change() {
	re_delete();
	re_append();
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

	nb_push(&gbl_redo_append, &brake);
	nb_push(&gbl_redo_append, from_prev);
	nb_push(&gbl_redo_append, from);
	nb_push(&gbl_redo_append, to);
	nb_push(&gbl_redo_append, to_next);
	nb_push(&gbl_redo_append, move_to);
	nb_push(&gbl_redo_append, move_to_subsequent);
}

static void re_move() {
	node_t *move_to_subsequent = nb_pop(&gbl_redo_append);
	node_t *move_to = nb_pop(&gbl_redo_append);
	node_t *to_next = nb_pop(&gbl_redo_append);
	node_t *to = nb_pop(&gbl_redo_append);
	node_t *from = nb_pop(&gbl_redo_append);
	node_t *from_prev = nb_pop(&gbl_redo_append);
	/* Pop the brakes off */
	nb_pop(&gbl_redo_append);

	ll_attach_nodes(from_prev, to_next);
	ll_attach_nodes(move_to, from);
	ll_attach_nodes(to, move_to_subsequent);	

	push_to_append_buf(&brake);
	push_to_append_buf(from_prev);
	push_to_append_buf(from);
	push_to_append_buf(to);
	push_to_append_buf(to_next);
	push_to_append_buf(move_to);
	push_to_append_buf(move_to_subsequent);
}



#if 0
static void un_edit() {
	node_t *current = nb_pop(&gbl_delete_buf);
	nb_pop(&gbl_delete_buf);
	char *filename = ll_s(current);
	ed_edit(NULL, NULL, filename);
	undo_pop(&gbl_undo_buf);
	ll_free_node(current);
}
#endif

static void un_join() {
	node_t *c1, *c2;
	/* cut is the size that needs to be cut from the 
	 * rear end of the string.
	 */
	size_t cut = 0;
	nb_push(&gbl_redo_append, &brake);
	while ((c1 = nb_pop(&gbl_delete_buf)) != &brake) {
		ll_attach_nodes(c1, ll_next(c1, 1));
		ll_attach_nodes(ll_prev(c1, 1), c1);
		nb_push(&gbl_redo_append, c1);
		cut += ll_node_size(c1) - 1;
		c2 = c1;
	}
	cut++;
	c2 = ll_prev(c2, 1);
	size_t sz = ll_node_size(c2);

	/* cut is now the index at which a the newstring should end */
	cut = sz - cut;
	ll_cut_node(c2, cut); 
}

static void re_join() {
	node_t *from, *to, *tmp;
	from = nb_pop(&gbl_redo_append);
	from = ll_prev(from, 1);
	while ((tmp = nb_pop(&gbl_redo_append)) != &brake) {
		to = tmp;
	}
	ed_join(from, to, NULL);
	undo_pop(&gbl_undo_buf);
}

#if 0
static void nb_print(nb_t *nb) {
	for (int i = 0; i < nb->nmemb; ++i) {
		if (nb->nb[i] == &brake) {
			printf("[brake]\n");
		}
		else {
			printf("%s", ll_s(nb->nb[i]));
		}
	}
}
#endif

static void un_subs() {
	node_t *old, *new;
	nb_push(&gbl_redo_append, &brake);
	nb_push(&gbl_redo_delete, &brake);
	while (((old = nb_pop(&gbl_delete_buf)) != &brake) &&
			((new = nb_pop(&gbl_append_buf)) != &brake)) {
		ll_attach_nodes(ll_prev(new, 1), old);
		ll_attach_nodes(old, ll_next(new, 1));
		nb_push(&gbl_redo_append, old);
		nb_push(&gbl_redo_delete, new);
	}
	nb_pop(&gbl_append_buf);
}

static void re_subs() {
	node_t *old, *new;
	push_to_append_buf(&brake);
	push_to_delete_buf(&brake);
	while (((old = nb_pop(&gbl_redo_delete)) != &brake) &&
			((new = nb_pop(&gbl_redo_append)) != &brake)) {
		ll_attach_nodes(ll_prev(new, 1), old);
		ll_attach_nodes(old, ll_next(new, 1));
		push_to_append_buf(old);
		push_to_delete_buf(new);
	}
	nb_pop(&gbl_redo_append);
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
	fp_assign(fptr_table_undo, 'd', un_delete);
	fp_assign(fptr_table_undo, 'c', un_change);
	fp_assign(fptr_table_undo, 'm', un_move);
	fp_assign(fptr_table_undo, 'j', un_join);
	fp_assign(fptr_table_undo, 'r', un_append);
	fp_assign(fptr_table_undo, 't', un_append);
	fp_assign(fptr_table_undo, 's', un_subs);
	/* Redo */
	fp_assign(fptr_table_redo, 'a', re_append);
	fp_assign(fptr_table_redo, 'i', re_append);
	fp_assign(fptr_table_redo, 'd', re_delete);
	fp_assign(fptr_table_redo, 'c', re_change);
	fp_assign(fptr_table_redo, 'm', re_move);
	fp_assign(fptr_table_redo, 'j', re_join);
	fp_assign(fptr_table_redo, 'r', re_append);
	fp_assign(fptr_table_redo, 't', re_append);
	fp_assign(fptr_table_redo, 's', re_subs);
}

void push_to_append_buf(node_t *node) {
	nb_push(&gbl_append_buf, node);
}

node_t *pop_delete_buf() {
	return nb_pop(&gbl_delete_buf);
}

node_t *pop_append_buf() {
	return nb_pop(&gbl_append_buf);
}

void push_to_delete_buf(node_t *node) {
	nb_push(&gbl_delete_buf, node);
}

void push_to_undo_buf(char c) {
	undo_push(&gbl_undo_buf, c);
}

void reset_undo() {
	gbl_undo_buf.undo_count = 0;
	ds_clear(gbl_undo_buf.buf);
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

