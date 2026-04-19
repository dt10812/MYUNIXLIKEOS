#ifndef _SYSINFO_H_
#define _SYSINFO_H_

#include <stdint.h>

struct sysinfo {
    uint32_t uptime;         /* Seconds since boot */
    uint32_t totalram;       /* Total usable main memory size in bytes */
    uint32_t freeram;        /* Available memory size in bytes */
    uint32_t sharedram;      /* Amount of shared memory in bytes */
    uint32_t bufferram;      /* Memory used by buffers in bytes */
    uint32_t totalswap;      /* Total swap space size in bytes */
    uint32_t freeswap;       /* Available swap space in bytes */
    uint16_t procs;          /* Number of current processes */
    uint32_t totalhigh;      /* Total high memory size in bytes */
    uint32_t freehigh;       /* Available high memory size in bytes */
    uint32_t mem_unit;       /* Memory unit size in bytes */
};

#endif