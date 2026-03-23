.SUFFIXES:

AS      = nasm
CC      = i686-elf-gcc
LD      = i686-elf-ld
AR      = i686-elf-ar
OBJCOPY = i686-elf-objcopy

GRUB_RESCUE := $(shell \
	if command -v i686-elf-grub-mkrescue >/dev/null 2>&1; then \
		echo i686-elf-grub-mkrescue; \
	elif command -v grub-mkrescue >/dev/null 2>&1; then \
		echo grub-mkrescue; \
	fi)

SRC_DIR    = ./kernel
CMD_DIR    = ./commands
BIN_DIR    = ./bin
ISO_DIR    = ./iso

LIB_DIR    = ./lib
LIBC_DIR   = $(LIB_DIR)/libc
STDIO_DIR  = $(LIBC_DIR)/stdio
STRING_DIR = $(LIBC_DIR)/string
INCLUDE_DIR = ./include

ASFLAGS = -f elf
CFLAGS  = -ffreestanding -O2 -m32 -fno-stack-protector -fno-builtin \
          -Wall -Wextra -I$(INCLUDE_DIR) -Icommands
LDFLAGS = -m elf_i386 -n -T linker.ld

SRC = \
	$(SRC_DIR)/kernel.c \
	$(CMD_DIR)/ls.c \
	$(CMD_DIR)/mkdir.c \
	$(CMD_DIR)/touch.c \
	$(CMD_DIR)/cat.c \
	$(CMD_DIR)/cd.c \
	$(CMD_DIR)/grep.c \
	$(CMD_DIR)/fs.c \
	$(CMD_DIR)/help.c \
	$(CMD_DIR)/echo.c \
	$(BIN_DIR)/vi/vi.c \

STDIO_SRC = \
	$(STDIO_DIR)/getchar.c \
	$(STDIO_DIR)/putchar.c \

STRING_SRC = \
	$(STRING_DIR)/memset.c \
	$(STRING_DIR)/strcmp.c \
	$(STRING_DIR)/strlen.c \

ELF = kernel.elf
BIN = kernel.bin
ISO = os-bios.iso

QEMU      = qemu-system-x86_64
QEMUFLAGS = -m 2G -boot d \
            -drive file=$(ISO),format=raw,if=ide,media=cdrom \
            -serial mon:stdio

LIBC_SRC  = $(STDIO_SRC) $(STRING_SRC)
LIBC_OBJS = $(notdir $(LIBC_SRC:.c=.o))
LIBC      = $(LIB_DIR)/libc.a

KERN_OBJS = $(notdir $(SRC:.c=.o))

.PHONY: all run clean libc

all: $(ISO)

entry.o: entry.asm
	$(AS) $(ASFLAGS) $< -o $@

libc:
	$(CC) $(CFLAGS) -c $(LIBC_SRC)
	$(AR) rcs $(LIBC) $(LIBC_OBJS)

$(ELF): entry.o libc
	$(CC) $(CFLAGS) -c $(SRC)
	$(LD) $(LDFLAGS) -o $@ entry.o $(KERN_OBJS) $(LIBC)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(ISO): $(ELF) $(BIN)
	@test -n "$(GRUB_RESCUE)" || { echo "ERROR: grub-mkrescue not found."; exit 1; }
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(ELF) $(ISO_DIR)/boot/kernel.elf
	printf 'set timeout=5\nset default=0\nmenuentry "MYUNIXLIKEOS" {\n    multiboot2 /boot/kernel.elf\n    boot\n}\n' \
		> $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_RESCUE) -o $@ $(ISO_DIR)

run: all
	$(QEMU) $(QEMUFLAGS)

clean:
	rm -f entry.o $(KERN_OBJS) $(LIBC_OBJS) $(LIBC) $(ELF) $(BIN)
	rm -rf $(ISO_DIR) $(ISO)