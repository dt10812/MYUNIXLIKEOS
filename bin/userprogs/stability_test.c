#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <stdint.h>

#ifndef SYS_clock_gettime
#define SYS_clock_gettime 0x0022
#endif

#define HEAP_STEPS 96
#define FILE_STEPS 16
#define MAX_PATH 64
#define TRACKED_ALLOCS 24
#define FD_STRESS 16
#define PAGE_SIZE 4096

static void *allocate_region(size_t size) {
    void *ptr = sbrk((intptr_t)size);
    return ptr == (void *)-1 ? NULL : ptr;
}

static unsigned char pattern_for_round(int round, size_t size) {
    return (unsigned char)((round * 131u + size) & 0xFFu);
}

static void fill_pattern(unsigned char *buf, size_t len, unsigned char value) {
    for (size_t i = 0; i < len; i++) {
        buf[i] = value ^ (unsigned char)i;
    }
}

static int check_pattern(const unsigned char *buf, size_t len, unsigned char value) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != (unsigned char)(value ^ (unsigned char)i)) {
            return 0;
        }
    }
    return 1;
}

static int run_heap_test(void) {
    unsigned char *allocs[TRACKED_ALLOCS] = {0};
    size_t sizes[TRACKED_ALLOCS] = {0};
    unsigned char seeds[TRACKED_ALLOCS] = {0};
    void *base_break = sbrk(0);

    if (base_break == (void *)-1) {
        printf("[stability-test] sbrk(0) failed\n");
        return 1;
    }

    for (int round = 0; round < HEAP_STEPS; round++) {
        size_t size = 64u + ((round * 97u) & 0x7FFu);
        unsigned char seed = pattern_for_round(round, size);
        unsigned char *mem = allocate_region(size);

        if (!mem) {
            printf("[stability-test] sbrk failed at round %d (size=%zu)\n", round, size);
            return 2;
        }

        fill_pattern(mem, size, seed);
        if (!check_pattern(mem, size, seed)) {
            printf("[stability-test] memory corruption detected at round %d\n", round);
            return 3;
        }

        int slot = round % TRACKED_ALLOCS;
        allocs[slot] = mem;
        sizes[slot] = size;
        seeds[slot] = seed;

        for (int i = 0; i < TRACKED_ALLOCS; i++) {
            if (!allocs[i]) continue;
            if (!check_pattern(allocs[i], sizes[i], seeds[i])) {
                printf("[stability-test] stale heap block corrupted at slot %d after round %d\n", i, round);
                return 4;
            }
        }

        if ((round & 0x0F) == 0 && round > 0) {
            void *current_break = sbrk(0);
            void *shrink_target = (void *)((uintptr_t)current_break - PAGE_SIZE);
            if (brk(shrink_target) != 0) {
                printf("[stability-test] brk shrink failed at round %d\n", round);
                return 5;
            }
            if (brk(current_break) != 0) {
                printf("[stability-test] brk restore failed at round %d\n", round);
                return 6;
            }
        }

        if ((round & 0x07) == 0) {
            printf("[stability-test] heap round %d ok (%zu bytes, top=%p)\n", round, size, (void *)sbrk(0));
        }
    }

    if (sbrk(0) <= base_break) {
        printf("[stability-test] heap did not grow as expected\n");
        return 7;
    }

    return 0;
}

static int run_filesystem_test(void) {
    char path[MAX_PATH];
    const char *payload = "stability-test file content\n";
    char buffer[256];

    for (int idx = 0; idx < FILE_STEPS; idx++) {
        sprintf(path, "/stability_test_%02d.log", idx);

        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            printf("[stability-test] failed to create %s\n", path);
            return 10;
        }

        ssize_t bytes = write(fd, payload, strlen(payload));
        close(fd);
        if (bytes != (ssize_t)strlen(payload)) {
            printf("[stability-test] failed to write %s\n", path);
            return 11;
        }

        int fd2 = open(path, O_RDONLY, 0);
        if (fd2 < 0) {
            printf("[stability-test] failed to reopen %s\n", path);
            return 12;
        }

        memset(buffer, 0, sizeof(buffer));
        ssize_t read_count = read(fd2, buffer, strlen(payload));
        close(fd2);
        if (read_count != (ssize_t)strlen(payload)) {
            printf("[stability-test] failed to read %s after write\n", path);
            return 13;
        }
        buffer[read_count] = '\0';

        if (strcmp(buffer, payload) != 0) {
            printf("[stability-test] corrupted content for %s\n", path);
            return 14;
        }

        struct stat st;
        if (stat(path, &st) != 0) {
            printf("[stability-test] stat failed for %s\n", path);
            return 15;
        }
        if (st.st_size != (off_t)strlen(payload)) {
            printf("[stability-test] stat size mismatch for %s\n", path);
            return 16;
        }

        if (unlink(path) != 0) {
            printf("[stability-test] failed to unlink %s\n", path);
            return 17;
        }

        if ((idx & 0x03) == 0) {
            printf("[stability-test] file round %d ok (%s)\n", idx, path);
        }
    }

    return 0;
}

