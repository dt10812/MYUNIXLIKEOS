#ifndef _UNISTD_H_
#define _UNISTD_H_

#include "sys/types.h"
#include "sys/sysinfo.h"

ssize_t write(int fildes, const void *buf, size_t nbyte);
int execl(const char *path, ...);
int sysinfo(struct sysinfo *info);

#endif /* unistd.h */