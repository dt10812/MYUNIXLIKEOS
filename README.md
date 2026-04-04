# MYUNIXLIKEOS

A 32-bit Unix-like operating system kernel built from scratch. This project implements a freestanding environment, a Virtual File System (VFS), and a custom command-line interface.

## Features
- **Monolithic Kernel**: Written in freestanding C.
- **Bootloader**: GRUB-based booting via Multiboot2.
- **CLI**: Custom shell supporting `ls`, `cd`, `cat`, `grep`, `mkdir`, `touch`, `fs`, `clear` and a basic `nano` editor.
- **Keyboard Driver**: PS/2 Set 1 support for all letters and base symbols `-=[]\;',./` and Shift key.
- **Filesystem**: Integrated Virtual File System layer.

## Prerequisites (macOS)
This project is currently optimized for development on macOS. You will need the following tools installed via [Homebrew](https://brew.sh/):

```bash
brew install nasm qemu xorriso
# Note: You need a cross-compiler (x86_64-elf-gcc or i686-elf-gcc)
brew install x86_64-elf-gcc x86_64-elf-binutils
#IMPORTANT: When compiling using x86_64-elf-gcc, you must specify the flags to compile to a 32-bit system.

```

## Project Goals & Roadmap
The primary objective of this project is to evolve from a basic hobbyist kernel into a fully functional, POSIX-compliant Unix-like environment. 

**Current Focus:**
- **Advanced Input Handling:** Our immediate goal is to expand the keyboard driver to support the full ASCII set, including uppercase letters, shifted symbols (`!@#$%^&*()_+`), and modifier keys (Control). While the current implementation successfully handles lowercase characters and base symbols, the transition to a state-based driver for full modifier support is the next major milestone.
- **Extended CLI Toolset:** We are working to broaden the native command suite beyond basic file manipulation (e.g. `ls`, `cat`, `touch`) to include more complex Unix utilities.
- **Networking Stack:** In the long term, we aim to implement a network stack to enable basic socket communication and networking capabilities directly from the kernel.
- - **Proper Documentations** We aim to perfect the documentations to build the OS and also guide our new contributors about how to improve our OS.
## System Requirements
To successfully pass the GRUB bootloader and initialize the kernel, the following hardware is required:

- **Processor**: Intel 80386 (i386) or later (32-bit x86 architecture)
- **Memory (RAM)**: 3.6 MB
- **Keyboard**: PS/2 (Legacy) or any other keyboards that support Standard ANSI. Cmd/Caps Lock keys not yet mapped.
- **Video (Display)**: VGA-compatible video card supporting Mode 3 (80x25 Text Mode).
- **Storage**: None (RAM-Only Execution)