static int run_descriptor_test(void) {
    char path[MAX_PATH];
    int fds[FD_STRESS];
    char payload[64];

    for (int idx = 0; idx < FD_STRESS; idx++) {
        sprintf(path, "/fd_stress_%02d.tmp", idx);
        fds[idx] = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
        if (fds[idx] < 0) {
            printf("[stability-test] failed to open descriptor %d\n", idx);
            return 20;
        }

        int len = sprintf(payload, "fd stress %02d\n", idx);
        if (write(fds[idx], payload, len) != len) {
            printf("[stability-test] failed to write descriptor %d\n", idx);
            return 21;
        }
    }

    for (int idx = 0; idx < FD_STRESS; idx++) {
        if (close(fds[idx]) != 0) {
            printf("[stability-test] failed to close descriptor %d\n", idx);
            return 22;
        }
    }

    for (int idx = 0; idx < FD_STRESS; idx++) {
        sprintf(path, "/fd_stress_%02d.tmp", idx);
        int fd = open(path, O_RDONLY, 0);
        if (fd < 0) {
            printf("[stability-test] failed to reopen %s\n", path);
            return 23;
        }
        memset(payload, 0, sizeof(payload));
        ssize_t read_count = read(fd, payload, sizeof(payload) - 1);
        close(fd);
        if (read_count <= 0) {
            printf("[stability-test] failed to read reopened %s\n", path);
            return 24;
        }
        if (strstr(payload, "fd stress") == NULL) {
            printf("[stability-test] invalid content in %s\n", path);
            return 25;
        }
        if (unlink(path) != 0) {
            printf("[stability-test] failed to unlink %s\n", path);
            return 26;
        }
    }

    printf("[stability-test] descriptor stress passed (%d fds)\n", FD_STRESS);
    return 0;
}

static int run_directory_test(void) {
    int fd = open("/", O_RDONLY, 0);
    if (fd < 0) {
        printf("[stability-test] failed to open root directory\n");
        return 30;
    }

    struct dirent entries[32];
    ssize_t count = getdents(fd, entries, sizeof(entries));
    close(fd);
    if (count <= 0) {
        printf("[stability-test] getdents returned %zd\n", count);
        return 31;
    }

    printf("[stability-test] directory listing returned %zd bytes\n", count);
    return 0;
}

static int run_time_test(void) {
    struct timespec first;
    struct timespec second;

    if (syscall(SYS_clock_gettime, CLOCK_REALTIME, (long)&first, 0, 0, 0) != 0) {
        printf("[stability-test] initial clock_gettime failed\n");
        return 40;
    }

    if (syscall(SYS_clock_gettime, CLOCK_REALTIME, (long)&second, 0, 0, 0) != 0) {
        printf("[stability-test] second clock_gettime failed\n");
        return 41;
    }

    if (second.tv_sec < first.tv_sec ||
        (second.tv_sec == first.tv_sec && second.tv_nsec < first.tv_nsec)) {
        printf("[stability-test] clock time moved backwards\n");
        return 42;
    }

    printf("[stability-test] clock time is monotonic: %ld.%09ld -> %ld.%09ld\n",
           (long)first.tv_sec, (long)first.tv_nsec,
           (long)second.tv_sec, (long)second.tv_nsec);
    return 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("[stability-test] starting combined user-mode stress checks\n");

    int result = run_heap_test();
    if (result != 0) return result;

    result = run_filesystem_test();
    if (result != 0) return result;

    result = run_descriptor_test();
    if (result != 0) return result;

    result = run_directory_test();
    if (result != 0) return result;

    result = run_time_test();
    if (result != 0) return result;

    printf("[stability-test] all checks passed\n");
    return 0;
}
