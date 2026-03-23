/* Subject to change after file streams.
 * Standard implementation below.
 */

#include "stdio.h"

extern char keyboard_getchar();

int getchar(){
    return keyboard_getchar();
}

/*
int getchar(){
    return getc(stdin);
}
*/