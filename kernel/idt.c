#include "idt.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

void set_idt_gate(int vector, void* isr, uint8_t flags) {
    uintptr_t addr = (uintptr_t)isr;
    idt[vector].isr_low = addr & 0xFFFF;
    idt[vector].kernel_cs = 0x08; // Your Kernel Code Segment in GDT
    idt[vector].ist = 0;
    idt[vector].attributes = flags;
    idt[vector].isr_mid = (addr >> 16) & 0xFFFF;
    idt[vector].isr_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].reserved = 0;
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uintptr_t)&idt;

    // You need assembly stubs for these. For now, we point to a dummy.
    // set_idt_gate(33, keyboard_handler_stub, 0x8E); 

    __asm__ volatile ("lidt %0" : : "m"(idtp));
    __asm__ volatile ("sti"); // Enable interrupts
}
