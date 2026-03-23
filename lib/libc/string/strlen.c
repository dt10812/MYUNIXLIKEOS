#include "stdio.h"
#include <stddef.h>

size_t strlen(const char *s){
    const char *s1 = s;
    for(; *s1 != '\0'; s1++);
    return (s1-s);
}