#include "commands.h"
#include "stdio.h"
#include "io.h"

extern void terminal_write(const char*);

int cmd_reboot(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("Rebooting system...\n");
    
    /* Send reboot command via keyboard controller */
    uint8_t status = inb(0x64);
    while ((status & 0x02) != 0) {
        status = inb(0x64);
    }
    outb(0x64, 0xFE);
    
    /* Fallback: triple fault to trigger reboot */
    __asm__ volatile ("cli; lidt (0x0); int $3");
    
    return 0;
}
