# User Development Guide - MYUNIXLIKEOS

## Quick Start: Writing and Compiling C Code

This guide shows you how to develop C programs inside MYUNIXLIKEOS using the built-in gcc compiler.

## Step-by-Step Example

### Step 1: Create a C Source File

Open the nano editor:
```bash
nano hello.c
```

### Step 2: Write Your Program

Type the following code:
```c
#include <stdio.h>

int main(int argc, char** argv) {
    printf("Hello from MYUNIXLIKEOS!\n");
    printf("This program was compiled inside the OS.\n");
    return 0;
}
```

Press `Ctrl+X` to save and exit.

### Step 3: Compile Your Program

```bash
gcc hello.c -o hello.elf
```

You'll see:
```
gcc: Compiling hello.c...
gcc: Found 1 symbols
gcc: Successfully compiled to hello.elf
```

### Step 4: Run Your Program

```bash
exec hello.elf
```

Output:
```
Hello from MYUNIXLIKEOS!
This program was compiled inside the OS.
```

## Available Header Files

### stdio.h
Basic I/O functions for user programs:

```c
#include <stdio.h>

int putchar(int c);           // Output a single character
int getchar(void);            // Read a single character
int printf(const char* fmt, ...); // Formatted output
```

### string.h
String and memory functions:

```c
#include <string.h>

size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);
char* strcpy(char* dest, const char* src);
void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
```

## Common Tasks

### Create a Math Program

File: `math.c`
```c
#include <stdio.h>

int square(int x) {
    return x * x;
}

int main() {
    for (int i = 1; i <= 5; i++) {
        printf("%d squared is %d\n", i, square(i));
    }
    return 0;
}
```

Commands:
```bash
gcc math.c -o math.elf
exec math.elf
```

### Process Command Line Arguments

File: `args.c`
```c
#include <stdio.h>

int main(int argc, char** argv) {
    printf("You passed %d arguments:\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("  arg[%d] = %s\n", i, argv[i]);
    }
    return 0;
}
```

Commands:
```bash
gcc args.c -o args.elf
exec args.elf hello world test
```

### Create a Simple Menu

File: `menu.c`
```c
#include <stdio.h>

int main() {
    printf("Simple Menu\n");
    printf("1. Option One\n");
    printf("2. Option Two\n");
    printf("3. Exit\n");
    printf("Enter your choice: ");
    
    int choice = getchar();
    printf("\nYou selected: %c\n", choice);
    return 0;
}
```

Commands:
```bash
gcc menu.c -o menu.elf
exec menu.elf
```

## File Management

### List Your Programs
```bash
ls
```

### View Source Code
```bash
cat hello.c
```

### Edit Existing Program
```bash
nano hello.c
gcc hello.c -o hello.elf
exec hello.elf
```

### Remove Program
```bash
rm hello.elf
```

## Tips

1. **Always include headers**: Use `#include <stdio.h>` for I/O functions

2. **Return zero for success**: Programs should return 0 on successful completion

3. **Check output carefully**: Compilation messages tell you if something went wrong

4. **Simple is better**: Start with simple programs and gradually add complexity

5. **Use descriptive names**: Use clear filenames for your programs

## Troubleshooting

### "gcc: [file]: No such file or directory"
- Make sure the file exists
- Check the spelling of the filename
- Use `ls` to see available files

### Program runs but shows no output
- Add `printf()` statements to verify execution
- Check that you're using correct function names

### Program compiles but doesn't run as expected
- Test individual functions with `printf()` calls
- Verify command-line arguments are correct

## Next Steps

1. Study example programs in the VFS
2. Experiment with different C features
3. Read the full GCC_USER_GUIDE.md for advanced information
4. Try combining multiple functions in one program

## Get Help

```bash
help gcc
```

This shows the compiler usage instructions directly in the OS.

---

**Remember**: The power of MYUNIXLIKEOS development framework is in your hands. Write, compile, and execute C programs without leaving the OS!
