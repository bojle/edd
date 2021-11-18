#ifndef LL_H
#define LL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
 * public:
 * 		node_t *gbl_current_node;
 * 		typedef struct {
 * 				node_t *prev;
 * 				char *s;
 * 				node_t *next;
 * 			} node_t;
 *
 * 		ll_add_next(node, s)
 * 		ll_add_prev(node, s)
 * 		ll_make_node(prev, s, next)
 * 		ll_remove_node(node)
 * 		ll_next(node, num)
 * 		ll_prev(node, num)
 * 		ll_at(num)
 * 		ll_free()
 *
 * private:
 * 		ll_free_node(node)
 * 		ll_alloc_node(size)
 */ 

typedef struct LL{
	struct LL *prev;
	char *s;
	struct LL *next;
} node_t;


static void ll_free_node(node_t* node);
static node_t *ll_alloc_node(size_t size); // `size` -> size of string

node_t *ll_add_next(node_t *node,  char *s);
node_t *ll_add_prev(node_t *node,  char *s);

node_t *ll_make_node(node_t *prev, char *s, node_t *next);
node_t *ll_remove_node(node_t *node);

/* return the node which is 'offset' distance apart from 'node' */
node_t *ll_next(node_t *node, int offset);
node_t *ll_prev(node_t *node, int offset);

/* Linked lists are 0 indexed */
node_t *ll_at(int n);

node_t *ll_attach_nodes(node_t *n1, node_t *n2);

node_t *ll_init();

node_t *global_head();
node_t *global_current();
node_t *global_tail();

#endif
