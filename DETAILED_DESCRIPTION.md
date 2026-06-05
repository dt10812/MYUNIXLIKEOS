# MYUNIXLIKEOS — Detailed Operating System Description

## 1. Project Overview

MYUNIXLIKEOS is a 32-bit Unix-like operating system implemented from scratch for x86 hardware. The project is built as a freestanding kernel, with a custom runtime environment, a shell-driven user interface, and an embedded user program execution model. It is designed as an educational and development-focused operating system: the kernel handles low-level hardware setup, memory management, filesystem services, and command execution, while user-space programs run as ELF binaries under a ring-3 execution model.

The operating system emphasizes a practical monolithic kernel structure, direct hardware control, and a command shell that feels familiar to Unix users while remaining lightweight enough to fit within an instructional kernel project.

## 2. Goals and Design Philosophy

The main goals of MYUNIXLIKEOS are:

- Build a working 32-bit kernel without relying on external operating system services.
- Provide a minimal but functional Unix-like user environment.
- Support interactive shell usage, file operations, process execution, and system utilities.
- Demonstrate kernel concepts such as bootstrapping, paging, file system abstraction, interrupt handling, and user-mode execution.
- Offer a development workflow where source files can be edited and programs can be compiled and executed from within the OS.

This design favors clarity, direct hardware integration, and incremental growth over a large enterprise-style kernel architecture.

## 3. Hardware and Platform Target

MYUNIXLIKEOS targets the Intel 32-bit x86 platform (i386-class) and is built using a cross-compiler toolchain. The runtime environment is intended for emulation under QEMU, but the kernel is structured to run on compatible PC hardware.

### Supported hardware assumptions

- Intel 80386-compatible CPU or later
- 32-bit protected mode execution
- VGA text mode display support
- PS/2 keyboard interface
- Multiboot2-compatible boot environment
- RAM-backed execution model for the ISO boot image

The kernel uses classic protected mode mechanisms, page tables, descriptor tables, and interrupt gates to manage the CPU state.

## 4. Boot and Early Initialization

The system boots through GRUB using a Multiboot2-compatible image. The kernel image is embedded in an ISO and launched by the bootloader. During startup, the kernel initializes its early execution environment before handing control to the shell.

### Boot sequence

1. GRUB loads the kernel image and passes the Multiboot2 information block.
2. The kernel entry point establishes the initial stack and low-level execution environment.
3. The system validates the Multiboot information, parses memory maps, and checks boot metadata.
4. Physical memory management is initialized.
5. Paging structures are created and enabled.
6. Core subsystems are brought up: filesystem, GDT, IDT, keyboard, time, and user program installation.
7. The shell is started and the interactive prompt becomes available.

This boot path gives the kernel a deterministic startup sequence and exposes a clear place for hardware bring-up and subsystem initialization.

## 5. Kernel Architecture

MYUNIXLIKEOS uses a monolithic kernel architecture. The kernel contains the core services required to manage memory, files, hardware, and user process execution. The structure is organized into kernel-level source files, shared headers, user-space helper libraries, and command modules.

### Major kernel layers

- **Boot and architecture layer:** early entry, paging, descriptor tables, interrupt setup.
- **Memory layer:** physical memory allocator, paging support, kernel heap and stack management.
- **Device layer:** keyboard, VGA, RTC, serial, and input handling.
- **Filesystem layer:** virtual filesystem, generic file caching, file operations, user-visible storage abstractions.
- **Execution layer:** ELF loading, user-mode transitions, command dispatch, shell runtime.
- **Utility layer:** time, string, console, and kernel logging helpers.

Because the kernel is monolithic, device and filesystem services run inside kernel space rather than through user-space daemons.

## 6. Memory Management

Memory management is a core subsystem of MYUNIXLIKEOS. The kernel allocates physical memory, sets up paging, and supports user-mode execution through mapped pages and protected contexts.

### Physical memory management

The kernel tracks available physical pages from the boot memory map. This allocator is responsible for reserving kernel memory, tracking the bitmap of used pages, and handing out page frames for kernel and user mappings.

### Paging and virtual memory

