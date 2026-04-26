#include "string.h"

char *strncat(char *restrict s1, const char *restrict s2, size_t n) {
    char *s1ptr = (char *)s1;
    const char *s2ptr = s2;
    
    // Find end of s1
    for (; *s1ptr; s1ptr++);
    
    // Append up to n characters from s2
    for (; n && *s2ptr; n--) {
        *s1ptr++ = *s2ptr++;
    }
    
    // Null terminate
    *s1ptr = '\0';
    return s1;
}
