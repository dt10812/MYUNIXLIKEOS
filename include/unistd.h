#ifndef _UNISTD_H_
#define _UNISTD_H_

#include "sys/types.h"
#include "sys/stat.h"
#include "sys/syscall.h"
#include "fcntl.h"
#include "dirent.h"
#include "sys/sysinfo.h"

ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);
int close(int fildes);
int open(const char *path, int oflag, ...);
int stat(const char *path, struct stat *buf);
int fstat(int fildes, struct stat *buf);
int chdir(const char *path);
int getdents(int fd, struct dirent *dirp, size_t count);
int unlink(const char *path);
pid_t fork(void);
pid_t wait(int *status);
int brk(void *addr);
void *sbrk(intptr_t increment);
void _exit(int status);
int execl(const char *path, ...);
int sysinfo(struct sysinfo *info);

#endif /* unistd.h */