#include "idt.h"
#include "gdt.h"
#include "io.h"
#include "stdio.h"
#include "sys/syscall.h"

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

extern void _syscall(void);
extern void *isr_table[];
extern uintptr_t get_physical_address(uintptr_t virtual);

static void serial_write_dec_local(uint32_t value) {
    char buf[12];
    int i = 0;
    if (value == 0) {
        serial_write("0");
        return;
    }
    while (value > 0 && i < (int)sizeof(buf) - 1) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        char c[2] = {buf[j], '\0'};
        serial_write(c);
    }
}

struct trapframe {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; /* pusha */
    uint32_t trapno, err_code;
    /* auto-pushed by the cpu */
    uint32_t eip, cs, eflags;
} __attribute__((packed));

void fault_handler(struct trapframe *regs) {
    static const char *names[] = {
        "#DE", "#DB", "NMI", "#BP", "#OF", "#BR", "#UD", "#NM",
        "#DF", "??",  "#TS", "#NP", "#SS", "#GP", "#PF", "??",
        "#MF", "#AC", "#MC", "#XF"
    };
    const char *name = regs->trapno < 20 ? names[regs->trapno] : "??";

    uintptr_t cr2;
    uintptr_t cr3;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

    serial_write("FAULT trapno=");
    serial_write_dec_local(regs->trapno);
    serial_write(" name=");
    serial_write(name);
    serial_write(" err=");
    serial_write_dec_local(regs->err_code);
    serial_write(" cr2=");
    serial_write_dec_local((uint32_t)cr2);
    serial_write(" cr3=");
    serial_write_dec_local((uint32_t)cr3);
    uintptr_t phys = get_physical_address((uintptr_t)cr2);
    serial_write(" phys=");
    serial_write_dec_local((uint32_t)phys);
    serial_write(" eip=");
    serial_write_dec_local(regs->eip);
    serial_write(" cs=");
    serial_write_dec_local(regs->cs);
    serial_write(" eflags=");
    serial_write_dec_local(regs->eflags);
    serial_write(" esp=");
    serial_write_dec_local(regs->esp_dummy);
    serial_write(" ebp=");
    serial_write_dec_local(regs->ebp);
    serial_write(" eax=");
    serial_write_dec_local(regs->eax);
    serial_write(" ebx=");
    serial_write_dec_local(regs->ebx);
    serial_write(" ecx=");
    serial_write_dec_local(regs->ecx);
    serial_write(" edx=");
    serial_write_dec_local(regs->edx);
    serial_write("\n");
    
    if (regs->trapno == 14) { /* Page Fault */
        
        /* Clear screen and set blue background */
        for (int i = 0; i < 80 * 25; i++) {
            ((uint16_t*)0xB8000)[i] = 0x1F00; /* Blue background, black text */
        }
        
        /* Display blue screen of death */
        const char *bsod_title = "MYUNIXLIKEOS - KERNEL PANIC";
        const char *bsod_msg = "A fatal error has occurred and the system has been halted.";
        const char *pf_msg = "PAGE FAULT";
        
        int pos = 80 * 2 + 20; /* Row 2, column 20 */
        for (int i = 0; bsod_title[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | bsod_title[i];
        }
        
        pos = 80 * 4 + 10;
        for (int i = 0; bsod_msg[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | bsod_msg[i];
        }
        
        pos = 80 * 6 + 30;
        for (int i = 0; pf_msg[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | pf_msg[i];
        }
        
        /* Display register values */
        char reg_buf[32];
        pos = 80 * 8 + 10;
        
        sprintf(reg_buf, "EAX: 0x%08X", regs->eax);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        pos = 80 * 9 + 10;
        sprintf(reg_buf, "EBX: 0x%08X", regs->ebx);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        pos = 80 * 10 + 10;
        sprintf(reg_buf, "ECX: 0x%08X", regs->ecx);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        pos = 80 * 11 + 10;
        sprintf(reg_buf, "EDX: 0x%08X", regs->edx);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        pos = 80 * 12 + 10;
        sprintf(reg_buf, "ESP: 0x%08X", regs->esp_dummy);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        pos = 80 * 13 + 10;
        sprintf(reg_buf, "EBP: 0x%08X", regs->ebp);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        pos = 80 * 14 + 10;
        sprintf(reg_buf, "ESI: 0x%08X", regs->esi);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        pos = 80 * 15 + 10;
        sprintf(reg_buf, "EDI: 0x%08X", regs->edi);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        pos = 80 * 16 + 10;
        sprintf(reg_buf, "EIP: 0x%08X", regs->eip);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        pos = 80 * 17 + 10;
        sprintf(reg_buf, "Error Code: 0x%08X", regs->err_code);
        for (int i = 0; reg_buf[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | reg_buf[i];
        }
        
        const char *halt_msg = "System halted. Please restart your computer.";
        pos = 80 * 20 + 15;
        for (int i = 0; halt_msg[i]; i++) {
            ((uint16_t*)0xB8000)[pos++] = 0x1F00 | halt_msg[i];
        }
    } else {
        printf("\nFAULT %s (int=0x%x) err=0x%x\n", name, regs->trapno, regs->err_code);
        printf("System halted.\n");
    }
    
    __asm__ volatile ("cli; hlt");
}

void pic_remap(int offset1, int offset2) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, offset1);
    outb(0xA1, offset2);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void set_idt_gate(int vector, void* isr, uint8_t flags) {
    uintptr_t addr = (uintptr_t)isr;
    idt[vector].isr_low    = addr & 0xFFFF;
    idt[vector].isr_high   = (addr >> 16) & 0xFFFF;
    idt[vector].kernel_cs  = GDT_KERNEL_CODE;
    idt[vector].reserved   = 0;
    idt[vector].attributes = flags;
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < 32; i++)
        set_idt_gate(i, isr_table[i], 0x8E);

    for (int i = 32; i < 256; i++)
        set_idt_gate(i, isr_table[0], 0x8E);

    /* syscall — DPL=3 */
    set_idt_gate(0x80, _syscall, 0xEE);

    __asm__ volatile ("lidt %0" : : "m"(idtp));
    pic_remap(0x20, 0x28);
    __asm__ volatile ("sti");
}