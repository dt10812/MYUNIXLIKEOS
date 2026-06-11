# MYUNIXLIKEOS

MYUNIXLIKEOS is a 32-bit Unix-like operating system kernel built from scratch for x86 hardware. It combines a protected-mode kernel, paging, a Virtual File System, an interactive shell, and ELF-based user program execution into a single educational platform.

## What the OS provides

- A freestanding monolithic kernel written in C and assembly.
- Multiboot2 booting through GRUB with ISO generation.
- Physical memory management, paging, protected-mode execution, and descriptor-table setup.
- A VFS with file caching and a shell-driven command environment.
- Built-in commands for file management, system inspection, text processing, editors, games, and development tasks.
- User-mode program execution using ELF binaries and an `exec` workflow.
- Kernel protections such as stack canaries and safe failure handling.

## Documentation

- [DETAILED_DESCRIPTION.md](DETAILED_DESCRIPTION.md) — full architecture, subsystem, feature, and capability overview.
- [USAGE_GUIDE.md](USAGE_GUIDE.md) — daily usage, command examples, editing, compiling, and runtime workflows.

## Highlights

- **Kernel architecture:** monolithic, low-level, x86 protected mode.
- **Memory:** physical page allocation, paging, virtual address setup, stack protection.
- **Filesystem:** VFS abstraction, file cache, directory/file handling, terminal-friendly utilities.
- **Shell:** interactive command parser, built-in utilities, editor integration, command help.
- **Execution:** ELF loading and user-mode program execution.
- **Development workflow:** create source files, compile with the built-in toolchain, run programs inside the OS.

## Quick start

Build and run the OS with QEMU:

```bash
make clean && make
make run
```

## Supported build environment

This project builds with a 32-bit x86 cross-toolchain. The Makefile auto-selects `i686-elf-gcc`, `clang`, or a compatible host compiler when available.

Required host tools include:

- `i686-elf-gcc` / `i686-elf-ld`
- `grub-mkrescue`
- `objcopy`
- `qemu-system-x86_64`

If using macOS, install a compatible `i686-elf` cross toolchain or use a native `clang`/`gcc` fallback.

## Supported runtime environment

- QEMU-based x86 testing
- VGA text console
- PS/2 keyboard input
- Serial diagnostics
- GRUB boot image generation

## Current focus

The project emphasizes practical kernel development, interactive shell workflow, filesystem support, and user-mode execution. The full architecture is described in [DETAILED_DESCRIPTION.md](DETAILED_DESCRIPTION.md), while the day-to-day command and execution workflow is covered in [USAGE_GUIDE.md](USAGE_GUIDE.md).
