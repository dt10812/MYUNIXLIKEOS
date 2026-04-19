/* stdio.h - User-friendly C standard library header for MYUNIXLIKEOS programs */

#ifndef _STDIO_USER_H_
#define _STDIO_USER_H_

#include <stddef.h>

/* Standard I/O streams - for user programs */
#define NULL 0
#define EOF -1

/* Basic character I/O */
extern int putchar(int c);
extern int getchar(void);

/* Formatted output */
extern int printf(const char* format, ...);

/* String functions */
extern size_t strlen(const char* s);
extern int strcmp(const char* s1, const char* s2);
extern char* strcpy(char* dest, const char* src);
extern void* memset(void* s, int c, size_t n);
extern void* memcpy(void* dest, const void* src, size_t n);

/* System calls */
extern int write(int fd, const void* buf, size_t count);
extern void _exit(int code);

/* Math operations */
static inline int add(int a, int b) { return a + b; }
static inline int sub(int a, int b) { return a - b; }
static inline int mul(int a, int b) { return a * b; }
static inline int div(int a, int b) { return b ? a / b : 0; }

#endif
