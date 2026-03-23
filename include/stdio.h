/* Base standard, XSI extensions excluded */

#ifndef _STDIO_H_
#define _STDIO_H_

typedef struct file_t{
    int flags; /* -r, -r, -rw... */
    int fileno;
    unsigned char *buf;
    unsigned char *bptr; /* file cursor */
    int len; /* buffer length in bytes */
    char free; /* flag for fclose */
    char iobf; /* io buffer flush: _IOFBF, _IOLBF, or _IONBF */
    char eof; /* flag if EOF is present */
} FILE;

#define _IOFBF 0x1 /* Input/output fully buffered. */
#define _IOLBF 0x2 /* Input/output line buffered. */
#define _IONBF 0x3 /* Input/output unbuffered. */

#define SEEK_CUR 0x1
#define SEEK_END 0x2
#define SEEK_SET 0x3

#define EOF -1

#define FILENAME_MAX 256 /* arbatrally set to _POSIX_PATH_MAX */ 
#define FOPEN_MAX 20
#define TMP_MAX 25

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int putchar(int c);
int getchar();

#endif /* _STDIO_H_ */