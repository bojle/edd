#include "ll.h"
#include <string.h>
#include "err.h"
#include <errno.h>
#include <stdlib.h>
#include <regex.h>

/*
 * The underlying data structure of edd is a doubly linked list with three 
 * extra pointers: head, current and tail. This file contains that data 
 * structure. Head and tail are hollow nodes, * i.e. they do not have any 
 * values. Therefore, the first and last nodes of the list are (head + 1) 
 * and (tail - 1) nodes respectievely. The 'current' node is not a node in 
 * itself, but a pointer to an existing node between (head + 1) and (tail - 1).
 */

/* should address '0' correspond to the first line or address '1' */
#define ED_INDEXING 1

struct node_t{
	struct node_t *prev;
	char *s;
	ssize_t size;
	struct node_t *next;
};

node_t brake;

static node_t gbl_current_node;
static node_t gbl_head_node;
static node_t gbl_tail_node;
static ssize_t gbl_len;

void ll_free_node(node_t* node) {
	free((char *)node->s);
	free((node_t *)node);
}

static node_t *ll_alloc_node(size_t size) {
	node_t *node = calloc(1, sizeof(*node));
	if (!node) {
		err(&to_repl, strerror(errno));
	}

	if (size == 0) {
		/* Do not allocate space for the string */
		goto end; 
	}
	/* +1 for null byte at the end */
	node->s = calloc(size + 1, sizeof(*(node->s))); 
	if (!node->s) {
		err(&to_repl, strerror(errno));
	}
end:
	return node;
}

node_t *ll_add_next(node_t *node,  char *s) {
	node_t *newnode = ll_make_node(node, s, node->next);
	ll_attach_nodes(newnode, node->next);
	ll_attach_nodes(node, newnode);
	ll_set_current_node(newnode);	
	gbl_len++;
	return newnode;
}

node_t *ll_add_prev(node_t *node,  char *s) {
	node_t *newnode = ll_make_node(node->prev, s, node);
	node->prev->next = newnode;
	node->prev = newnode;
	ll_set_current_node(newnode);	
	gbl_len++;
	return newnode;
}

node_t *ll_make_node(node_t *prev, char *s, node_t *next) {
	size_t size = (s == NULL ? 0 : strlen(s));
	node_t *nd = ll_alloc_node(size);
	nd->prev = prev;
	nd->next = next;
	nd->size = size;
	if (size != 0) {
		strncpy(nd->s, s, size + 1);
	}
	ll_set_current_node(nd);	
	return nd;
}

node_t *ll_remove_node(node_t *node) {
	node_t *prev_node = node->prev;
	node_t *next_node = node->next;
	if (prev_node != NULL) {
		prev_node->next = next_node;
	}
	if (next_node != NULL) {
		next_node->prev = prev_node;
	}
	ll_free_node(node);
	ll_set_current_node(next_node);	
	gbl_len--;
	return next_node;
}	

node_t *ll_remove_shallow(node_t *node) {
	ll_detach_node(node);
	ll_set_current_node(node->next);	
	gbl_len--;
	return node->next;
}
	
node_t *ll_next(node_t *node, int offset) {
	if (offset == 1) {
		return node->next;
	}
	while (node != global_tail() && offset > 0) {
		node = node->next;
		offset--;
	}
	ll_set_current_node(node);	
	return node;
}

node_t *ll_prev(node_t *node, int offset) {
	if (offset == 1) {
		return node->prev;
	}
	while (node != global_head() && offset > 0) {
		node = node->prev;
		offset--;
	}
	ll_set_current_node(node);	
	return node;
}

node_t *ll_reg_next(node_t *node, regex_t *reg) {
	for (node_t *nd = node; nd != global_tail(); nd = nd->next) {
		if (regexec(reg, nd->s, 0, NULL, 0) == 0) {
			ll_set_current_node(nd);
			return nd;
		}
	}
	ll_set_current_node(node);	
	return NULL;
}

