#ifndef IO_H
#define IO_H

#include "ll.h"
/* 
 */

FILE *fileopen(char *filename, char *mode);
void io_load_file(char *filename);
void io_write_file(char *filename);

ssize_t io_read_line(char **line, size_t *linecap, FILE *fp, char *prompt);
int io_write_line(FILE *fp, char *line);


#endif
