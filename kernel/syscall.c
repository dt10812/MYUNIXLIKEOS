/* TODO: abstract logic out of syscall.c */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sys/syscall.h"
#include "sys/sysinfo.h"
#include "sys/stat.h"
#include "dirent.h"
#include "fcntl.h"
#include "vfs.h"
#include "nvram.h"
#include "sys/time.h"
#include "commands.h"
#include "pmm.h"
#include "rtc.h"

extern void *user_sbrk(intptr_t increment);
extern int user_brk(void *addr);

#define MAX_FDS 32

typedef struct {
    int used;
    vnode_t *node;
    uint32_t offset;
    int flags;
} fd_entry_t;

static fd_entry_t fd_table[MAX_FDS];

static int allocate_fd(void) {
    for (int i = 3; i < MAX_FDS; i++) {
        if (!fd_table[i].used) return i;
    }
    return -1;
}

static fd_entry_t *get_fd(int fd) {
    if (fd < 0 || fd >= MAX_FDS) return NULL;
    return fd_table[fd].used ? &fd_table[fd] : NULL;
}

static void release_fd(int fd) {
    if (fd >= 3 && fd < MAX_FDS) {
        fd_table[fd].used = 0;
        fd_table[fd].node = NULL;
        fd_table[fd].offset = 0;
        fd_table[fd].flags = 0;
    }
}

static void ensure_writable_content(vnode_t *node) {
    if (!node || !(node->flags & VFS_FILE)) return;
    if (!node->content) {
        node->content = (char *)pmm_alloc_z(64);
        if (node->content) node->content[0] = '\0';
        node->size = 0;
        return;
    }

    /* Copy static or read-only file content into a writable buffer. */
    char *new_content = (char *)pmm_alloc_z(64);
    if (!new_content) return;
    uint32_t copy_size = node->size < 63 ? node->size : 63;
    memcpy(new_content, node->content, copy_size);
    new_content[copy_size] = '\0';
    node->content = new_content;
}

static int fill_stat(vnode_t *node, struct stat *st) {
    if (!node || !st) return -1;

    st->st_dev = 0;
    st->st_ino = (ino_t)(uintptr_t)node;
    st->st_mode = (node->flags & VFS_DIRECTORY) ? (S_IFDIR | 0755) : (S_IFREG | 0644);
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_size = (off_t)node->size;
    st->st_blksize = 512;
    st->st_blocks = (st->st_size + 511) / 512;
    st->st_atime = st->st_mtime = st->st_ctime = 0;
    return 0;
}

struct linux_dirent {
    uint32_t d_ino;
    uint32_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[256];
};

/* syscall number: regs->eax
 * arg1: regs->ebx
 * arg2: regs->ecx
 * arg3: regs->edx
 * arg4: regs->esi
 * arg5: regs->edi
 */
struct trapframe {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; /* pusha */
    uint32_t trapno, err_code;
    /* auto-pushed by the cpu */
    uint32_t eip, cs, eflags;
} __attribute__((packed));

extern void terminal_write(const char*);

extern void serial_write(const char*);

