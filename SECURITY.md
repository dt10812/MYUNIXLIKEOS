# Security

## Stack Canary Protection

MYUNIXLIKEOS now enables GCC stack protector support in the kernel build using `-fstack-protector-all`.
The kernel initializes the global guard value `__stack_chk_guard` during boot using entropy from the RTC and runtime kernel state.

If a protected function detects stack corruption, the kernel calls `__stack_chk_fail()`, which prints a security alert, disables interrupts, and halts the system to prevent further damage.

## How It Works

- Protected functions store the stack canary value on the stack when they enter.
- Before returning, the compiler-generated check compares the canary against the global `__stack_chk_guard`.
- A mismatch triggers `__stack_chk_fail()`.

## Verification

1. Rebuild the kernel:

```bash
make clean && make
```

2. Boot the OS with `make run`.
3. Confirm the boot log includes the stack canary initialization message.

## Notes

This implementation is an important defensive layer for the freestanding kernel.
Future work will continue to improve entropy sources and broader exploit mitigation features.
