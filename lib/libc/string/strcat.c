/* Implementation of strcat */

#include "string.h"

char *strcat(char *restrict dest, const char *restrict src) {
    char *ptr = dest;

    /* Find the end of dest */
    while (*ptr != '\0') {
        ptr++;
    }

    /* Copy src to the end of dest */
    while (*src != '\0') {
        *ptr++ = *src++;
    }

    /* Null terminate */
    *ptr = '\0';

    return dest;
}