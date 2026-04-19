#ifndef IO_H
#define IO_H

#include "stddef.h"
#include "stdint.h"
#include "string.h"

#define TERM_WIDTH  80
#define TERM_HEIGHT 25

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0); /* Write to an unused port to create a small delay */
}

static inline void pc_speaker_beep(uint32_t frequency, uint32_t duration_ms) {
    if (frequency == 0 || duration_ms == 0)
        return;

    uint16_t divisor = (uint16_t)(1193180 / frequency);
    outb(0x43, 0xB6);
    outb(0x42, divisor & 0xFF);
    outb(0x42, (divisor >> 8) & 0xFF);

    uint8_t portb = inb(0x61);
    outb(0x61, portb | 0x03);

    volatile uint32_t delay = duration_ms * 2000;
    while (delay--) {
        io_wait();
    }

    portb = inb(0x61);
    outb(0x61, portb & ~0x03);
}

extern size_t terminal_row;
extern size_t terminal_col;

char keyboard_getchar(void);
char keyboard_pollchar(void);

void read_line(char* buf, size_t size);

void move_cursor(int x, int y);

void terminal_putc(char c);

void terminal_set_color(uint8_t color);
uint8_t terminal_get_color(void);

void terminal_backspace(void);

void terminal_write(const char* s);

void serial_write(const char* s);

void serial_init(void);

#endif