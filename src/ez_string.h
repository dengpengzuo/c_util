#ifndef _EZ_STRING_H
#define _EZ_STRING_H

#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>

char *ez_strncpy(char *dest, char *src, size_t n);

/** 格式化输出函数 snprintf, vsnprintf */
int ez_snprintf(char *buf, size_t size, const char *fmt, ...);

int ez_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif /* _EZ_STRING_H */