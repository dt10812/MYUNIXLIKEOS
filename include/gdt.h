#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// In 64-bit mode, TSS descriptors are 16 bytes, while code/data are 8 bytes.
// We use a packed structure to prevent the compiler from adding padding.

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_tss_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid_low;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_mid_high;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// Task State Segment (TSS) structure for x86_64
// This is required for the CPU to find the stack when an interrupt happens.
struct tss {
    uint32_t reserved0;
    uint64_t rsp0;      // Stack pointer for Ring 0 (Kernel)
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;      // Interrupt Stack Table 1
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed));

// GDT Index Constants
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_DATA   0x18
#define GDT_USER_CODE   0x20
#define GDT_TSS         0x28

// API Functions
void gdt_init();
void gdt_load(struct gdt_ptr* gdt_ptr);
void tss_write(uint32_t gdt_index, uint64_t stack_address);

#endif