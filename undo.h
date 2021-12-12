#ifndef UNDO_H
#define UNDO_H

#include "ll.h"
#include "parse.h"

void un_fptr_init();
void push_to_append_buf(node_t *node);
node_t *pop_append_buf();
void push_to_delete_buf(node_t *node);
node_t *pop_delete_buf();
void push_to_undo_buf(char c);
void reset_undo();
void undo_buffers_free();
char undo();
char redo();

#endif
