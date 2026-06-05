#ifndef _DIRENT_H_
#define _DIRENT_H_

#include "sys/types.h"

#define DT_UNKNOWN 0
#define DT_REG     1
#define DT_DIR     2

struct dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
};

#endif /* dirent.h */
