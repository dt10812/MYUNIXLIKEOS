#include "string.h"

char *strncpy(char *restrict s1, const char *restrict s2, size_t n) {
    char *s1ptr = (char *)s1;
    const char *s2ptr = s2;
    for (; n && *s2ptr; n--, *s1ptr++ = *s2ptr++);
    for (; n; n--) *s1ptr++ = '\0';
    return s1;
}