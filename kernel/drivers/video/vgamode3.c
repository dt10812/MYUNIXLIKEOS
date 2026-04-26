/* vga mode 3 driver */

#include "stdint.h"
#include "string.h"
#include "io.h"

static volatile uint16_t* const VGA_BUFFER = (volatile uint16_t*)0xB8000;
size_t terminal_row = 0;
size_t terminal_col = 0;
uint8_t terminal_color = 0x0F;

/* Hardware-aware VGA scroll: shift content up by one row and clear bottom row */
static void vga_scroll_up(void) {
    /* Copy rows 1-24 to rows 0-23 (shift up by one row) */
    /* Each row is TERM_WIDTH (80) uint16_t entries = 160 bytes */
    memmove((void*)(VGA_BUFFER), 
            (const void*)(VGA_BUFFER + TERM_WIDTH), 
            (TERM_HEIGHT - 1) * TERM_WIDTH * sizeof(uint16_t));
    
    /* Clear the bottom row (row 24) */
    for (size_t i = 0; i < TERM_WIDTH; i++) {
        VGA_BUFFER[(TERM_HEIGHT - 1) * TERM_WIDTH + i] = 
            ((uint16_t)terminal_color << 8) | ' ';
    }
    
    /* Position cursor at start of last row */
    terminal_row = TERM_HEIGHT - 1;
    terminal_col = 0;
}

void move_cursor(int x, int y) {
    uint16_t pos = y * 80 + x;
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
}

void terminal_putc(char c) {
    if(c == 0x1B)
        return; /* early exit on esc */
    if (c == '\n') {
        terminal_col = 0;
        terminal_row++;
        if (terminal_row >= TERM_HEIGHT) {
            vga_scroll_up();
        } else {
            move_cursor(terminal_col, terminal_row);
        }
        return;
    }
    VGA_BUFFER[terminal_row * TERM_WIDTH + terminal_col] =
        ((uint16_t)terminal_color << 8) | (uint8_t)c;
    terminal_col++;
    if (terminal_col >= TERM_WIDTH) {
        terminal_col = 0;
        terminal_row++;
        if (terminal_row >= TERM_HEIGHT) {
            vga_scroll_up();
        } else {
            move_cursor(terminal_col, terminal_row);
        }
    } else {
        move_cursor(terminal_col, terminal_row);
    }
}

void terminal_backspace(void) {
    if (terminal_col == 0) {
        if (terminal_row == 0)
            return;
        terminal_row--;
        terminal_col = TERM_WIDTH - 1;
    } else {
        terminal_col--;
    }
    VGA_BUFFER[terminal_row * TERM_WIDTH + terminal_col] = 
        ((uint16_t)terminal_color << 8) | ' ';
    move_cursor(terminal_col, terminal_row);
}

void terminal_set_color(uint8_t color) {
    terminal_color = color;
}

uint8_t terminal_get_color(void) {
    return terminal_color;
}

void terminal_write(const char* s) {
    for(; *s; terminal_putc(*s++));
}

void serial_write(const char* s) {
    for(; *s; s++) {
        while ((inb(0x3FD) & 0x20) == 0); // Wait for transmit buffer empty
        outb(0x3F8, *s);
    }
}

void serial_init(void) {
    outb(0x3F9, 0x00); // Disable interrupts
    outb(0x3FB, 0x80); // Enable DLAB
    outb(0x3F8, 0x03); // Set divisor to 3 (38400 baud)
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x03); // 8 bits, no parity, one stop bit
    outb(0x3FC, 0x00); // No flow control
    outb(0x3F9, 0x01); // Enable interrupts for received data
}