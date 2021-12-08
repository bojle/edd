#ifndef UNDO_H
#define UNDO_H

#include "ll.h"
#include "parse.h"

void un_fptr_init();
void push_to_append_buf(node_t *node);
void push_to_delete_buf(node_t *node);
void push_to_undo_buf(char c);
void undo();
void redo();

#endif
