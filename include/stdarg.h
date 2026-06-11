/* Only supports IA-32, x86_64 passes args in registers, not the stack. */

#ifndef _STDARG_H_
#define _STDARG_H_

typedef __builtin_va_list va_list;

#define va_start(ap, argN) __builtin_va_start(ap, argN)
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

#endif /* _STDARG_H_ */