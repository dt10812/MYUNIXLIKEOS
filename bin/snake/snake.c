#include "commands.h"
#include "stdio.h"
#include "string.h"
#include "stdint.h"
#include "stdbool.h"
#include "io.h"

#define BOARD_ORIGIN_X 1
#define BOARD_ORIGIN_Y 1
#define BOARD_WIDTH  30
#define BOARD_HEIGHT 16
#define BOARD_MAX_SNAKE 128

static const struct {
    const char *name;
    uint8_t value;
} color_table[] = {
    {"black",        0x00},
    {"blue",         0x01},
    {"green",        0x02},
    {"cyan",         0x03},
    {"red",          0x04},
    {"magenta",      0x05},
    {"brown",        0x06},
    {"lightgray",    0x07},
    {"darkgray",     0x08},
    {"lightblue",    0x09},
    {"lightgreen",   0x0A},
    {"lightcyan",    0x0B},
    {"lightred",     0x0C},
    {"lightmagenta", 0x0D},
    {"yellow",       0x0E},
    {"white",        0x0F},
};

static uint8_t snake_color = 0x0A;
static uint8_t food_color = 0x0C;
static uint8_t border_color = 0x07;
static uint8_t board_color = 0x00;
static uint8_t text_color = 0x0E;
static uint32_t snake_rng = 0x12345678;

static uint8_t snake_x[BOARD_MAX_SNAKE];
static uint8_t snake_y[BOARD_MAX_SNAKE];
static uint8_t food_x;
static uint8_t food_y;
static uint8_t snake_length;

static void snake_set_color(uint8_t attr) {
    terminal_set_color(attr);
}

