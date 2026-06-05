#ifndef _STAT_H_
#define _STAT_H_

#include "sys/types.h"

#define S_IFMT   0170000
#define S_IFDIR  0040000
#define S_IFREG  0100000

struct stat {
    dev_t    st_dev;
    ino_t    st_ino;
    mode_t   st_mode;
    nlink_t  st_nlink;
    uid_t    st_uid;
    gid_t    st_gid;
    off_t    st_size;
    blksize_t st_blksize;
    fsblkcnt_t st_blocks;
    time_t   st_atime;
    time_t   st_mtime;
    time_t   st_ctime;
};

int mkdir(const char *path, mode_t mode);
int stat(const char *path, struct stat *buf);
int fstat(int fildes, struct stat *buf);

#endif /* stat.h */