/* Note: regs->eax is the return value on exit */
void syscall_handler(struct trapframe *regs) {
    switch (regs->eax) {
        case SYS_write: {
            int fd    = (int)regs->ebx;
            const char *buf = (const char *)regs->ecx;
            uint32_t count = regs->edx;

            if (!buf || !count) {
                regs->eax = (uint32_t)-1;
                break;
            }

            if (fd == 1 || fd == 2) {
                for (uint32_t i = 0; i < count; i++) {
                    terminal_write((char[]){buf[i], '\0'});
                    serial_write((char[]){buf[i], '\0'});
                }
                regs->eax = count;
                break;
            }

            fd_entry_t *entry = get_fd(fd);
            if (!entry || !entry->node || !(entry->node->flags & VFS_FILE)) {
                regs->eax = (uint32_t)-1;
                break;
            }

            ensure_writable_content(entry->node);
            if (!entry->node->content) {
                regs->eax = (uint32_t)-1;
                break;
            }

            uint32_t write_offset = entry->offset;
            uint32_t available = 63 - write_offset;
            uint32_t write_size = count < available ? count : available;
            memcpy(entry->node->content + write_offset, buf, write_size);
            entry->node->content[write_offset + write_size] = '\0';
            entry->node->size = write_offset + write_size;
            entry->offset += write_size;
            regs->eax = write_size;
            break;
        }

        case SYS_read: {
            int fd = (int)regs->ebx;
            char *buf = (char *)regs->ecx;
            uint32_t count = regs->edx;

            if (!buf || !count) {
                regs->eax = (uint32_t)-1;
                break;
            }

            fd_entry_t *entry = get_fd(fd);
            if (!entry || !entry->node || !(entry->node->flags & VFS_FILE)) {
                regs->eax = (uint32_t)-1;
                break;
            }

            uint32_t file_size = entry->node->size;
            if (entry->offset >= file_size) {
                regs->eax = 0;
                break;
            }

            uint32_t read_size = count;
            if (entry->offset + read_size > file_size) {
                read_size = file_size - entry->offset;
            }

            memcpy(buf, entry->node->content + entry->offset, read_size);
            entry->offset += read_size;
            regs->eax = read_size;
            break;
        }

        case SYS_close: {
            int fd = (int)regs->ebx;
            if (fd <= 2) {
                regs->eax = (uint32_t)-1;
                break;
            }
            fd_entry_t *entry = get_fd(fd);
            if (!entry) {
                regs->eax = (uint32_t)-1;
                break;
            }
            release_fd(fd);
            regs->eax = 0;
            break;
        }

        case SYS_open: {
            const char *path = (const char *)regs->ebx;
            int flags = (int)regs->ecx;
            int mode = (int)regs->edx;

            if (!path || !*path) {
                regs->eax = (uint32_t)-1;
                break;
            }

            vnode_t *node = vfs_lookup(path);
            if (!node && (flags & O_CREAT)) {
                if (k_touch(path) != 0) {
                    regs->eax = (uint32_t)-1;
                    break;
                }
                node = vfs_lookup(path);
            }

            if (!node) {
                regs->eax = (uint32_t)-1;
                break;
            }

            if ((flags & O_TRUNC) && (node->flags & VFS_FILE)) {
                vfs_write_file(path, "", 0);
            }

            int fd = allocate_fd();
            if (fd < 0) {
                regs->eax = (uint32_t)-1;
                break;
            }

            fd_table[fd].used = 1;
            fd_table[fd].node = node;
            fd_table[fd].offset = 0;
            fd_table[fd].flags = flags;
            (void)mode;
            regs->eax = fd;
            break;
        }

        case SYS_mkdir:
            regs->eax = (uint32_t)k_mkdir((const char *)regs->ebx);
            break;

        case SYS_unlink: {
            regs->eax = (uint32_t)k_unlink((const char *)regs->ebx);
            break;
        }

        case SYS_chdir: {
            const char *path = (const char *)regs->ebx;
            vnode_t *node = vfs_lookup(path);
            if (!node || !(node->flags & VFS_DIRECTORY)) {
                regs->eax = (uint32_t)-1;
            } else {
                current_dir = node;
                regs->eax = 0;
            }
            break;
        }

        case SYS_stat: {
            const char *path = (const char *)regs->ebx;
            struct stat *st = (struct stat *)regs->ecx;
            vnode_t *node = vfs_lookup(path);
            regs->eax = (fill_stat(node, st) == 0) ? 0 : (uint32_t)-1;
            break;
        }

        case SYS_fstat: {
            int fd = (int)regs->ebx;
            struct stat *st = (struct stat *)regs->ecx;
            fd_entry_t *entry = get_fd(fd);
            if (!entry || !entry->node) {
                regs->eax = (uint32_t)-1;
            } else {
                regs->eax = (fill_stat(entry->node, st) == 0) ? 0 : (uint32_t)-1;
            }
            break;
        }

        case SYS_getdents: {
            int fd = (int)regs->ebx;
            void *buf = (void *)regs->ecx;
            uint32_t count = regs->edx;
            fd_entry_t *entry = get_fd(fd);

            if (!buf || !count || !entry || !entry->node || !(entry->node->flags & VFS_DIRECTORY)) {
                regs->eax = (uint32_t)-1;
                break;
            }

            uint32_t filled = 0;
            uint32_t index = entry->offset;
            const uint16_t header_len = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t);

            while (index < entry->node->child_count) {
                vnode_t *child = entry->node->children[index];
                size_t name_len = strlen(child->name) + 1;
                uint16_t reclen = header_len + (uint16_t)name_len;
                if (filled + reclen > count) break;

                struct linux_dirent d;
                d.d_ino = (uint32_t)(uintptr_t)child;
                d.d_off = index + 1;
                d.d_reclen = reclen;
                d.d_type = (child->flags & VFS_DIRECTORY) ? DT_DIR : DT_REG;
                memcpy(d.d_name, child->name, name_len);

                memcpy((uint8_t *)buf + filled, &d, header_len + name_len);
                filled += reclen;
                index++;
            }

            entry->offset = index;
            regs->eax = filled;
            break;
        }

        case SYS_clock_gettime:
            switch (regs->ebx) {
            case CLOCK_REALTIME: {
                uint8_t sec, min, hour, day, month, century;
                uint8_t rtc_year;
                rtc_get_time(&sec, &min, &hour, &day, &month, &rtc_year, &century);

                struct timespec *tp = (struct timespec *)regs->ecx;
                int year = century * 100 + rtc_year;

                /* Validate RTC data - if invalid, use default */
                if (year < 2000 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31 ||
                    hour > 23 || min > 59 || sec > 59) {
                    /* Use default date: 2024-04-18 12:00:00 */
                    year = 2024;
                    month = 4;
                    day = 18;
                    hour = 12;
                    min = 0;
                    sec = 0;
                }

                time_t total_days = 0;
                int start_year = 1970;
                #define IS_LEAP(y) (((y) % 4 == 0 && (y) % 100 != 0) || ((y) % 400 == 0))
                if (year < start_year) {
                    for (int y = year; y < start_year; y++) {
                        total_days -= IS_LEAP(y) ? 366 : 365;
                    }
                } else {
                    for (int y = start_year; y < year; y++) {
                        total_days += IS_LEAP(y) ? 366 : 365;
                    }
                }

                uint8_t days_per_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

                for (int m = 0; m < month - 1; m++) {
                    total_days += days_per_month[m];
                    if (m == 1 && IS_LEAP(year))
                        total_days++;
                }

                total_days += day - 1;
                tp->tv_sec = total_days * 86400 + hour * 3600 + min * 60 + sec;
                tp->tv_nsec = 0;
                regs->eax = 0;
                break;
            }
            default:
                regs->eax = (uint32_t)-1;
                break;
            }
            break;
        case SYS_execl:
            regs->eax = (uint32_t)k_exec((const char*)regs->ebx, (const char**)regs->ecx);
            break;
        case SYS_exit: {
            int status = (int)regs->ebx;
            (void)status;
            regs->cs = 0x08;
            regs->eip = (uint32_t)(uintptr_t)sh;
            regs->eax = 0;
            break;
        }
        case SYS_getpid:
            /* Return fixed PID for now (single process system) */
            regs->eax = 1;
            break;
        case SYS_getppid:
            /* Return parent PID (always init = 1 for now) */
            regs->eax = 1;
            break;
        case SYS_getuid:
            /* Return root user ID */
            regs->eax = 0;
            break;
        case SYS_geteuid:
            /* Return effective UID (same as UID for now) */
            regs->eax = 0;
            break;
        case SYS_setuid:
            /* Allow setuid (always succeed for now) */
            regs->eax = 0;
            break;
        case SYS_getgid:
            /* Return root group ID */
            regs->eax = 0;
            break;
        case SYS_getegid:
            /* Return effective GID (same as GID for now) */
            regs->eax = 0;
            break;
        case SYS_setgid:
            /* Allow setgid (always succeed for now) */
            regs->eax = 0;
            break;
        case SYS_fork:
            /* Fork not yet implemented */
            regs->eax = (uint32_t)-1;
            break;
        case SYS_wait:
            /* Wait not yet implemented */
            regs->eax = (uint32_t)-1;
            break;
        case SYS_waitpid:
            /* Waitpid not yet implemented */
            regs->eax = (uint32_t)-1;
            break;
        case SYS_kill:
            /* Kill not yet implemented */
            regs->eax = (uint32_t)-1;
            break;
        case SYS_pause:
            /* Pause - sleep until signal (not implemented, just return) */
            regs->eax = 0;
            break;
        case SYS_sleep:
            /* Sleep for N seconds - just return 0 for now */
            regs->eax = 0;
            break;
        case SYS_usleep:
            /* Sleep for microseconds - just return 0 for now */
            regs->eax = 0;
            break;
        case SYS_alarm:
            /* Alarm - not implemented */
            regs->eax = 0;
            break;
        case SYS_brk:
            regs->eax = (uint32_t)user_brk((void *)regs->ebx);
            break;
        case SYS_sbrk:
            regs->eax = (uint32_t)(uintptr_t)user_sbrk((intptr_t)regs->ebx);
            break;
        case SYS_getenv:
            /* getenv - not implemented, return NULL */
            regs->eax = 0;
            break;
        case SYS_setenv:
            /* setenv - not implemented, return error */
            regs->eax = (uint32_t)-1;
            break;
        case SYS_unsetenv:
            /* unsetenv - not implemented, return error */
            regs->eax = (uint32_t)-1;
            break;
        case SYS_gettimeofday:
            /* gettimeofday - similar to clock_gettime */
            regs->eax = 0;
            break;
        case SYS_mmap:
            /* mmap - not implemented */
            regs->eax = (uint32_t)-1;
            break;
        case SYS_munmap:
            /* munmap - not implemented */
            regs->eax = 0;
            break;
        case SYS_mprotect:
            /* mprotect - not implemented */
            regs->eax = 0;
            break;
        case SYS_sysinfo: {
            struct sysinfo *info = (struct sysinfo *)regs->ebx;
            if (info) {
                info->uptime = 0; /* TODO: implement uptime */
                info->totalram = pmm_total_pages() * PAGE_SIZE;
                info->freeram = pmm_free_pages() * PAGE_SIZE;
                info->sharedram = 0; /* Not implemented */
                info->bufferram = 0; /* Not implemented */
                info->totalswap = 0; /* No swap */
                info->freeswap = 0; /* No swap */
                info->procs = 1; /* Single process system */
                info->totalhigh = 0; /* No high memory distinction */
                info->freehigh = 0; /* No high memory distinction */
                info->mem_unit = 1; /* Memory unit is bytes */
                regs->eax = 0;
            } else {
                regs->eax = (uint32_t)-1;
            }
            break;
        }
        default:
            regs->eax = (uint32_t)-1;
            break;
    }
}