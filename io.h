#ifndef IO_H
#define IO_H

/* fopen() wrapper */
FILE *fileopen(char *filename, char *mode);
FILE *shopen(char *cmd, char *mode);
char *parse_filename(char *filename);

/* Load 'fp' in the global list */
void io_load_file(FILE *fp);
/* Write the global list to 'filename' */
void io_write_file(char *filename);

/* getline() wrapper */
ssize_t io_read_line(char **line, size_t *linecap, FILE *fp, char *prompt);
/* printf() wrapper */
int io_write_line(FILE *fp, const char *fmt, ...);

#endif
