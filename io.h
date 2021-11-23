#ifndef IO_H
#define IO_H

/* fopen() wrapper */
FILE *fileopen(char *filename, char *mode);

/* Load 'filename' in the global list */
void io_load_file(char *filename);
/* Write the global list to 'filename' */
void io_write_file(char *filename);

/* getline() wrapper */
ssize_t io_read_line(char **line, size_t *linecap, FILE *fp, char *prompt);
/* printf() wrapper */
int io_write_line(FILE *fp, const char *fmt, ...);

#endif