Paging support provides virtual addressing, page-table creation, and address translation support. The kernel uses page directories and page tables to map kernel memory, user memory, and executable images. This is essential both for stability and for transitioning into user mode.

### Heap and allocation

The kernel includes allocation support for dynamic memory structures used by filesystem code, command execution, line buffers, cache management, and runtime objects. Memory allocation is designed around predictable kernel-side allocation rather than a full user-space malloc implementation.

### Stack protections

The kernel enables stack canaries during builds to detect stack corruption. The stack protector is initialized during boot and can halt the kernel safely if corruption is detected.

## 7. Interrupts, Descriptor Tables, and CPU State

The system uses the standard x86 protected-mode mechanisms for CPU control:

- Global Descriptor Table (GDT)
- Interrupt Descriptor Table (IDT)
- Task State Segment (TSS)
- Keyboard and exception handling

The GDT defines kernel and user segments, while the IDT manages hardware interrupts and CPU exceptions. The TSS is used for privilege domain transitions and stack switching. Fault handling is part of the kernel's debugging and robustness strategy.

This layer is what makes ring transitions, interrupt-driven input, and exception handling possible.

## 8. Filesystem Layer

The OS exposes a Virtual File System (VFS) abstraction on top of kernel-managed file operations. The design supports directory hierarchy, file creation, reads, writes, metadata, file caching, and command-level file interactions.

### Filesystem features

- Directory and file abstraction through vnode-style objects
- File cache support for repeated access patterns
- File metadata handling
- Kernel-managed read/write paths
- Command-line file utilities for listing, creating, copying, moving, deleting, and viewing files

### File cache

A file cache stores frequently used files in memory, tracks usage statistics, and improves repeated access. This is especially helpful for interactive shell usage where commands and small files are read repeatedly.

### Storage assumptions

The current filesystem design is kernel-centric and suitable for embedded-style file handling. The user-visible shell commands operate over the VFS abstraction and the available in-memory filesystem structure.

## 9. Command Shell and User Interface

The shell is the primary interface to the operating system. It provides prompt rendering, character input, command parsing, built-in command dispatch, and the execution of user programs. The shell is designed for interactive work instead of background daemons.

### Shell behavior

- Reads keyboard input character by character
- Maintains a command line buffer
- Splits input into command and arguments
- Dispatches built-in commands to kernel code
- Launches user programs via the exec path
- Displays status and command output through the terminal console

The shell provides the operating system's primary user-facing workflow, including editing, file manipulation, system inspection, and command execution.

## 10. Built-in Commands and Utilities

MYUNIXLIKEOS ships with a broad command set for interactive use. The commands are implemented both as kernel-resident utilities and as user-space applications embedded in the build.

### File management commands

- `ls` — list directory contents
- `mkdir` — create directories
- `cd` — change current working directory
- `pwd` — print current working directory
- `touch` — create empty files
- `cat` — display file contents
- `cp` — copy files
- `mv` — move or rename files
- `rm` — remove files
- `clear` — clear the terminal screen

### System information commands

- `date` — display current time
- `free` — show memory usage
- `memory` — inspect memory allocation state
- `top` — show runtime system information
- `vmstat` — display virtual memory statistics
- `sysinfo` — show system information
- `uname` — display OS identity
- `whoami` — show current user identity
- `uptime` — show elapsed uptime

### Text and utility commands

- `echo` — print text to console
- `grep` — search text within files
- `head` — show beginning of file content
- `tail` — show end of file content
- `wc` — count words, characters, and lines
- `uniq` — remove duplicate lines
- `sort` — sort file contents
- `sh` — invoke subshell behavior where appropriate
- `help` — display command help

### Editor commands

- `nano` — interactive editor for creating and editing files
- `vi` — text editor command

### Development and utility commands

- `gcc` — compile source files into executable artifacts within the OS
- `exec` — run ELF binaries
- `hello` — example program
- `simple` — example program
- `color` — change terminal color settings in supported display utilities

### Entertainment and extras

- `snake` — interactive snake game

These commands give the OS a complete interactive environment covering file editing, system inspection, text processing, entertainment, and development.

