# MYUNIXLIKEOS — Detailed Operating System Description

## Overview

MYUNIXLIKEOS is a 32-bit Unix-like operating system for x86 hardware, built as a freestanding kernel with a shell-driven user interface and user program execution. The kernel manages low-level hardware, memory, filesystem services, and command execution, while user-space programs run as ELF binaries in ring-3 mode.

The project emphasizes practical kernel design: a monolithic architecture, direct hardware control, and a Unix-familiar command shell suitable for educational and development purposes.

## Hardware Platform

MYUNIXLIKEOS targets 32-bit x86 architecture (i386-class) using a cross-compiler toolchain. It runs under QEMU emulation or compatible PC hardware.

**Supported features:**
- Intel 80386-compatible CPU
- 32-bit protected mode, paging, and virtual memory
- VGA text console and PS/2 keyboard
- Multiboot2 boot protocol
- Page tables, descriptor tables, and interrupt gates

## Boot Sequence

The system boots through GRUB via Multiboot2, initializing memory management, paging, descriptor tables, and core subsystems before starting the shell. The boot flow establishes the kernel environment, sets up the filesystem, enables interrupts, and launches the interactive command prompt.

## Kernel Architecture

MYUNIXLIKEOS uses a monolithic kernel with core services for memory, files, hardware, and user execution.

**Key layers:**
- **Boot & CPU:** Protected mode, paging, descriptor tables, interrupts
- **Memory:** Physical allocator, virtual memory, kernel heap
- **Devices:** Keyboard, VGA console, RTC, serial I/O
- **Filesystem:** Virtual File System with caching and file operations
- **Execution:** ELF loading, user-mode transitions, shell command dispatch
- **Error Handling:** POSIX errno integration for system calls

The monolithic design runs device and filesystem services in kernel space for simplicity and performance.

## Memory Management

The kernel manages physical memory allocation, virtual address mapping through paging, kernel heap allocation, and stack protection. Memory is tracked from the boot memory map, with page directories and tables mapping kernel, user, and executable address spaces. Stack canaries protect against buffer overflows and kernel corruption.

## CPU Control and Interrupts

The kernel uses x86 protected-mode mechanisms: the Global Descriptor Table (GDT) defines kernel and user segments, the Interrupt Descriptor Table (IDT) manages hardware interrupts and exceptions, and the Task State Segment (TSS) handles privilege transitions. This enables ring switching, interrupt-driven input, and safe fault handling.

## Filesystem

The Virtual File System (VFS) provides directory hierarchy, file operations, and metadata handling. A file cache stores frequently used files in memory to improve performance. Shell commands interact with the VFS through kernel-managed read/write paths for a responsive interactive experience.

## Shell and User Interface

The shell is the primary user interface, reading keyboard input, parsing commands, dispatching built-in utilities, and executing user programs. It maintains an interactive prompt with command buffering, argument parsing, and output rendering through the VGA console.

## Built-in Commands

The OS provides utilities for file management (`ls`, `mkdir`, `cd`, `pwd`, `touch`, `cat`, `cp`, `mv`, `rm`), system diagnostics (`date`, `free`, `memory`, `top`, `vmstat`, `sysinfo`, `uname`, `whoami`, `uptime`), text processing (`echo`, `grep`, `head`, `tail`, `wc`, `sort`, `uniq`), editing (`nano`, `vi`), development (`gcc`, `exec`), and utilities like `help`, `clear`, and games.

## User Program Execution

The kernel loads and executes ELF32 binaries in user mode (ring-3). Programs are compiled, installed into the filesystem, and launched via the `exec` command. This demonstrates kernel-to-user-space separation and forms the basis for the development workflow.

## Input, Display, and Diagnostics

The system handles PS/2 keyboard input, displays output on VGA text console, and provides serial diagnostics. RTC supports time operations. Stack canaries protect against kernel corruption.

## Development Workflow

The OS supports an integrated development environment: create or edit files with `nano`/`vi`, compile with the built-in toolchain, execute with `exec`, and inspect results with standard utilities.

## Summary

MYUNIXLIKEOS is a complete educational Unix-like kernel combining low-level x86 hardware control with an interactive shell, filesystem abstraction, command utilities, and ELF user program execution. It demonstrates practical kernel programming with bootstrapping, memory management, protected-mode execution, interrupt handling, and a development workflow entirely within the OS.
