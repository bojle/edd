#ifndef LL_H
#define LL_H

#include <regex.h>
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
 */ 

/*
 * Declarations
 */


/* The pointer type for a node in the global linked list */
typedef struct node_t node_t;

/* Used by undo functions */
extern node_t brake;


/*
 * Constructive Functions 
 */

/* Add a node with 's' as its value next to 'node' */
node_t *ll_add_next(node_t *node,  char *s);
/* Add a node with 's' as its value before 'node' */
node_t *ll_add_prev(node_t *node,  char *s);
/* Return a node with 's' as its values, 'prev' and 'next' as its pointers */
node_t *ll_make_node(node_t *prev, char *s, node_t *next);
node_t *ll_attach_nodes(node_t *n1, node_t *n2);
/* Initialise the global list */
node_t *ll_init();
/* Join (Concatenate) the strings of n1 and n2 */
node_t *ll_join_nodes(node_t *n1, node_t *n2);
void ll_set_current_node(node_t *node);
void ll_set_s(node_t *n, char *s);
node_t *ll_make_shallow(char *s);

/*
 * Destructive Functions
 */
node_t *ll_remove_node(node_t *node);
/* Detach but don't free 'node' */
node_t *ll_remove_shallow(node_t *node);
void ll_detach_node(node_t *node);
/* Free the global list */
void ll_free();
void ll_free_node(node_t* node);
void ll_cut_node(node_t *n, int where);

/*
 * Lookup Functions 
 */
/* return the node which is 'offset' distance apart from 'node' */
node_t *ll_next(node_t *node, int offset);
node_t *ll_prev(node_t *node, int offset);

/* return the next node that matches 'reg' after 'node' */
node_t *ll_reg_next(node_t *node, regex_t *reg);
node_t *ll_reg_prev(node_t *node, regex_t *reg);

/* return the next node that DOES NOT match 'reg' after 'node' */
node_t *ll_reg_next_invert(node_t *node, regex_t *reg);
node_t *ll_reg_prev_invert(node_t *node, regex_t *reg);

/* Linked lists are 0 indexed */
node_t *ll_at(int n);

node_t *global_head();
node_t *global_current();
node_t *global_tail();

/* Access the value of a node */
char *ll_s(node_t *node);
/* return the length of the string at 'node', i.e. node->size */
ssize_t ll_node_size(node_t *node);

/* First node of the global list, according to the current convention */
node_t *ll_first_node();
node_t *ll_last_node();

/* return the line number in the global list corresponding 'node' */
int ll_node_index(node_t *node);

#endif