static int parse_color_value(const char *arg) {
    if (!arg) return -1;

    if (arg[0] == '0' && arg[1] == 'x' && arg[2] && !arg[3]) {
        char c = arg[2];
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    for (size_t i = 0; i < sizeof(color_table) / sizeof(color_table[0]); i++) {
        if (!strcmp(color_table[i].name, arg))
            return color_table[i].value;
    }

    return -1;
}

static uint32_t snake_random(void) {
    snake_rng ^= snake_rng << 13;
    snake_rng ^= snake_rng >> 17;
    snake_rng ^= snake_rng << 5;
    return snake_rng;
}

static bool snake_occupies(uint8_t x, uint8_t y) {
    for (uint8_t i = 0; i < snake_length; i++) {
        if (snake_x[i] == x && snake_y[i] == y)
            return true;
    }
    return false;
}

static void snake_place_food(void) {
    while (true) {
        uint8_t x = (snake_random() % (BOARD_WIDTH - 2)) + 1;
        uint8_t y = (snake_random() % (BOARD_HEIGHT - 2)) + 1;
        if (!snake_occupies(x, y)) {
            food_x = x;
            food_y = y;
            return;
        }
    }
}

static void snake_put_cell(uint8_t x, uint8_t y, char ch, uint8_t attr) {
    uint8_t previous = terminal_get_color();
    snake_set_color(attr);
    move_cursor(BOARD_ORIGIN_X + x, BOARD_ORIGIN_Y + y);
    terminal_putc(ch);
    snake_set_color(previous);
}

static void snake_clear_screen(void) {
    uint8_t previous = terminal_get_color();
    snake_set_color(0x07);
    terminal_row = 0;
    terminal_col = 0;
    for (uint8_t row = 0; row < TERM_HEIGHT; row++) {
        for (uint8_t col = 0; col < TERM_WIDTH; col++) {
            terminal_putc(' ');
        }
    }
    snake_set_color(previous);
    terminal_row = 0;
    terminal_col = 0;
    move_cursor(0, 0);
}

static void snake_draw_board_background(void) {
    for (uint8_t y = 1; y + 1 < BOARD_HEIGHT; y++) {
        for (uint8_t x = 1; x + 1 < BOARD_WIDTH; x++) {
            snake_put_cell(x, y, ' ', board_color);
        }
    }
}

static void snake_draw_border(void) {
    for (uint8_t x = 0; x < BOARD_WIDTH; x++) {
        snake_put_cell(x, 0, '#', border_color);
        snake_put_cell(x, BOARD_HEIGHT - 1, '#', border_color);
    }
    for (uint8_t y = 1; y + 1 < BOARD_HEIGHT; y++) {
        snake_put_cell(0, y, '#', border_color);
        snake_put_cell(BOARD_WIDTH - 1, y, '#', border_color);
    }
}

static void snake_draw_snake(void) {
    if (snake_length == 0) return;
    snake_put_cell(snake_x[0], snake_y[0], 'O', snake_color);
    for (uint8_t i = 1; i < snake_length; i++) {
        snake_put_cell(snake_x[i], snake_y[i], 'o', snake_color);
    }
}

static void snake_delay(void) {
    for (volatile uint32_t i = 0; i < 2500000; i++) {
        __asm__ volatile ("nop");
    }
}

static void snake_draw_info(int score) {
    uint8_t previous = terminal_get_color();
    snake_set_color(text_color);
    move_cursor(0, BOARD_ORIGIN_Y + BOARD_HEIGHT);
    printf("Score: %d  Use W,A,S,D to move. Press Q to quit.\n", score);
    snake_set_color(previous);
}

static void snake_show_game_over(int score) {
    uint8_t previous = terminal_get_color();
    snake_set_color(text_color);
    move_cursor(0, BOARD_ORIGIN_Y + BOARD_HEIGHT + 1);
    printf("Game Over! Score: %d\n", score);
    printf("Press any key to return to shell.\n");
    snake_set_color(previous);
    keyboard_getchar();
}

static void snake_reset_colors(void) {
    snake_color = 0x0A;
    food_color = 0x0C;
    border_color = 0x07;
    board_color = 0x00;
    text_color = 0x0E;
}

static bool snake_set_custom_color(const char *target, const char *value) {
    int color = parse_color_value(value);
    if (color < 0) return false;

    if (!strcmp(target, "snake")) {
        snake_color = color;
    } else if (!strcmp(target, "food")) {
        food_color = color;
    } else if (!strcmp(target, "border")) {
        border_color = color;
    } else if (!strcmp(target, "board")) {
        board_color = color;
    } else if (!strcmp(target, "text")) {
        text_color = color;
    } else {
        return false;
    }

    return true;
}

static int snake_play(void) {
    uint8_t head_x = BOARD_WIDTH / 2;
    uint8_t head_y = BOARD_HEIGHT / 2;
    snake_length = 4;

    for (uint8_t i = 0; i < snake_length; i++) {
        snake_x[i] = head_x - i;
        snake_y[i] = head_y;
    }

    int8_t dx = 1;
    int8_t dy = 0;
    int score = 0;
    snake_place_food();
    snake_clear_screen();
    snake_draw_board_background();
    snake_draw_border();
    snake_draw_snake();
    snake_put_cell(food_x, food_y, '*', food_color);
    snake_draw_info(score);

    while (true) {
        char c = keyboard_pollchar();
        if (c == 'q' || c == 'Q')
            break;

        if ((c == 'w' || c == 'W') && dy != 1) {
            dx = 0;
            dy = -1;
        } else if ((c == 's' || c == 'S') && dy != -1) {
            dx = 0;
            dy = 1;
        } else if ((c == 'a' || c == 'A') && dx != 1) {
            dx = -1;
            dy = 0;
        } else if ((c == 'd' || c == 'D') && dx != -1) {
            dx = 1;
            dy = 0;
        }

        int next_x = snake_x[0] + dx;
        int next_y = snake_y[0] + dy;

        if (next_x == 0 || next_x == BOARD_WIDTH - 1 || next_y == 0 || next_y == BOARD_HEIGHT - 1) {
            break;
        }

        bool ate_food = (next_x == food_x && next_y == food_y);
        uint8_t collision_len = snake_length - (ate_food ? 0 : 1);
        for (uint8_t i = 0; i < collision_len; i++) {
            if (snake_x[i] == (uint8_t)next_x && snake_y[i] == (uint8_t)next_y) {
                goto end_game;
            }
        }

        uint8_t tail_x = snake_x[snake_length - 1];
        uint8_t tail_y = snake_y[snake_length - 1];
        for (uint8_t i = snake_length; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
        snake_x[0] = (uint8_t)next_x;
        snake_y[0] = (uint8_t)next_y;

        if (!ate_food) {
            snake_put_cell(tail_x, tail_y, ' ', board_color);
        } else {
            if (snake_length + 1 < BOARD_MAX_SNAKE)
                snake_length++;
            score += 10;
            snake_place_food();
        }

        if (snake_length > 1) {
            snake_put_cell(snake_x[1], snake_y[1], 'o', snake_color);
        }
        snake_put_cell(snake_x[0], snake_y[0], 'O', snake_color);
        snake_put_cell(food_x, food_y, '*', food_color);
        snake_draw_info(score);

        snake_delay();
    }

end_game:
    snake_show_game_over(score);
    return 0;
}

static void snake_print_usage(void) {
    terminal_write("snake: start the snake game or configure game colors.\n");
    terminal_write("Usage:\n");
    terminal_write("  snake                 Start the game.\n");
    terminal_write("  snake color <type> <color>\n");
    terminal_write("    type: snake, food, border, board, text\n");
    terminal_write("  snake reset           Restore default game colors.\n");
    terminal_write("  snake help            Show this help.\n");
    terminal_write("Colors are names or 0xN such as green or 0xC.\n");
}

int cmd_snake(int argc, char** argv) {
    if (argc == 1 || (argc == 2 && !strcmp(argv[1], "play"))) {
        return snake_play();
    }

    if (argc == 2 && (!strcmp(argv[1], "help") || !strcmp(argv[1], "?"))) {
        snake_print_usage();
        return 0;
    }

    if (argc == 2 && !strcmp(argv[1], "reset")) {
        snake_reset_colors();
        terminal_write("Snake game colors reset to defaults.\n");
        return 0;
    }

    if (argc == 4 && !strcmp(argv[1], "color")) {
        if (!snake_set_custom_color(argv[2], argv[3])) {
            terminal_write("Invalid snake color command.\n");
            snake_print_usage();
            return 1;
        }
        terminal_write("Snake color updated.\n");
        return 0;
    }

    snake_print_usage();
    return 1;
}
