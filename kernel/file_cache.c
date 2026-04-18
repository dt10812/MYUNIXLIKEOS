#include "file_cache.h"
#include "string.h"
#include "stdio.h"
#include "pmm.h"
#include "sys/time.h"

static file_cache_entry_t cache[FILE_CACHE_MAX_ENTRIES];
static uint32_t cache_used = 0;
static uint32_t cache_hits = 0;
static uint32_t cache_misses = 0;

void file_cache_init(void) {
    for (uint32_t i = 0; i < FILE_CACHE_MAX_ENTRIES; i++) {
        cache[i].filename[0] = '\0';
        cache[i].size = 0;
        cache[i].hits = 0;
        cache[i].timestamp = 0;
    }
    cache_used = 0;
    cache_hits = 0;
    cache_misses = 0;
}

uint8_t* file_cache_get(const char* filename, uint32_t* size) {
    for (uint32_t i = 0; i < cache_used; i++) {
        if (strcmp(cache[i].filename, filename) == 0) {
            cache[i].hits++;
            cache_hits++;
            *size = cache[i].size;
            return cache[i].data;
        }
    }
    cache_misses++;
    return NULL;
}

void file_cache_put(const char* filename, const uint8_t* data, uint32_t size) {
    if (size > FILE_CACHE_ENTRY_SIZE) {
        return; /* File too large for cache */
    }
    
    /* Check if already cached, update instead */
    for (uint32_t i = 0; i < cache_used; i++) {
        if (strcmp(cache[i].filename, filename) == 0) {
            memcpy(cache[i].data, data, size);
            cache[i].size = size;
            cache[i].timestamp = 0; /* Update timestamp */
            return;
        }
    }
    
    /* Add new entry */
    if (cache_used < FILE_CACHE_MAX_ENTRIES) {
        strcpy(cache[cache_used].filename, filename);
        memcpy(cache[cache_used].data, data, size);
        cache[cache_used].size = size;
        cache[cache_used].hits = 0;
        cache[cache_used].timestamp = 0;
        cache_used++;
    } else {
        /* Cache full - find least used entry and replace */
        uint32_t min_hits = 0xFFFFFFFF;
        uint32_t min_idx = 0;
        for (uint32_t i = 0; i < FILE_CACHE_MAX_ENTRIES; i++) {
            if (cache[i].hits < min_hits) {
                min_hits = cache[i].hits;
                min_idx = i;
            }
        }
        strcpy(cache[min_idx].filename, filename);
        memcpy(cache[min_idx].data, data, size);
        cache[min_idx].size = size;
        cache[min_idx].hits = 0;
        cache[min_idx].timestamp = 0;
    }
}

void file_cache_invalidate(const char* filename) {
    for (uint32_t i = 0; i < cache_used; i++) {
        if (strcmp(cache[i].filename, filename) == 0) {
            /* Shift remaining entries */
            for (uint32_t j = i; j < cache_used - 1; j++) {
                cache[j] = cache[j + 1];
            }
            cache_used--;
            return;
        }
    }
}

void file_cache_flush(void) {
    file_cache_init();
}

void file_cache_stats(void) {
    printf("File Cache Statistics:\n");
    printf("  Entries in use: %u/%u\n", cache_used, FILE_CACHE_MAX_ENTRIES);
    printf("  Cache hits: %u\n", cache_hits);
    printf("  Cache misses: %u\n", cache_misses);
    uint32_t total_requests = cache_hits + cache_misses;
    if (total_requests > 0) {
        uint32_t hit_rate = (cache_hits * 100) / total_requests;
        printf("  Hit rate: %u%%\n", hit_rate);
    }
    printf("\nCached files:\n");
    for (uint32_t i = 0; i < cache_used; i++) {
        printf("  [%u] %s (%u bytes, %u hits)\n",
               i, cache[i].filename, cache[i].size, cache[i].hits);
    }
}
