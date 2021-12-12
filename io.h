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

/* args */

extern _Bool opt_restricted;
extern _Bool opt_silent;
extern _Bool opt_extended;

/* 
 * returns optind i.e. index of the first argument
 * in argv that is not an option
 */
int parse_args(int argc, char **argv);

#endif
