# TODO: Create individual Makefiles per directory and invoke from here

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

# Directories
SYS_DIR      = ./kernel
DRVR_DIR     = $(SYS_DIR)/drivers
DRVR_INP_DIR = $(DRVR_DIR)/input
DRVR_VGA_DIR = $(DRVR_DIR)/video
BIN_DIR      = ./bin
CMD_DIR      = $(BIN_DIR)/sh
ISO_DIR      = ./iso

LIB_DIR      = ./lib
LIBC_DIR     = $(LIB_DIR)/libc
FCNTL_DIR    = $(LIBC_DIR)/fcntl
STDIO_DIR    = $(LIBC_DIR)/stdio
STRING_DIR   = $(LIBC_DIR)/string
UNISTD_DIR   = $(LIBC_DIR)/unistd
INCLUDE_DIR  = ./include

USER_DIR      = ./usr
USER_LIBC_DIR = $(LIB_DIR)/libc
USER_OBJ_DIR  = $(USER_DIR)/obj
USER_PROG_DIR = $(USER_DIR)/programs

OBJ_DIR       = $(USER_DIR)/obj/sys

# Kernel flags
ASFLAGS = -f elf32
CFLAGS  = -ffreestanding -O2 -m32 -fno-stack-protector -fno-builtin \
          -Wall -Wextra -I$(INCLUDE_DIR) -Icommands
LDFLAGS = -m elf_i386 -n -T linker.ld

# Userspace flags
USER_CFLAGS = -ffreestanding -O2 -m32 -fno-stack-protector -fno-builtin \
              -fno-pie -no-pie \
              -Wall -Wextra -I$(INCLUDE_DIR)
USER_LDFLAGS = -m elf_i386 -T user.ld

# Kernel sources

SRC = \
	$(SYS_DIR)/kernel.c \
	$(SYS_DIR)/fs.c \
	$(SYS_DIR)/idt.c \
	$(SYS_DIR)/gdt.c \
	$(SYS_DIR)/syscall.c \
	$(SYS_DIR)/elf.c \
	$(SYS_DIR)/sys/stat/mkdir.c \
	$(SYS_DIR)/sys/time/time.c \
	$(SYS_DIR)/sys/time/localtime_r.c \
	$(DRVR_INP_DIR)/pskey.c \
	$(DRVR_VGA_DIR)/vgamode3.c \
	$(DRVR_DIR)/nvram/nvram.c \
	$(BIN_DIR)/ls/ls.c \
	$(BIN_DIR)/mkdir/sh_mkdir.c \
	$(BIN_DIR)/cat/cat.c \
	$(BIN_DIR)/grep/grep.c \
	$(BIN_DIR)/echo/echo.c \
	$(BIN_DIR)/color/color.c \
	$(BIN_DIR)/vi/vi.c \
	$(BIN_DIR)/nano/nano.c \
	$(CMD_DIR)/sh.c \
	$(CMD_DIR)/cd/cd.c \
	$(CMD_DIR)/help/help.c \
	$(CMD_DIR)/touch/touch.c \
	$(CMD_DIR)/clear/clear.c \

# Kernel libc sources
STDIO_SRC = \
	$(STDIO_DIR)/getchar.c \
	$(STDIO_DIR)/putc.c \
	$(STDIO_DIR)/putchar.c \
	$(STDIO_DIR)/printf.c \

STRING_SRC = \
	$(STRING_DIR)/memset.c \
	$(STRING_DIR)/memcpy.c \
	$(STRING_DIR)/strcmp.c \
	$(STRING_DIR)/strlen.c \
	$(STRING_DIR)/strcpy.c \
	$(STRING_DIR)/strstr.c \
	$(STRING_DIR)/strncpy.c \

UNISTD_SRC = \
	$(UNISTD_DIR)/write.c \
	$(UNISTD_DIR)/execl.c \

FCNTL_SRC = \
	$(FCNTL_DIR)/creat.c \

LIBC_SRC  = $(STDIO_SRC) $(STRING_SRC) $(FCNTL_SRC) $(UNISTD_SRC)

# Userspace libc sources
USER_LIBC_SRC = \
	$(USER_LIBC_DIR)/stdio/printf.c \
	$(USER_LIBC_DIR)/stdio/putchar.c \
	$(USER_LIBC_DIR)/stdio/putc.c \
	$(USER_LIBC_DIR)/string/strlen.c \
	$(USER_LIBC_DIR)/string/strcpy.c \
	$(USER_LIBC_DIR)/string/strcmp.c \
	$(USER_LIBC_DIR)/string/memcpy.c \
	$(USER_LIBC_DIR)/string/memset.c \
	$(USER_LIBC_DIR)/unistd/write.c \
	$(USER_LIBC_DIR)/stdio/streams.c \

# User programs
# To add a new program: append its source path here.
# The makefile auto-generates the incbin ASM and k_install C code.

USER_PROG_SRCS = \
	bin/hello/hello.c \

USER_PROGS = $(patsubst %.c,$(USER_OBJ_DIR)/%.elf,$(notdir $(USER_PROG_SRCS)))

# Output names
ELF   = kernel.elf
BIN   = kernel.bin
ISO   = os-bios.iso
LIBC  = $(LIB_DIR)/libc.a
ULIBC = $(USER_OBJ_DIR)/libc.a

QEMU      = qemu-system-x86_64
QEMUFLAGS = -m 2G -boot d \
            -drive file=$(ISO),format=raw,if=ide,media=cdrom \
            -serial mon:stdio \
            -d int,cpu_reset -no-reboot

