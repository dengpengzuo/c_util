#include "ez_string.h"

char *ez_strncpy(char *dst, char *src, size_t n) {
    if (n == 0) {
        return dst;
    }

    while (--n) {
        *dst = *src;

        if (*dst == '\0') {
            return dst;
        }

        dst++;
        src++;
    }

    *dst = '\0';

    return dst;
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

/*
    0000 0000->0000 007F | 0xxxxxxx                             // 7  位有效位.
    0000 0080->0000 07FF | 110xxxxx 10xxxxxx                    // 11 位有效位.
    0000 0800->0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx           // 16 位有效位.
    0001 0000->0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  // 21 位有效位.  TODO:?? 按理是 0x1f_FFFF
    ---------------------+-----------------------------------------------------------------------------
    0020 0000->03ff ffff | 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx // 26 位有效位.
    0400 0000->7fff ffff | 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx // 31 位有效位.
    ---------------------+-----------------------------------------------------------------------------
*/
size_t ez_utf8_encode(u_int8_t *dst, const u_char *src, size_t n) {
    size_t c = 0;
    for (size_t i = 0; i < n; ++i) {
        if (src[i] <= 0x7f) {
            dst[c++] = src[i++];
        } else if (src[i] >= 0x80 && src[i] <= 0x07FF) {
            dst[c++] = (u_int8_t) (0xC0 + ((src[i] >> 6) & 0x1F)); // 高5位 -> 1_1111
            dst[c++] = (u_int8_t) (0x80 + (src[i] & 0x3F));         // 低6位 -> 11_1111
        } else if (src[i] >= 0x800 && src[i] <= 0xFFFF) {
            dst[c++] = (u_int8_t) (0xE0 + ((src[i] >> 12) & 0xF)); // 高4位 -> 1111
            dst[c++] = (u_int8_t) (0x80 + ((src[i] >> 6) & 0x3F));  // 6位
            dst[c++] = (u_int8_t) (0x80 + (src[i] & 0x3F));         // 6位
        } else if (src[i] >= 0x10000 && src[i] <= 0x10FFFF) {
            dst[c++] = (u_int8_t) (0xF0 + ((src[i] >> 18) & 0x7)); // 高3位 -> 111
            dst[c++] = (u_int8_t) (0x80 + ((src[i] >> 12) & 0x3F));// 6位
            dst[c++] = (u_int8_t) (0x80 + ((src[i] >> 6) & 0x3F)); // 6位
            dst[c++] = (u_int8_t) (0x80 + (src[i] & 0x3F));        // 6位
        } else {
            // skip
        }
    }
    return c;
}

// 取从最高位起,有几位1.
static inline int left_bits(u_int8_t x) {
    // 2, 3, 4
    int bit = 0;
    while ((x & 0x80) == 0x80) {
        x = x << 1;
        ++bit;
    }
    return bit;
}

size_t ez_utf8_decode(u_char *dst, const u_int8_t *src, size_t n) {
    size_t c = 0;
    for (size_t i = 0; i < n; ++i) {
        if (src[i] <= 0x7f) {
            dst[c++] = (u_char) src[i++];
        } else {
            int bit = left_bits(src[i]);
            if (bit == 2) {

            } else if (bit == 3) {

            } else if (bit == 4) {

            } else {
                // skip
            }
        }
    }
    return c;
}