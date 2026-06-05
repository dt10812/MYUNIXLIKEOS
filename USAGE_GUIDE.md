# MYUNIXLIKEOS Usage Guide

## 1. Getting Started

MYUNIXLIKEOS boots into a shell prompt on the VGA console. The shell supports built-in commands, file utilities, editor commands, system inspection, and execution of embedded user-mode programs.

To start the system, build the image and launch QEMU:

```bash
make clean && make
make run
```

The shell will display the welcome screen and then provide an interactive prompt.

## 2. Working with the Shell

The shell accepts commands, parses arguments, and dispatches them to built-in handlers or user programs. Command names are case-sensitive and match Unix-style usage.

### Common shell patterns

```bash
ls
pwd
cat filename.txt
mkdir newdir
touch newfile.txt
cp source.txt destination.txt
mv oldname.txt newname.txt
rm file.txt
```

### Getting help

```bash
help
help <command>
```

The `help` command lists available commands and usage information.

## 3. File Operations

The file system supports common interactive operations.

| Command | Purpose |
| --- | --- |
| `ls` | List files and directories |
| `mkdir DIR` | Create a directory |
| `cd DIR` | Change directory |
| `pwd` | Show current directory |
| `touch FILE` | Create an empty file |
| `cat FILE` | Display file contents |
| `cp SRC DST` | Copy a file |
| `mv SRC DST` | Rename or move a file |
| `rm FILE` | Remove a file |
| `clear` | Clear the console |

### Example workflow

```bash
pwd
mkdir demo
cd demo
touch notes.txt
cat notes.txt
cp notes.txt backup.txt
mv backup.txt saved.txt
rm saved.txt
cd ..
```

## 4. Systems and Diagnostics

The OS includes system inspection commands for memory, time, uptime, and process-like runtime information.

| Command | Purpose |
| --- | --- |
| `date` | Display current time |
| `free` | View memory usage |
| `memory` | Inspect memory allocator state |
| `top` | Show runtime status |
| `vmstat` | View virtual memory statistics |
| `uname` | Display OS identity |
| `whoami` | Display current identity |
| `uptime` | Show elapsed runtime |
| `sysinfo` | Show system information |

### Example

```bash
free
memory
top
vmstat
uname
uptime
date
```

## 5. Text Processing and Utilities

MYUNIXLIKEOS includes several text-oriented tools.

| Command | Purpose |
| --- | --- |
| `echo TEXT` | Print text |
| `grep PATTERN FILE` | Search file contents |
| `head FILE` | Show the beginning of a file |
| `tail FILE` | Show the end of a file |
| `wc FILE` | Count words, characters, and lines |
| `sort FILE` | Sort lines |
| `uniq FILE` | Remove duplicates |
| `color` | Adjust supported display colors |

### Example

```bash
echo "Hello from MYUNIXLIKEOS"
grep "error" logfile.txt
head config.txt
tail config.txt
wc notes.txt
sort names.txt
uniq list.txt
```

## 6. Editing Files

The operating system includes interactive text editors so files can be created, viewed, and modified from the shell.

### `nano`

Use `nano` to create or edit text files interactively.

```bash
nano example.txt
```

Within `nano`, use the editor shortcuts:

- `Ctrl+O` — save file
- `Ctrl+X` — exit
- `Ctrl+R` — reload file
- `Ctrl+W` — search
- `Ctrl+K` — cut line
- `Ctrl+U` — paste
- `Ctrl+A` / `Ctrl+E` — jump to line start/end
- `Ctrl+B` / `Ctrl+F` / `Ctrl+P` / `Ctrl+Z` — cursor movement helpers

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
