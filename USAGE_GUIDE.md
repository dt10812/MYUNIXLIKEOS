# MYUNIXLIKEOS Usage Guide

## Getting Started

Build and run the OS:

```bash
make clean && make
make run
```

The shell prompts on the VGA console. Commands are case-sensitive and follow Unix convention.

## Common Tasks

### File Management
```bash
ls                    # List directory
pwd                   # Show current directory
cd <dir>              # Change directory
mkdir <dir>           # Create directory
touch <file>          # Create file
cat <file>            # Display file
cp <src> <dst>        # Copy file
mv <src> <dst>        # Move/rename
rm <file>             # Remove file
clear                 # Clear screen
```

### System Information
```bash
date                  # Show time
free                  # Show memory usage
uname                 # Show OS info
uptime                # Show runtime
sysinfo               # Show system details
top, vmstat, memory   # Diagnostics
```

### Text Processing
```bash
echo <text>           # Print text
grep <pattern> <file> # Search in file
head <file>           # Show start of file
tail <file>           # Show end of file
wc <file>             # Count lines/words
sort <file>           # Sort content
uniq <file>           # Remove duplicates
```

### Editing
```bash
nano <file>           # Edit with nano
```

**nano shortcuts:** `Ctrl+O` (save), `Ctrl+X` (exit), `Ctrl+W` (search), `Ctrl+K` (cut), `Ctrl+U` (paste)

### Help
```bash
help                  # List commands
help <command>        # Command help
```

### `vi`

`vi` is available as a text editor command for viewing and editing files.

```bash
vi file.txt
```

## 7. Compiling and Running Programs

MYUNIXLIKEOS supports compiling small C programs from inside the shell using the built-in compiler command. The resulting binaries are ELF files that can be executed with `exec`.

### Development workflow

1. Create a source file with `nano` or `vi`.
2. Compile the file into an ELF executable.
3. Run the resulting program with `exec`.

The shell includes a built-in `gcc` command that compiles small C programs into ELF binaries suitable for execution with `exec`. User-mode programs must be linked as 32-bit ELF executables to run in the OS.

Example:

```bash
nano hello.c
```

Write a simple program:

```c
#include <stdio.h>

int main(void) {
    printf("Hello from MYUNIXLIKEOS!\n");
    return 0;
}
```

Compile and run:

```bash
gcc hello.c -o hello.elf
exec hello.elf
```

The shell and kernel use the built-in execution path so programs can run in user mode.

### Example programs

The OS includes example programs such as `hello`, `simple`, and `echo_user`. They can be executed directly:

```bash
exec hello
exec simple
exec echo_user hello world
```

## 8. Entertainment and Extras

The OS includes a small game and display helpers.

### `snake`

Play the built-in snake game:

```bash
snake
```

Use `W`, `A`, `S`, and `D` to move, and `Q` to quit.

### Color helpers

The `color` utility allows changing console color support used by compatible shell utilities.

```bash
color
```

## 9. Common Tasks

### Create a file and display it

```bash
nano sample.txt
cat sample.txt
```

### Copy and rename files

```bash
cp source.txt copy.txt
mv copy.txt renamed.txt
```

### Search inside files

```bash
grep "pattern" document.txt
```

### Check memory usage

```bash
free
memory
vmstat
```

### Manage runtime information

```bash
date
uptime
uname
whoami
```

## 10. Troubleshooting

### Command not found

If a command is not recognized, confirm that it exists or use `help`:

```bash
help
help <command>
```

### File missing

Use `ls` to confirm that the file is present.

```bash
ls
pwd
```

### Program does not run

Check the executable name and invoke it with `exec`:

```bash
ls
exec hello.elf
```

### Editor save issues

Within `nano`, save using `Ctrl+O` and exit with `Ctrl+X`.

## 11. Development Notes

MYUNIXLIKEOS is designed so development can happen directly inside the operating system:

- create source files with `nano` or `vi`
- compile with `gcc`
- execute with `exec`
- inspect with `cat`, `grep`, `head`, `tail`, and `wc`

This workflow demonstrates kernel, filesystem, and user program execution working together.

## 12. Summary

MYUNIXLIKEOS provides an interactive Unix-style environment with built-in file management, system inspection, editor tools, user-mode execution, and a lightweight development workflow. The command set is designed to be practical for experimentation, kernel development, and small user-space programs.