node_t *ll_reg_prev(node_t *node, regex_t *reg) {
	for (node_t *nd = node; nd != global_head(); nd = nd->prev) {
		if (regexec(reg, nd->s, 0, NULL, 0) == 0) {
			ll_set_current_node(nd);
			return nd;
		}
	}
	ll_set_current_node(node);	
	return NULL;
}

node_t *ll_reg_next_invert(node_t *node, regex_t *reg) {
	for (node_t *nd = node; nd != global_tail(); nd = nd->next) {
		if (regexec(reg, nd->s, 0, NULL, 0) != 0) {
			ll_set_current_node(nd);
			return nd;
		}
	}
	ll_set_current_node(node);	
	return NULL;
}

node_t *ll_reg_prev_invert(node_t *node, regex_t *reg) {
	for (node_t *nd = node; nd != global_head(); nd = nd->prev) {
		if (regexec(reg, nd->s, 0, NULL, 0) != 0) {
			ll_set_current_node(nd);
			return nd;
		}
	}
	ll_set_current_node(node);	
	return NULL;
}


char *ll_s(node_t *node) {
	return node->s;
}

/* This is the only function that is aware of "indexes" */
node_t *ll_at(int n) {
	return ll_next(ll_first_node(), n - ED_INDEXING);
}

node_t *ll_attach_nodes(node_t *n1, node_t *n2) {
	n1->next = n2;
	n2->prev = n1;
	ll_set_current_node(n2);
	return n2;
}

node_t *ll_init() {
	ll_attach_nodes(&gbl_head_node, &gbl_tail_node);
	ll_set_current_node(&gbl_head_node);
	gbl_len = 0;
	return &gbl_head_node;
}

void ll_free() {
	node_t *node = ll_first_node();
	while (node != global_tail()) {
		node = ll_remove_node(node);
	}
}

node_t *global_head() {
	return &gbl_head_node;
}

node_t *global_current() {
	return &gbl_current_node;
}
node_t *global_tail() {
	return &gbl_tail_node;
}

ssize_t ll_node_size(node_t *node) {
	return node->size;
}

node_t *ll_first_node() {
	return gbl_head_node.next;
}

node_t *ll_last_node() {
	return gbl_tail_node.prev;
}

/* Concatenate strings of n1 and n2, delete n2 */
node_t *ll_join_nodes(node_t *n1, node_t *n2) {
	n1->s[n1->size - 1] = '\0';
	n1->size--;

	size_t new_sz = n1->size + n2->size;

	n1->s = realloc(n1->s, (new_sz + 1) * sizeof(*(n1->s)) );
	if (n1->s == NULL) {
		err(&to_repl, strerror(errno));
	}
	n1->size = new_sz;
	strncat(n1->s, n2->s, n2->size);
	ll_remove_shallow(n2);
	ll_set_current_node(n1);
	return n1;
}

node_t *ll_cut_node(node_t *n, int where) {
	n->s[where] = '\n';
	n->s[where + 1] = '\0';
	n->size = where + 1;
}

int ll_node_index(node_t *node) {
	if (node == global_head() || node == global_tail()) {
		return -1;
	}
	node_t *i = ll_first_node();
	int n = ED_INDEXING;
	while (i != node) {
		i = ll_next(i, 1);
		n++;
	}
	return n;
}

void ll_set_current_node(node_t *node) {
	if (node == global_tail()) {
		gbl_current_node = *(ll_last_node());
		return;
	}
	gbl_current_node = *node;
}

void ll_set_s(node_t *n, char *s) {
	if (s == NULL) {
		return;
	}
	n->s = s;
}

void ll_detach_node(node_t *node) {
	ll_attach_nodes(node->prev, node->next);
}

node_t *ll_make_shallow(char *s) {
	node_t *newnode = ll_alloc_node(0);
	newnode->prev = NULL;
	newnode->next = NULL;
	newnode->s = s;
	return newnode;
}
