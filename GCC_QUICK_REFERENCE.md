# GCC Quick Reference Card

## Basic Usage

```bash
gcc source.c                  # Compile to a.elf
gcc source.c -o prog.elf      # Compile to prog.elf
exec prog.elf                 # Run the program
```

## Common Programs

### Hello World
```c
#include <stdio.h>
int main() {
    printf("Hello!\n");
    return 0;
}
```

### Print Numbers
```c
#include <stdio.h>
int main() {
    for (int i = 1; i <= 10; i++)
        printf("%d\n", i);
    return 0;
}
```

### Add Numbers
```c
#include <stdio.h>
int add(int a, int b) { return a + b; }
int main() {
    printf("5 + 3 = %d\n", add(5, 3));
    return 0;
}
```

### Use Arguments
```c
#include <stdio.h>
int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++)
        printf("%s\n", argv[i]);
    return 0;
}
```

### String Operations
```c
#include <stdio.h>
#include <string.h>
int main() {
    char str[] = "hello";
    printf("Length: %d\n", strlen(str));
    return 0;
}
```

## Available Functions

### stdio.h
- `printf(const char* fmt, ...)` - Print formatted output
- `putchar(int c)` - Print single character
- `getchar()` - Read single character

### string.h
- `strlen(const char* s)` - String length
- `strcmp(const char* s1, s2)` - Compare strings
- `strcpy(char* dest, src)` - Copy string
- `memset(void* p, int c, n)` - Fill memory
- `memcpy(void* d, src, n)` - Copy memory

## File Management

```bash
ls              # List files (includes .elf files)
cat prog.c      # View source code
rm prog.elf     # Delete compiled program
nano prog.c     # Edit program
```

## Compile & Run Cycle

1. `nano myprogram.c`  → Write code
2. `gcc myprogram.c`   → Compile
3. `exec a.elf`        → Test
4. Repeat as needed

## Tips

- Use descriptive filenames
- Always `#include <stdio.h>` for printf
- Return 0 on success
- Use `-o` to control output filename
- Check `ls` after compiling to verify .elf created

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "No such file" | Check filename spelling, use `ls` to list files |
| "Parse error" | Check C syntax, use nano to edit |
| Program hangs | Use Ctrl+C, add `printf()` for debugging |
| No output | Add `printf()` statements to verify execution |

## Common Patterns

### Loop and Print
```c
for (int i = start; i < end; i++)
    printf("Value: %d\n", i);
```

### If/Else Logic
```c
if (condition)
    printf("True\n");
else
    printf("False\n");
```

### Function Definition
```c
int myfunction(int x) {
    return x * 2;
}
```

### Main with Args
```c
int main(int argc, char** argv) {
    // argc = count of arguments
    // argv = array of strings
    return 0;
}
```

## Get Help

```bash
help gcc
```

Shows detailed gcc usage information within OS.

---

**Remember**: Start simple, test often, use printf() for debugging!
