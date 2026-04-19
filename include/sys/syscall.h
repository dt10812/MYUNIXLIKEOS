/* subject to changes */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "stdarg.h"

/* files and io */
#define SYS_read           0x0000   /* for: read(fd, buf, count) - returns: bytes read */
#define SYS_write          0x0001   /* for: write(fd, buf, count) - returns: bytes written */
#define SYS_open           0x0002   /* for: open(path, flags, mode) - returns: fd */
#define SYS_close          0x0003   /* for: close(fd) - returns: 0 on success */
#define SYS_stat           0x0004   /* for: stat(path, statbuf) - returns: 0 on success */
#define SYS_fstat          0x0005   /* for: fstat(fd, statbuf) - returns: 0 on success */
#define SYS_lstat          0x0006   /* for: lstat(path, statbuf) - returns: 0 on success. */
#define SYS_lseek          0x0007   /* for: lseek(fd, offset, whence) - returns: new offset */
#define SYS_dup            0x0008   /* for: dup(fd) - returns: new fd pointing to same file */
#define SYS_dup2           0x0009   /* for: dup2(oldfd, newfd) - returns: newfd. */
#define SYS_pipe           0x000A   /* for: pipe(fds[2]) - returns: 0 on success. */
#define SYS_mkdir          0x000B   /* for: mkdir(path, mode) - returns: 0 on success */
#define SYS_rmdir          0x000C   /* for: rmdir(path) - returns: 0 on success. */
#define SYS_rename         0x000D   /* for: rename(oldpath, newpath) - returns: 0 on success */
#define SYS_truncate       0x000E   /* for: truncate(path, length) - returns: 0 on success. */
#define SYS_ftruncate      0x000F   /* for: ftruncate(fd, length) - returns: 0 on success. */
#define SYS_chmod          0x0010   /* for: chmod(path, mode) - returns: 0 on success. */
#define SYS_fchmod         0x0011   /* for: fchmod(fd, mode) - returns: 0 on success. */
#define SYS_chown          0x0012   /* for: chown(path, uid, gid) - returns: 0 on success. */
#define SYS_fchown         0x0013   /* for: fchown(fd, uid, gid) - returns: 0 on success. */
#define SYS_unlink         0x0014   /* for: unlink(path) - returns: 0 on success. */
#define SYS_mknod          0x0015   /* for: mknod(path, mode, dev) - returns: 0 on success. */
#define SYS_umask          0x0016   /* for: umask(mask) - returns: old mask. */
#define SYS_access         0x0017   /* for: access(path, mode) - returns: 0 on success. */
#define SYS_utime          0x0018   /* for: utime(path, times) - returns: 0 on success. */
#define SYS_utimes         0x0019   /* for: utimes(path, times[2]) - returns: 0 on success. */
#define SYS_link           0x001A   /* for: link(oldpath, newpath) - returns: 0 on success. */
#define SYS_symlink        0x001B   /* for: symlink(target, linkpath) - returns: 0 on success. */
#define SYS_readlink       0x001C   /* for: readlink(path, buf, bufsz) - returns: bytes read. */
#define SYS_sync           0x001D   /* for: sync() - No return value. */
#define SYS_fsync          0x001E   /* for: fsync(fd) - returns: 0 on success. */
/* directory traversal */
#define SYS_chdir          0x001F   /* for: chdir(path) - returns: 0 on success. */
#define SYS_getcwd         0x0020   /* for: getcwd(buf, size) - returns: 0 on success. */
#define SYS_getdents       0x0021   /* for: getdents(fd, buf, count) - returns: bytes read. */
/* time */
#define SYS_clock_gettime  0x0022   /* for: clock_gettime(clockid, timespec) - returns: 0 on success */
/* unistd */
#define SYS_execl          0x0023   /* for: int execl(const char *path, const char *arg0, ...) */
#define SYS_exit           0x0024   /* for: exit(int ret) */
#define SYS_fork           0x0025   /* for: pid_t fork(void) - returns: child PID in parent, 0 in child */
#define SYS_getpid         0x0026   /* for: pid_t getpid(void) - returns: process ID */
#define SYS_getppid        0x0027   /* for: pid_t getppid(void) - returns: parent process ID */
#define SYS_getuid         0x0028   /* for: uid_t getuid(void) - returns: user ID */
#define SYS_geteuid        0x0029   /* for: uid_t geteuid(void) - returns: effective user ID */
#define SYS_setuid         0x002A   /* for: int setuid(uid_t uid) - returns: 0 on success */
#define SYS_getgid         0x002B   /* for: gid_t getgid(void) - returns: group ID */
#define SYS_getegid        0x002C   /* for: gid_t getegid(void) - returns: effective group ID */
#define SYS_setgid         0x002D   /* for: int setgid(gid_t gid) - returns: 0 on success */
#define SYS_wait           0x002E   /* for: pid_t wait(int *status) - returns: child PID */
#define SYS_waitpid        0x002F   /* for: pid_t waitpid(pid_t pid, int *status, int flags) */
#define SYS_kill           0x0030   /* for: int kill(pid_t pid, int sig) - returns: 0 on success */
#define SYS_pause          0x0031   /* for: int pause(void) - returns: -1 on signal */
#define SYS_sleep          0x0032   /* for: unsigned int sleep(unsigned int seconds) - returns: seconds remaining */
#define SYS_usleep         0x0033   /* for: int usleep(useconds_t usec) - returns: 0 on success */
#define SYS_alarm          0x0034   /* for: unsigned int alarm(unsigned int seconds) - returns: previous alarm */
#define SYS_brk            0x0035   /* for: int brk(void *addr) - returns: 0 on success */
#define SYS_sbrk           0x0036   /* for: void *sbrk(intptr_t increment) - returns: previous break */
#define SYS_getenv         0x0037   /* for: char *getenv(const char *name) - returns: env value or NULL */
#define SYS_setenv         0x0038   /* for: int setenv(const char *name, const char *value, int overwrite) */
#define SYS_unsetenv       0x0039   /* for: int unsetenv(const char *name) */
#define SYS_time           0x003A   /* for: time_t time(time_t *t) - returns: current time */
#define SYS_gettimeofday   0x003B   /* for: int gettimeofday(struct timeval *tv, struct timezone *tz) */
#define SYS_mmap           0x003C   /* for: void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) */
#define SYS_munmap         0x003D   /* for: int munmap(void *addr, size_t length) */
#define SYS_mprotect       0x003E   /* for: int mprotect(void *addr, size_t len, int prot) */
/* system info */
#define SYS_sysinfo        0x003F   /* for: sysinfo(struct sysinfo *info) - returns: 0 on success */

/*
 * Usage is sort of unintuitive:
 * systemcall with one value pass: syscall(SYS_xyz, arg, 0, 0, 0, 0);
 * systemcall with two value pass: syscall(SYS_xyz, arg1, arg2, 0, 0, 0);
 * ...
 * systemcall with five value pass: syscall(SYS_xyz, arg1, arg2, arg3, arg4, arg5);
 */
static inline long syscall(int id, long arg1, long arg2, long arg3, long arg4, long arg5) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(id), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5)
        : "memory"
    );
    return ret;
}

#endif   /* syscall.h */