/* gcc.md - Simple C Compiler Documentation */

# GCC - Simple C Compiler for MYUNIXLIKEOS

## Overview

The `gcc` command allows users to write and compile C code directly within the OS. Compiled programs are stored as ELF executables in the virtual file system and can be executed using the `exec` command.

## Usage

```
gcc <source.c> [-o output.elf]
```

### Parameters

- `<source.c>`: Input C source file (must exist in VFS)
- `-o output.elf`: (Optional) Output filename (default: a.elf)

## Workflow

### 1. Write C Code

Use the built-in text editors (nano or vi) to create C source files:

```bash
nano myprogram.c
```

### 2. Write Your Program

Example program:

```c
#include <stdio.h>

int main(int argc, char** argv) {
    printf("Hello from compiled C code!\n");
    return 0;
}
```

### 3. Compile

```bash
gcc myprogram.c -o myprogram.elf
```

### 4. Execute

```bash
exec myprogram.elf
```

## Supported C Features

### Data Types
- `int`
- `char`
- `void`
- Basic pointers

### Functions
- Function declarations and definitions
- Function calls
- Return values
- Arguments/parameters

### Standard Library Functions
- `printf()` - formatted output
- `putchar()` - character output
- `getchar()` - character input
- `strlen()` - string length
- `strcmp()` - string comparison
- `strcpy()` - string copy
- `memset()` - memory fill
- `memcpy()` - memory copy

### Control Flow
- `if / else` statements
- `for` loops
- `while` loops
- `return` statements

### Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `&&`, `||`, `!`
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`

## Examples

### Simple Addition

Create `add.c`:
```c
#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int main(int argc, char** argv) {
    int result = add(5, 3);
    printf("5 + 3 = %d\n", result);
    return 0;
}
```

Compile and run:
```bash
gcc add.c -o add.elf
exec add.elf
```

Output:
```
5 + 3 = 8
```

### Command-line Echo

Create `echo_prog.c`:
```c
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("\n");
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
    return 0;
}
```

Compile:
```bash
gcc echo_prog.c -o echo_prog.elf
exec echo_prog.elf Hello World
```

Output:
```
Hello World
```

### Loop and Array

Create `loop.c`:
```c
#include <stdio.h>

int main() {
    for (int i = 1; i <= 5; i++) {
        printf("Count: %d\n", i);
    }
    return 0;
}
```

Compile:
```bash
gcc loop.c -o loop.elf
exec loop.elf
```

## Limitations

### Not Supported (Current Version)

- Structs and unions
- Floating point numbers
- File I/O operations
- Dynamic memory allocation (malloc/free)
- Inline assembly
- Complex pointer operations
- Recursive compilation

## Tips and Tricks

1. **Default Output Name**: If you don't specify `-o`, the output is `a.elf`

2. **List Compiled Programs**: Use `ls` to see what you've compiled:
   ```bash
   ls
   ```

3. **Remove Programs**: Delete unwanted executables:
   ```bash
   rm myprogram.elf
   ```

4. **Read Source Code**: View compiled source:
   ```bash
   cat add.c
   ```

5. **Edit Existing Programs**: Modify and recompile:
   ```bash
   nano add.c
   gcc add.c -o add.elf
   ```

## Error Messages

- `gcc: [file]: No such file or directory` - Source file doesn't exist
- `gcc: Parse error` - Syntax error in C code
- `gcc: Failed to generate output file` - Compilation failed

## See Also

- `nano` - Text editor for creating C files
- `vi` - Alternative text editor
- `exec` - Execute compiled programs
- `cat` - View source code
- `ls` - List files

## Future Enhancements

Planned features:
- Full C99 support
- Optimization flags (-O2, -O3)
- Debug symbols (-g)
- Linking multiple object files
- Static/dynamic linking options
- Preprocessor directives (#include, #define)
