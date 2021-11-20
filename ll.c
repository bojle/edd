#include "ll.h"
#include <string.h>

struct node_t{
	struct node_t *prev;
	char *s;
	struct node_t *next;
};

static node_t *gbl_current_node;
static node_t *gbl_head_node;
static node_t *gbl_tail_node;
static size_t gbl_len;

static void ll_free_node(node_t* node) {
	free((char *)node->s);
	free((node_t *)node);
}

static node_t *ll_alloc_node(size_t size) {
	// TODO: better error checking with suspension upon failure

	node_t *node = calloc(1, sizeof(*node));
	if (!node) {
		return NULL;
	}

	if (size == 0) {
		goto end; // do not allocate space for the string
	}
	node->s = calloc(size, sizeof(*(node->s)));
	if (!node->s) {
		return NULL;
	}

end:
	return node;
}

node_t *ll_add_next(node_t *node,  char *s) {
	node_t *newnode = ll_make_node(node, s, node->next);
	node_t *next_node = node->next;
	node->next = newnode;
	next_node->prev = newnode;
	gbl_current_node = newnode;
	gbl_len++;
	return newnode;
}

node_t *ll_add_prev(node_t *node,  char *s) {
	node_t *newnode = ll_make_node(node->prev, s, node);
	node->prev->next = newnode;
	node->prev = newnode;
	gbl_current_node = newnode;
	gbl_len++;
	return newnode;
}

node_t *ll_make_node(node_t *prev, char *s, node_t *next) {
	size_t size = (s == NULL ? 0 : strlen(s));
	node_t *nd = ll_alloc_node(size);
	nd->prev = prev;
	nd->next = next;
	if (size != 0) {
		strcpy(nd->s, s);
	}
	gbl_current_node = nd;
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
	gbl_current_node = next_node;
	gbl_len--;
	return next_node;
}	
	
node_t *ll_next(node_t *node, int offset) {
	while (/*node->next != gbl_tail_node &&*/ node->next != NULL && offset > 0) {
		node = node->next;
		offset--;
	}
	gbl_current_node = node;
	return node;
}

node_t *ll_prev(node_t *node, int offset) {
	while (/*node->prev != gbl_head_node &&*/ node->prev != NULL && offset > 0) {
		node = node->prev;
		offset--;
	}
	gbl_current_node = node;
	return node;
}

char *ll_s(node_t *node) {
	return node->s;
}

node_t *ll_at(int n) {
	return ll_next(gbl_head_node->next, n);
}

node_t *ll_attach_nodes(node_t *n1, node_t *n2) {
	n1->next = n2;
	n2->prev = n1;
	return n2;
}

node_t *ll_init() {
	gbl_head_node = ll_make_node(NULL, NULL, NULL);
	gbl_tail_node = ll_make_node(gbl_head_node, NULL, NULL);
	gbl_head_node->next = gbl_tail_node;
	gbl_current_node = gbl_head_node;
	gbl_len = 0;
	return gbl_head_node;
}

void ll_free() {
	if (gbl_len <= 0) {
		goto end;
	}
	node_t *node = gbl_head_node;
	while ((node = ll_next(node, 1)) != gbl_tail_node) {
		node = ll_remove_node(node);
	}
end:
	ll_free_node(gbl_head_node);
	ll_free_node(gbl_tail_node);
	/* gbl_current_node is not freed because it does not exist.
	 * It is but a pointer to some other existing nodes
	 */
}

node_t *global_head() {
	return gbl_head_node;
}

node_t *global_current() {
	return gbl_current_node;
}
node_t *global_tail() {
	return gbl_tail_node;
}

