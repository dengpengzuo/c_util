
#ifndef _EZ_UTIL_H_
#define _EZ_UTIL_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define EZ_NOTUSED(V)	 ((void) V)

#define NELEMS(ARRAY)    ((sizeof(ARRAY)) / sizeof((ARRAY)[0]))

#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#define MAX(a, b)        ((a) > (b) ? (a) : (b))

#define ez_offsetof(TYPE, MEMBER)             ((uintptr_t)&(((TYPE *)0)->MEMBER))
#define ez_container_of(ptr, type, member)    ((type *)((uint8_t*)ptr - ez_offsetof(type, member)))
/*
 * Make data 'd' or pointer 'p', n-byte aligned, where n is a power of 2
 * of 2.
 */
#define EZ_ALIGNMENT        sizeof(unsigned long)	/* platform word */
#define EZ_ALIGN(d, n)      ((size_t)(((d) + (n - 1)) & ~(n - 1)))
#define EZ_ALIGN_PTR(p, n)  \
    (void *) (((uintptr_t) (p) + ((uintptr_t) n - 1)) & ~((uintptr_t) n - 1))

#define ez_atoi(_line, _n)          \
    _ez_atoi((uint8_t *)_line, (size_t)_n)

int _ez_atoi(uint8_t * line, size_t n);

/*
 * Wrappers around strtoull, strtoll, strtoul, strtol that are safer and
 * easier to use. Returns true if conversion succeeds.
 */
bool ez_strtoull_len(const char *str, uint64_t * out, size_t len);
bool ez_strtoull(const char *str, uint64_t * out);
bool ez_strtoll(const char *str, int64_t * out);
bool ez_strtoul(const char *str, uint32_t * out);
bool ez_strtol(const char *str, int32_t * out);
bool ez_str2oct(const char *str, int32_t * out);

/** 格式化输出函数 snprintf, vsnprintf */
#define ez_snprintf(_s, _n, ...)         snprintf((char *)_s, _n, __VA_ARGS__)
#define ez_vsnprintf(_s, _n, _f, _a)     vsnprintf((char*) _s, _n, _f, _a)

/** 格式化输出函数 _scnprintf, _vscnprintf */
#define ez_scnprintf(_s, _n, ...)       _scnprintf((char *)_s, _n, __VA_ARGS__)
#define ez_vscnprintf(_s, _n, _f, _a)   _vscnprintf((char*) _s, _n, _f, _a)

int _scnprintf(char *buf, size_t size, const char *fmt, ...);
int _vscnprintf(char *buf, size_t size, const char *fmt, va_list args);

int64_t ez_get_cur_milliseconds();

void ez_localtime_r(const time_t * _time_t, struct tm *_tm);
#endif
