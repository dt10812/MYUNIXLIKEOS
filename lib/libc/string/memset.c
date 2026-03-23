#include "string.h"

void *memset(void *s, int val, size_t n){
    for(char *sptr = (char *)s; n--; *sptr = (unsigned char)val);
    return s;
}