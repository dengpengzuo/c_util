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

/*-----------------------+-----------------------------------------------------------------------------
    0000 0000->0000 007F | 0xxxxxxx                             // 7  位有效位.
    0000 0080->0000 07FF | 110xxxxx 10xxxxxx                    // 11 位有效位.
    0000 0800->0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx           // 16 位有效位.
      + 0800->D7FF
      + E000->FFFF
    0001 0000->0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  // 21 位有效位.
  -----------------------+----------------------------------------------------------------------------- */
size_t ez_utf8_encode(uint8_t *dst, const wchar_t *src, size_t n) {
    size_t c = 0;
    for (size_t i = 0; i < n; ++i) {
        if (src[i] <= 0x7f) {
            dst[c++] = (uint8_t) src[i++] ;
        } else if (src[i] >= 0x80 && src[i] <= 0x07FF) {
            dst[c++] = (uint8_t) (0xC0 | ((src[i] >> 6) & 0x1F)); // 高5位 -> 1_1111
            dst[c++] = (uint8_t) (0x80 | (src[i] & 0x3F));        // 低6位 -> 11_1111
        } else if (src[i] >= 0x800 && src[i] <= 0xFFFF) {
            dst[c++] = (uint8_t) (0xE0 | ((src[i] >> 12) & 0xF)); // 高4位 -> 1111
            dst[c++] = (uint8_t) (0x80 | ((src[i] >> 6) & 0x3F)); // 6位
            dst[c++] = (uint8_t) (0x80 | (src[i] & 0x3F));        // 6位
        } else if (src[i] >= 0x10000 && src[i] <= 0x10FFFF) {
            dst[c++] = (uint8_t) (0xF0 | ((src[i] >> 18) & 0x7)); // 高3位 -> 111
            dst[c++] = (uint8_t) (0x80 | ((src[i] >> 12) & 0x3F));// 6位
            dst[c++] = (uint8_t) (0x80 | ((src[i] >> 6) & 0x3F)); // 6位
            dst[c++] = (uint8_t) (0x80 | (src[i] & 0x3F));        // 6位
        } else {
            // skip
        }
    }
    return c;
}

// 取从最高位起,有几位1.
static inline uint32_t left_bits(uint8_t x) {
    // 2, 3, 4
    if ((x & 0xE0) == 0xc0) {
        return 2;
    } else if ((x & 0xF0) == 0xE0) {
        return 3;
    } else if ((x & 0xF8) == 0xF0) {
        return 4;
    } else {
        return 0;
    }
}

size_t ez_utf8_decode(wchar_t *dst, const uint8_t *src, size_t n) {
    size_t c = 0;
    for (size_t i = 0; i < n;) {
        if (src[i] <= 0x7f) {
            dst[c++] = (wchar_t) src[i++];
        } else {
            uint32_t bit = left_bits(src[i]);
            if (bit == 2) {
                dst[c] = (wchar_t) (src[i++] & 0x1F);
                dst[c] = (dst[c] << 5) | (wchar_t) (src[i++] & 0x3F);
                c++;
            } else if (bit == 3) {
                dst[c] = (wchar_t) (src[i++] & 0xF);
                dst[c] = (dst[c] << 6) | (wchar_t) (src[i++] & 0x3F);
                dst[c] = (dst[c] << 6) | (wchar_t) (src[i++] & 0x3F);
                c++;
            } else if (bit == 4) {
                dst[c] = (wchar_t) (src[i++] & 0x7);
                dst[c] = (dst[c] << 6) | (wchar_t) (src[i++] & 0x3F);
                dst[c] = (dst[c] << 6) | (wchar_t) (src[i++] & 0x3F);
                dst[c] = (dst[c] << 6) | (wchar_t) (src[i++] & 0x3F);
                c++;
            } else {
                // skip
                ++i;
            }
        }
    }
    return c;
}