#include "commands.h"
#include "stdio.h"
#include "string.h"
#include "io.h"

extern void terminal_write(const char*);

/* Simple recursive descent calculator parser */

static const char *expr_ptr;
static int parse_expr(void);
static int parse_term(void);
static int parse_factor(void);

static void skip_whitespace(void) {
    while (*expr_ptr && (*expr_ptr == ' ' || *expr_ptr == '\t' || *expr_ptr == '\n')) {
        expr_ptr++;
    }
}

static int parse_factor(void) {
    skip_whitespace();
    
    if (*expr_ptr == '(') {
        expr_ptr++;
        int result = parse_expr();
        skip_whitespace();
        if (*expr_ptr == ')') expr_ptr++;
        return result;
    }
    
    int num = 0;
    while (*expr_ptr >= '0' && *expr_ptr <= '9') {
        num = num * 10 + (*expr_ptr - '0');
        expr_ptr++;
    }
    skip_whitespace();
    return num;
}

static int parse_term(void) {
    int result = parse_factor();
    
    while (true) {
        skip_whitespace();
        if (*expr_ptr != '*' && *expr_ptr != '/') break;
        
        char op = *expr_ptr++;
        int right = parse_factor();
        if (op == '*')
            result = result * right;
        else if (right != 0)
            result = result / right;
    }
    
    return result;
}

static int parse_expr(void) {
    int result = parse_term();
    
    while (true) {
        skip_whitespace();
        if (*expr_ptr != '+' && *expr_ptr != '-') break;
        
        char op = *expr_ptr++;
        int right = parse_term();
        if (op == '+')
            result = result + right;
        else
            result = result - right;
    }
    
    return result;
}

static int evaluate(const char *expression) {
    expr_ptr = expression;
    int result = parse_expr();
    skip_whitespace();
    if (*expr_ptr != '\0') {
        /* Invalid expression - didn't consume all input */
        return 0;
    }
    return result;
}

int cmd_calc(int argc, char** argv) {
    if (argc < 2) {
        terminal_write("calc: Simple calculator\n");
        terminal_write("Usage: calc <expression>\n");
        terminal_write("Example: calc \"2+3*4\"\n");
        terminal_write("Supports: +, -, *, /, ()\n");
        return 0;
    }
    
    const char *expr = argv[1];
    int result = evaluate(expr);
    
    printf("= %d\n", result);
    return 0;
}
