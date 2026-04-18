#ifndef FILE_CACHE_H
#define FILE_CACHE_H

#include <stdint.h>
#include "vfs.h"

#define FILE_CACHE_MAX_ENTRIES 16
#define FILE_CACHE_ENTRY_SIZE  4096

typedef struct {
    char filename[64];
    uint32_t size;
    uint8_t data[FILE_CACHE_ENTRY_SIZE];
    uint32_t hits;
    uint32_t timestamp;
} file_cache_entry_t;

/* Initialize the file cache */
void file_cache_init(void);

/* Get a file from cache, return NULL if not cached */
uint8_t* file_cache_get(const char* filename, uint32_t* size);

/* Add a file to cache */
void file_cache_put(const char* filename, const uint8_t* data, uint32_t size);

/* Clear a specific file from cache */
void file_cache_invalidate(const char* filename);

/* Clear all cache entries */
void file_cache_flush(void);

/* Get cache statistics */
void file_cache_stats(void);

#endif
