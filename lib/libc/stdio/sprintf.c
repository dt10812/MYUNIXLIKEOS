#include "stdio.h"
#include "stdarg.h"
#include <stdbool.h>
#include <stddef.h>
#include "stdint.h"

static int _sprintf_c(char c, char **buf);
static int _sprintf_s(char *s, char **buf);
static int _sprintf_i(int i, char **buf);
static int _sprintf_u(unsigned int u, char **buf);
static int _sprintf_x(int x, char **buf);
static int _sprintf_li(long li, char **buf);
static int _sprintf_lu(unsigned long lu, char **buf);
static int _sprintf_lli(long long lli, char **buf);
static int _sprintf_llu(unsigned long long llu, char **buf);
static int _sprintf_hi(short hi, char **buf);
static int _sprintf_hhi(char hhi, char **buf);
static int _sprintf_zu(size_t zu, char **buf);
static int _sprintf_p(void *p, char **buf);

int vsprintf(char *restrict str, const char *restrict format, va_list args) {
    char *buf = str;
    bool l  = false;
    bool ll = false;
    bool h  = false;
    bool hh = false;
    bool L  = false;
    bool z  = false;

    const char *restrict fmtptr = format;
    for (; *fmtptr; fmtptr++) {
        if (*fmtptr == '%') {
            fmtptr++;
            l = ll = h = hh = L = z = false;

            switch (*fmtptr) {
            case 'l':
                fmtptr++;
                if (*fmtptr == 'l') { ll = true; fmtptr++; }
                else                  l  = true;
                break;
            case 'h':
                fmtptr++;
                if (*fmtptr == 'h') { hh = true; fmtptr++; }
                else                  h  = true;
                break;
            case 'L': L = true; fmtptr++; break;
            case 'z': z = true; fmtptr++; break;
            default: break;
            }

            switch (*fmtptr) {
            case 'i':
            case 'd':
                if      (ll) _sprintf_lli(va_arg(args, long long), &buf);
                else if (l)  _sprintf_li(va_arg(args, long), &buf);
                else if (hh) _sprintf_hhi((char)va_arg(args, int), &buf);
                else if (h)  _sprintf_hi((short)va_arg(args, int), &buf);
                else         _sprintf_i(va_arg(args, int), &buf);
                break;
            case 'u':
                if      (ll) _sprintf_llu(va_arg(args, unsigned long long), &buf);
                else if (l)  _sprintf_lu(va_arg(args, unsigned long), &buf);
                else if (z)  _sprintf_zu(va_arg(args, size_t), &buf);
                else         _sprintf_u(va_arg(args, unsigned int), &buf);
                break;
            case 's': _sprintf_s(va_arg(args, char *), &buf);  break;
            case 'c': _sprintf_c((char)va_arg(args, int), &buf); break;
            case 'x':
            case 'X': _sprintf_x(va_arg(args, int), &buf);    break;
            case 'p': _sprintf_p(va_arg(args, void *), &buf); break;
            case 'n': break; /* not implemented */
            case '%': _sprintf_c('%', &buf); break;
            default:  break;
            }
            continue;
        }

        _sprintf_c(*fmtptr, &buf);
    }

    *buf = '\0';  /* null-terminate the string */
    return (int)(buf - str);
}

int sprintf(char *restrict str, const char *restrict format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsprintf(str, format, args);
    va_end(args);
    return ret;
}

static int _sprintf_c(char c, char **buf) {
    **buf = (unsigned char)c;
    (*buf)++;
    return 1;
}

static int _sprintf_s(char *s, char **buf) {
    int n = 0;
    for (char *c = s; *c; c++)
        n += _sprintf_c(*c, buf);
    return n;
}

static int _sprintf_i(int i, char **buf) {
    if (i < 0) { _sprintf_c('-', buf); i = -i; }
    if (i >= 10) _sprintf_i(i / 10, buf);
    _sprintf_c('0' + (i % 10), buf);
    return 0;
}

static int _sprintf_u(unsigned int u, char **buf) {
    if (u >= 10) _sprintf_u(u / 10, buf);
    _sprintf_c('0' + (u % 10), buf);
    return 0;
}

static int _sprintf_li(long li, char **buf) {
    if (li < 0) { _sprintf_c('-', buf); li = -li; }
    if (li >= 10) _sprintf_li(li / 10, buf);
    _sprintf_c('0' + (li % 10), buf);
    return 0;
}

static int _sprintf_lu(unsigned long lu, char **buf) {
    if (lu >= 10) _sprintf_lu(lu / 10, buf);
    _sprintf_c('0' + (lu % 10), buf);
    return 0;
}

static int _sprintf_lli(long long lli, char **buf) {
    if (lli < 0) { _sprintf_c('-', buf); lli = -lli; }
    if (lli >= 10) _sprintf_lli(lli / 10, buf);
    _sprintf_c('0' + (lli % 10), buf);
    return 0;
}

static int _sprintf_llu(unsigned long long llu, char **buf) {
    if (llu >= 10) _sprintf_llu(llu / 10, buf);
    _sprintf_c('0' + (llu % 10), buf);
    return 0;
}

static int _sprintf_hi(short hi, char **buf)   { return _sprintf_i((int)hi, buf); }
static int _sprintf_hhi(char hhi, char **buf)  { return _sprintf_i((int)hhi, buf); }
static int _sprintf_zu(size_t zu, char **buf)  { return _sprintf_llu((unsigned long long)zu, buf); }

static int _sprintf_x(int x, char **buf) {
    static const char hex[] = "0123456789abcdef";
    if (x >= 16) _sprintf_x(x / 16, buf);
    _sprintf_c(hex[x % 16], buf);
    return 0;
}

static int _sprintf_p(void *p, char **buf) {
    _sprintf_s("0x", buf);
    return _sprintf_x((int)(long)p, buf);
}