# Object paths
KERN_OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRC:.c=.o)))
LIBC_OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(LIBC_SRC:.c=.o)))
USER_LIBC_OBJS = $(addprefix $(USER_OBJ_DIR)/, $(notdir $(USER_LIBC_SRC:.c=.o)))

# Targets
.PHONY: all run clean user

all: user $(ISO)

# Kernel libc
define libc_rule
$(OBJ_DIR)/$(notdir $(1:.c=.o)): $(1) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $$< -o $$@
endef

$(foreach src,$(LIBC_SRC),$(eval $(call libc_rule,$(src))))

$(LIBC): $(LIBC_OBJS) | $(OBJ_DIR)
	$(AR) rcs $@ $^

# Auto-generate user_progs.asm and user_progs.c
# user_progs.asm : one incbin per ELF, exports name_start/name_end symbols
# user_progs.c   : calls k_install for every program at boot
# Both are regenerated whenever USER_PROGS changes.
$(OBJ_DIR)/user_progs.asm: $(USER_PROGS) | $(OBJ_DIR)
	@echo "section .rodata" > $@
	@$(foreach elf,$^, \
		name=$(basename $(notdir $(elf))); \
		echo "global $${name}_start, $${name}_end" >> $@; \
		echo "$${name}_start: incbin \"$(elf)\"" >> $@; \
		echo "$${name}_end:" >> $@; \
	)

$(OBJ_DIR)/user_progs.c: $(USER_PROGS) | $(OBJ_DIR)
	@echo "#include \"vfs.h\""  > $@
	@echo "#include <stdint.h>" >> $@
	@$(foreach elf,$^, \
		name=$(basename $(notdir $(elf))); \
		echo "extern uint8_t $${name}_start[], $${name}_end[];" >> $@; \
	)
	@echo "void install_user_progs(void) {" >> $@
	@$(foreach elf,$^, \
		name=$(basename $(notdir $(elf))); \
		echo "    k_install(\"$${name}\", $${name}_start, (uint32_t)($${name}_end - $${name}_start));" >> $@; \
	)
	@echo "}" >> $@

$(OBJ_DIR)/user_progs_bin.o: $(OBJ_DIR)/user_progs.asm | $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/user_progs.o: $(OBJ_DIR)/user_progs.c $(OBJ_DIR)/user_progs_bin.o | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Kernel objects
define kern_rule
$(OBJ_DIR)/$(notdir $(1:.c=.o)): $(1) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $$< -o $$@
endef

$(foreach src,$(SRC),$(eval $(call kern_rule,$(src))))

$(OBJ_DIR)/entry.o: entry.asm | $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Kernel ELF
$(ELF): $(OBJ_DIR)/entry.o $(OBJ_DIR)/user_progs_bin.o $(OBJ_DIR)/user_progs.o $(KERN_OBJS) $(LIBC) | $(OBJ_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_DIR)/entry.o $(OBJ_DIR)/user_progs_bin.o $(OBJ_DIR)/user_progs.o $(KERN_OBJS) $(LIBC)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# ISO
$(ISO): $(ELF) $(BIN)
	@test -n "$(GRUB_RESCUE)" || { echo "ERROR: grub-mkrescue not found."; exit 1; }
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(ELF) $(ISO_DIR)/boot/kernel.elf
	printf 'set timeout=5\nset default=0\nmenuentry "MYUNIXLIKEOS" {\n    multiboot2 /boot/kernel.elf\n    boot\n}\n' \
		> $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB_RESCUE) -o $@ $(ISO_DIR)

# Userspace libc
define user_libc_rule
$(USER_OBJ_DIR)/$(notdir $(1:.c=.o)): $(1) | $(USER_OBJ_DIR)
	$(CC) $(USER_CFLAGS) -c $$< -o $$@
endef

$(foreach src,$(USER_LIBC_SRC),$(eval $(call user_libc_rule,$(src))))

$(ULIBC): $(USER_LIBC_OBJS) | $(USER_OBJ_DIR)
	$(AR) rcs $@ $^

# crt0
$(USER_OBJ_DIR)/crt0.o: $(USER_DIR)/crt0.asm | $(USER_OBJ_DIR)
	$(AS) -f elf32 $< -o $@

# User programs
define user_prog_rule
$(USER_OBJ_DIR)/$(notdir $(1:.c=.elf)): $(1) $(ULIBC) $(USER_OBJ_DIR)/crt0.o | $(USER_OBJ_DIR)
	$(CC) $(USER_CFLAGS) -c $$< -o $(USER_OBJ_DIR)/$(notdir $(1:.c=.o))
	$(LD) $(USER_LDFLAGS) \
		$(USER_OBJ_DIR)/crt0.o \
		$(USER_LIBC_OBJS) \
		$(USER_OBJ_DIR)/$(notdir $(1:.c=.o)) \
		-o $$@
endef

$(foreach src,$(USER_PROG_SRCS),$(eval $(call user_prog_rule,$(src))))

user: $(USER_PROGS)

# Build dirs
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(USER_OBJ_DIR):
	mkdir -p $(USER_OBJ_DIR)

# Run
run: all
	$(QEMU) $(QEMUFLAGS)

# Clean
clean:
	rm -rf $(OBJ_DIR) $(USER_OBJ_DIR) $(ISO_DIR) $(ISO) $(ELF) $(BIN) $(LIBC)