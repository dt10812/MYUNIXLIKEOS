/* Subject to change after file streams.
 * Standard implementation below.
 */

#include "stdio.h"

extern void terminal_putc(char);

int putchar(int c){
    terminal_putc((char)c);
    return c;
}

/*
int putchar(int c){
    return putc(c, stdout);
}
*/