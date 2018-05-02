#ifndef _EZ_STRING_H
#define _EZ_STRING_H

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>             /* offsetof() */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

char *ez_strncpy(char *dst, char *src, size_t n);

/** 格式化输出函数 snprintf, vsnprintf */
int ez_snprintf(char *buf, size_t size, const char *fmt, ...);

int ez_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

size_t ez_utf8_encode(uint8_t *dst, const wchar_t *src, size_t n);

size_t ez_utf8_decode(wchar_t *dst, const uint8_t *src, size_t n);

#endif /* _EZ_STRING_H */