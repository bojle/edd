#include <stdio.h>
#include <stdlib.h>
#include "undo.h"
#include "ll.h"
#include "parse.h"
#include "aux.h"
#include "err.h"
#include "ed.h"

/*
 *  DATA BUFFERS
 *
 * 		gbl_append_buf, gbl_delete_buf (collectively called append buffers).
 * 		gbl_redo_append,gbl_redo_delete (redo buffers)
 * 		gbl_undo_buf (undo buffer)
 *
 * 	TERMS
 *
 * 	~ Buffers: "Lenient stacks". Lenient because because their stack properties 
 * 	aren't strictly adhered to. Many functions randomly access the "buffer" 
 * 	and the stack itself has helper functions that flagrantly violate the 
 * 	"stack data structure". For example, a 'false push' function that pushes 
 * 	nothing but increments the internal counter.
 *
 *  ~ The global list: or "global active list" or "active list" is all the 
 *  nodes that can be reached by starting from gbl_head_node or gbl_tail_node
 *  (defined in ll.c). In other words, a global list is all the nodes between
 *  gbl_head_node or gbl_tail_node.
 *
 *
 * 	HOW 'UNDO' WORKS
 *
 * 	Undos and Redos are generative, in that, a previous state is generated, 
 * 	not remembered. Undo takes an action, like 'append', inverts it, and 
 * 	carries out the inverted action. Redo, on the other hand is simply the 
 * 	'action' with no modifications.
 *
 * 	HOW 'UNDO' IS IMPLEMENTED
 *
 * 	1. All ed_ functions (for which an 'undo' operation makes sense) register 
 * 	themselves by pushing a character (which is the name of the
 * 	command they correspond to) to gbl_undo_buf (the undo stack).
 *
 * 	2. All ed_ functions push the data that they modified to the append
 * 	buffers. 
 *
 * 	3. All un_ functions pop data from the append buffers, and push them 
 * 	to the redo buffers.
 *
 * 	4. All re_ functions pop data from the redo buffers and push them to 
 *  the append buffers.
 *
 * 	5. undo() pops a character from the undo stack, looks up a table for an
 * 	un_ function corresponding that character and calls it. 
 *
 * 	6. undo() does not deletes a character from memory when it pops it; it
 * 	simply decrements the index, creating an illusion of a pop. This is done
 * 	so redo() can work. 
 *
 * 	7. redo() "looks ahead" in the undo buffer to see the latest action that
 * 	was undone, looks up a table for a re_ function corresponding that 
 * 	action and calls it.
 *
 *  8. An un_ function (nearly every ed_ function eligible for undo has one) 
 *  is an "inverter". An "append" action, for example, can be inverted (or undone)
 *  by a "delete" of whatever was appended.
 *
 *  9. A re_ function, superficially, is equivalent to an ed_ function. The 
 *  difference lies in the buffers that these functions read and update. ed_
 *  functions predominantly read from the user and write to append buffers,
 *  while re_ functions read from append buffers and write
 *  to redo buffers.
 *
 *  10. At any time, these invariants hold true:
 *  	- gbl_append_buf and gbl_redo_append point only to nodes in the active 
 *  	list.
 *  	- gbl_delete_buf and gbl_redo_delete point only to nodes NOT in the
 *  	active list.
 *  	- ed_ functions only push to gbl_append_buf or gbl_delete_buf.
 *  	- un_ functions only push to gbl_redo_append or gbl_redo_delete.
 *  	- re_ functions only push to gbl_append_buf or gbl_delete_buf.
 *
 */


/* Function pointer template */
typedef void (*fptr_t) (void);

/* Used by undo() */
static fptr_t fptr_table_undo[FPTR_ARRAY_SIZE];
/* Used by redo() */
static fptr_t fptr_table_redo[FPTR_ARRAY_SIZE];

static int fp_hash(char c) {
	return c - FIRST_ASCII_CHAR;
}

static void fp_assign(fptr_t *table, char c, fptr_t fn) {
	table[fp_hash(c)] = fn;
}

/* node buffers */

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

#if 0
static node_t *nb_top(nb_t *nb) {
	return nb->nb[nb->nmemb - 1];
}
#endif

static node_t *nb_pop(nb_t *nb) {
	nb->nmemb--;
	return nb->nb[nb->nmemb];
}

#if 0
static node_t *nb_at(nb_t *nb, int i) {
	return nb->nb[i];
}
#endif

static void nb_free(nb_t *nb) {
	if (nb->nb == NULL) {
		return;
	}
	free(nb->nb);
}

/* Undo buffers */

typedef struct undo_t {
	ds_t *buf;
	/* how many undos has there been */
	size_t undo_count;
} undo_t;

static undo_t gbl_undo_buf;

void undo_push(undo_t *u, char c) {
	ds_append(u->buf, c);
}

char undo_pop(undo_t *u) {
	return ds_pop(u->buf);
}

/*******************************************************************
 * 						un_ and re_ functions                      *
 *******************************************************************/

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

	/* cut is now the index at which the newstring should end */
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

static void un_global() {
	char c;
	while ((c = undo_pop(&gbl_undo_buf)) != 'g') {
		fptr_table_undo[fp_hash(c)]();
		gbl_undo_buf.undo_count++;
	}
}

static void re_global() {
	char c;
	while ((c = ds_false_push(gbl_undo_buf.buf)) != 'g') {
		fptr_table_redo[fp_hash(c)]();
		gbl_undo_buf.undo_count--;
	}
}

/***************************************************************/

/* 
 * Called once by main(), initializes fptr_table_undo and
 * fptr_table_redo 
 */
void un_fptr_init() {
	gbl_undo_buf.buf = ds_make();
	atexit(undo_buffers_free);
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
	fp_assign(fptr_table_undo, 'g', un_global);
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
	fp_assign(fptr_table_redo, 'g', re_global);
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

void undo_buffers_free() {
	nb_free(&gbl_append_buf);
	nb_free(&gbl_delete_buf);
	nb_free(&gbl_redo_append);
	nb_free(&gbl_redo_delete);
	ds_free(gbl_undo_buf.buf);
}

char undo() {
	char c = undo_pop(&gbl_undo_buf);
	if (c == '\0') {
		err_normal(&to_repl, "%s\n", "Already at the latest change.");
	}
	fptr_table_undo[fp_hash(c)]();
	gbl_undo_buf.undo_count++;
	return c;
}

/* redo = undo + 1 */
char redo() {
	if (gbl_undo_buf.undo_count == 0) {
		err_normal(&to_repl, "%s\n", "Nothing to redo.");
	}
	char c = ds_false_push(gbl_undo_buf.buf);
	if (c == '\0') {
		err_normal(&to_repl, "%s\n", "Already at the latest change.");
	}
	fptr_table_redo[fp_hash(c)]();
	gbl_undo_buf.undo_count--;
	return c;
}
