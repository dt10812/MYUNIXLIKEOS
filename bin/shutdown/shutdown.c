#include "commands.h"
#include "stdio.h"

extern void terminal_write(const char*);

int cmd_shutdown(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("Shutting down system...\n");
    terminal_write("Goodbye!\n");
    
    /* Halt the CPU */
    __asm__ volatile ("cli");
    __asm__ volatile ("hlt");
    
    return 0;
}
