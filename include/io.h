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

#endif