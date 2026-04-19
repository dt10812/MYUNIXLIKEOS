#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef enum {
    KEYBOARD_LAYOUT_QWERTY,
    KEYBOARD_LAYOUT_AZERTY
} keyboard_layout_t;

void keyboard_init(void);
void keyboard_set_layout(keyboard_layout_t layout);
char keyboard_getchar(void);

#endif