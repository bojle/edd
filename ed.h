#ifndef ED_H
#define ED_H

#include "ll.h"

char *get_prompt();
void set_prompt(char *s);

void ed_append(node_t *from, node_t *to, char *rest);
void ed_print(node_t *from, node_t *to, char *rest);
void ed_print_n(node_t *from, node_t *to, char *rest);
void ed_delete(node_t *from, node_t *to, char *rest);
void ed_change(node_t *from, node_t *to, char *rest);
void ed_move(node_t *from, node_t *to, char *rest);
void ed_newline(node_t *from, node_t *to, char *rest);
void ed_prompt(node_t *from, node_t *to, char *rest);
void ed_insert(node_t *from, node_t *to, char *rest);

#endif