## 11. User Programs and ELF Execution

MYUNIXLIKEOS supports loading and executing ELF32 user programs. The kernel embeds user binaries in the build, installs them into the filesystem, and executes them through the `exec` command. A ring-3 user-space program can print output, process arguments, and exit normally.

### Execution workflow

1. A user program is compiled into an ELF executable.
2. The executable is installed into the kernel-managed program store.
3. The shell invokes `exec` with the program path or name.
4. The kernel loads the ELF image into the user address space.
5. The CPU is switched into user mode at the ELF entry point.
6. Program output is displayed through the terminal console.

This workflow demonstrates the separation between kernel code and user mode execution and forms the basis for more advanced process management.

### Example programs

- `hello` — simple greeting demo
- `simple` — small arithmetic and output demo
- `echo_user` — argument handling demo
- `gcc`-compiled programs — user programs built from within the OS

## 12. Input Handling

Keyboard input is handled through a PS/2 driver. The driver supports character input, modifier keys, backspace, tab, enter, and editor-oriented control sequences. The shell and editor rely on this input layer for interactive use.

The OS is designed around terminal-style interaction, so keyboard handling is central to the user experience.

## 13. Graphics and Console

The system uses VGA-compatible text mode for its main display. The terminal path uses text rendering for the shell prompt, command output, and editor status. Console output remains stable and predictable, with low-level serial output available for diagnostics.

The console uses the VGA text buffer for visible user interaction, while serial I/O supports debugging and low-level runtime traces.

## 14. Time, RTC, and System Services

The kernel includes RTC and time handling so the OS can report the current time, show uptime, and use time data during initialization. The time layer supports conversion, timekeeping, and system utility output.

## 15. Security and Reliability

MYUNIXLIKEOS includes several reliability measures:

- Stack canary protection in kernel builds.
- Protected-mode execution and privilege separation.
- User-space execution through mapped pages instead of direct kernel execution.
- Controlled execution entry points for user programs.
- Safe failure behavior for kernel corruption conditions.

Because it is an instructional kernel, security features are foundational but not yet equivalent to a mature production OS.

## 16. Development Workflow

One of the strengths of MYUNIXLIKEOS is that it supports development directly inside the OS environment. Users can create source files, compile them, run them, inspect them, and iterate without leaving the shell.

Typical workflow:

1. Create or edit a file using `nano` or `vi`.
2. Compile the source into an ELF executable using the built-in compiler.
3. Execute the program with `exec`.
4. Inspect the result with `cat`, `grep`, `ls`, or memory utilities.

The project demonstrates a complete kernel-to-user-space development cycle inside the operating system itself.

## 17. Build and Runtime Environment

The project is built with a Makefile and runs through QEMU. The build process compiles the kernel, user libraries, built-in user binaries, and the ISO image required for booting. The runtime uses serial output for diagnostics and VGA display for the interactive console.

This makes MYUNIXLIKEOS practical for local development and live testing on macOS or other Unix-like hosts with the appropriate cross-compiler toolchain.

## 18. Current Capabilities and Limitations

### Strong capabilities

- Protected-mode kernel boot
- Physical memory and paging support
- Virtual filesystem and file cache
- Interactive shell with command parsing
- Built-in file, system, editor, and utility commands
- User-mode ELF execution
- Built-in development workflow for compiling and executing programs
- Console, keyboard, RTC, and serial support

### Current limitations

- The environment is still a teaching-focused kernel rather than a mature enterprise OS.
- Advanced process scheduling and full preemptive multitasking are not the defining features of the current release.
- Some command implementations are intentionally lightweight and educational.
- Graphics support is centered on VGA text console behavior rather than full framebuffer graphics.

## 19. Summary

MYUNIXLIKEOS is a practical, hands-on Unix-like kernel that combines low-level x86 hardware control with an interactive shell, a filesystem abstraction, a command suite, and ELF-based user program execution. It is designed as a complete learning environment and development platform for kernel programming, operating system internals, and user-mode application execution.

Its strongest identity is as a compact but capable educational kernel: bootable, interactive, memory-managed, filesystem-aware, and able to execute user programs from inside the OS.
