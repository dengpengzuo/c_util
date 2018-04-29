#include <stdio.h>
#include <stdarg.h>

#include "ez_string.h"

char *ez_strncpy(char *dest, char *src, size_t n) {
    if (n == 0) {
        return dest;
    }

    while (--n) {
        *dest = *src;

        if (*dest == '\0') {
            return dest;
        }

        dest++;
        src++;
    }

    *dest = '\0';

    return dest;
}

int ez_vsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
    int i = vsnprintf(buf, size, fmt, args);

    /*
     * The return value is the number of characters which would be written
     * into buf not including the trailing '\0'. If size is == 0 the
     * function returns 0.
     *
     * On error, the function also returns 0. This is to allow idiom such
     * as len += _vscnprintf(...)
     *
     * See: http://lwn.net/Articles/69419/
     */
    if (i <= 0) {
        return 0;
    }

    if (i < size) {
        return i;
    }

    return (int) (size - 1);
}

int ez_snprintf(char *buf, size_t size, const char *fmt, ...) {
    int i;
    va_list args;

    va_start(args, fmt);
    i = ez_vsnprintf(buf, size, fmt, args);
    va_end(args);

    return i;
}
