
#ifndef _EZ_UTIL_H_
#define _EZ_UTIL_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

int ez_atoi(uint8_t * line, size_t n);

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

int64_t ustime(void) ;

int64_t mstime(void) ;

void ez_localtime_r(const time_t * _time_t, struct tm *_tm);
#endif
