/* vga mode 3 driver */

#include "stdint.h"
#include "string.h"
#include "io.h"

static volatile uint16_t* const VGA_BUFFER = (volatile uint16_t*)0xB8000;
size_t terminal_row = 0;
size_t terminal_col = 0;
uint8_t terminal_color = 0x0F;

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
        if (terminal_row >= TERM_HEIGHT) terminal_row = TERM_HEIGHT - 1;
        move_cursor(terminal_col, terminal_row);
        return;
    }
    VGA_BUFFER[terminal_row * TERM_WIDTH + terminal_col] =
        ((uint16_t)terminal_color << 8) | (uint8_t)c;
    terminal_col++;
    if (terminal_col >= TERM_WIDTH) {
        terminal_col = 0;
        terminal_row++;
        if (terminal_row >= TERM_HEIGHT) terminal_row = TERM_HEIGHT - 1;
    }
    move_cursor(terminal_col, terminal_row);
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
    VGA_BUFFER[terminal_row * TERM_WIDTH + terminal_col] = ' ';
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