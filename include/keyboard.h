#ifndef KEYBOARD_H
#define KEYBOARD_H

#define INPUT_BUFFER_SIZE 256

typedef enum {
    KEYBOARD_LAYOUT_QWERTY,
    KEYBOARD_LAYOUT_AZERTY
} keyboard_layout_t;

extern char input_buffer[INPUT_BUFFER_SIZE];
extern size_t buffer_write_idx;
extern size_t buffer_read_idx;

void keyboard_init(void);
void keyboard_set_layout(keyboard_layout_t layout);
char keyboard_getchar(void);
char keyboard_pollchar(void);

#endif