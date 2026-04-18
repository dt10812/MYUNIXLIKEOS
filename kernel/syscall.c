/* TODO: abstract logic out of syscall.c */

#include <stdint.h>
#include <stdio.h>
#include "sys/syscall.h"
#include "fcntl.h"
#include "vfs.h"
#include "nvram.h"
#include "sys/time.h"
#include "commands.h"

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
            /* Only fd 1 (stdout) and fd 2 (stderr) supported for now. */
            int fd    = (int)regs->ebx;
            const char *buf = (const char *)regs->ecx;
            uint32_t count = regs->edx;

            if ((fd == 1 || fd == 2) && buf && count) {
                /* Write exactly count bytes — buf may not be NUL-terminated */
                for (uint32_t i = 0; i < count; i++) {
                    terminal_write((char[]){buf[i], '\0'});
                    serial_write((char[]){buf[i], '\0'});
                }
                regs->eax = count;
            } else {
                regs->eax = (uint32_t)-1;
            }
            break;
        }

        case SYS_mkdir:
            k_mkdir((const char *)regs->ebx);
            break;

        case SYS_open: /* only creating file for now */
            switch (regs->edx) {
            case O_CREAT:
                regs->eax = (uint32_t)k_touch((const char *)regs->ebx);
                break;
            default:
                regs->eax = (uint32_t)-1;
                break;
            }
            break;

        case SYS_clock_gettime:
            switch (regs->ebx) {
            case CLOCK_REALTIME: {
                char nvbuf[128];
                struct nvram_t *nvbufptr = (struct nvram_t *)nvbuf;
                readNVRAM(nvbuf);

                struct timespec *tp = (struct timespec *)regs->ecx;

                int sec   = bcd(nvbufptr->rtc_sec);
                int min   = bcd(nvbufptr->rtc_min);
                int hour  = bcd(nvbufptr->rtc_hour);
                int day   = bcd(nvbufptr->rtc_day);
                int month = bcd(nvbufptr->rtc_month);
                int year  = bcd(nvbufptr->century_BCD) * 100 + bcd(nvbufptr->rtc_year);

                uint8_t days_per_month[12] = {
                    31, 28, 31, 30, 31, 30,
                    31, 31, 30, 31, 30, 31
                };

                #define IS_LEAP(y) (((y) % 4 == 0 && (y) % 100 != 0) || ((y) % 400 == 0))

                time_t total_days = 0;
                for (int y = 1970; y < year; y++)
                    total_days += IS_LEAP(y) ? 366 : 365;

                for (int m = 0; m < month - 1; m++) {
                    total_days += days_per_month[m];
                    if (m == 1 && IS_LEAP(year))
                        total_days += 1;
                }

                total_days += day - 1;

                tp->tv_sec  = total_days * 86400
                            + hour * 3600
                            + min  * 60
                            + sec;
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
        case SYS_exit:
            /* temp fix till sh is a prog */
            sh();
            break;
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
            /* Heap break - just return 0 for now */
            regs->eax = 0;
            break;
        case SYS_sbrk:
            /* Heap increment - return error for now */
            regs->eax = (uint32_t)-1;
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
        default:
            regs->eax = (uint32_t)-1;
            break;
    }
}