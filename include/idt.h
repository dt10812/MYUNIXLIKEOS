#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// IDT Entry structure for x86_64
struct idt_entry {
    uint16_t isr_low;      // Lower 16 bits of ISR's address
    uint16_t kernel_cs;    // Kernel code segment selector
    uint8_t  ist;          // Interrupt Stack Table offset
    uint8_t  attributes;   // Type and attributes (Flags)
    uint16_t isr_mid;      // Middle 16 bits of ISR's address
    uint32_t isr_high;     // Higher 32 bits of ISR's address
    uint32_t reserved;     // Set to zero
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void idt_init(void);
void set_idt_gate(int vector, void* isr, uint8_t flags);

#endif