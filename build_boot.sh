#!/usr/bin/env bash
set -euo pipefail

if command -v i686-elf-grub-mkrescue >/dev/null 2>&1; then
    GRUB_RESCUE="i686-elf-grub-mkrescue"
elif command -v grub-mkrescue >/dev/null 2>&1; then
    GRUB_RESCUE="grub-mkrescue"
else
    echo "ERROR: grub-mkrescue not found."
    exit 1
fi

nasm -f elf entry.asm -o entry.o

i686-elf-gcc -ffreestanding -O2 -m32 -fno-stack-protector -fno-builtin -Wall -Wextra -Iheaders -Icommands \
    -c kernel/kernel.c commands/*.c

i686-elf-ld -m elf_i386 -n -T linker.ld -o kernel.elf entry.o kernel.o ls.o mkdir.o touch.o cat.o cd.o grep.o fs.o help.o

i686-elf-objcopy -O binary kernel.elf kernel.bin

rm -rf iso
mkdir -p iso/boot/grub
cp kernel.elf iso/boot/kernel.elf
cat > iso/boot/grub/grub.cfg <<'GRUBCFG'
set timeout=5
set default=0

menuentry "MYUNIXLIKEOS" {
    multiboot2 /boot/kernel.elf
    boot
}
GRUBCFG

$GRUB_RESCUE -o os-bios.iso iso

qemu-system-x86_64 -m 2G -boot d -drive file=os-bios.iso,format=raw,if=ide,media=cdrom -serial mon:stdio 
