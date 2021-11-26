#ifndef ED_H
#define ED_H

#include "ll.h"

void ed_append(node_t *from, node_t *to, char *rest);
void ed_print(node_t *from, node_t *to, char *rest);
void ed_delete(node_t *from, node_t *to, char *rest);
void ed_change(node_t *from, node_t *to, char *rest);

#endif
