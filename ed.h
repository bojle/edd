#ifndef ED_H
#define ED_H

#include "ll.h"

char *get_prompt();
void set_prompt(char *s);

char *get_default_filename();
void set_default_filename(char *s);

void set_command_buf(char *cmd);
char *get_command_buf();

void ed_append(node_t *from, node_t *to, char *rest);
void ed_print(node_t *from, node_t *to, char *rest);
void ed_print_n(node_t *from, node_t *to, char *rest);
void ed_delete(node_t *from, node_t *to, char *rest);
void ed_change(node_t *from, node_t *to, char *rest);
void ed_move(node_t *from, node_t *to, char *rest);
void ed_newline(node_t *from, node_t *to, char *rest);
void ed_prompt(node_t *from, node_t *to, char *rest);
void ed_insert(node_t *from, node_t *to, char *rest);
void ed_shell(node_t *from, node_t *to, char *rest);
void ed_edit(node_t *from, node_t *to, char *rest);
void ed_edit_force(node_t *from, node_t *to, char *rest);
void ed_file(node_t *from, node_t *to, char *rest);
void ed_join(node_t *from, node_t *to, char *rest);
void ed_quit(node_t *from, node_t *to, char *rest);
void ed_quit_force(node_t *from, node_t *to, char *rest);

#endif
