
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "ez_malloc.h"
#include "ez_log.h"
#include "ez_util.h"

int64_t ustime(void) {
    struct timeval tv;
    int64_t ust;

    gettimeofday(&tv, NULL);
    ust = ((int64_t) tv.tv_sec) * 1000000;
    ust += tv.tv_usec;
    return ust;
}

int64_t mstime(void) { return ustime() / 1000; }

void ez_localtime_r(const time_t *_time_t, struct tm *_tm) {
#if defined(__linux__) || defined(linux)
    localtime_r(_time_t, _tm);
#elif defined(__MINGW__) || defined(__MINGW32__)
    localtime_s(_tm, _time_t);
#else
#error "localtime_r not defined."
#endif
}

int ez_atoi(uint8_t *line, size_t n) {
    int value;

    if (n == 0) {
        return -1;
    }

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return -1;
        }

        value = value * 10 + (*line - '0');
    }

    if (value < 0) {
        return -1;
    }

    return value;
}

static void ez_skip_space(const char **str, size_t *len) {
    while (*len > 0 && isspace(**str)) {
        (*str)++;
        (*len)--;
    }
}

bool ez_strtoull_len(const char *str, uint64_t *out, size_t len) {
    *out = 0ULL;

    ez_skip_space(&str, &len);

    while (len > 0 && (*str) >= '0' && (*str) <= '9') {
        if (*out >= UINT64_MAX / 10) {
            /*
             * At this point the integer is considered out of range,
             * by doing so we convert integers up to (UINT64_MAX - 6)
             */
            return false;
        }
        *out = *out * 10 + *str - '0';
        str++;
        len--;
    }

    ez_skip_space(&str, &len);

    if (len == 0) {
        return true;
    } else {
        return false;
    }
}

bool ez_strtoull(const char *str, uint64_t *out) {
    char *endptr;
    unsigned long long ull;

    errno = 0;
    *out = 0ULL;

    ull = strtoull(str, &endptr, 10);

    if (errno == ERANGE) {
        return false;
    }

    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        if ((long long) ull < 0) {
            /*
             * Only check for negative signs in the uncommon case when
             * the unsigned number is so big that it's negative is a
             * signed number
             */
            if (strchr(str, '-') != NULL) {
                return false;
            }
        }

        *out = ull;

        return true;
    }

    return false;
}

bool ez_strtoll(const char *str, int64_t *out) {
    char *endptr;
    long long ll;

    errno = 0;
    *out = 0LL;

    ll = strtoll(str, &endptr, 10);

    if (errno == ERANGE) {
        return false;
    }

    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = ll;
        return true;
    }

    return false;
}

bool ez_strtoul(const char *str, uint32_t *out) {
    char *endptr;
    unsigned long l;

    errno = 0;
    *out = 0UL;

    l = strtoul(str, &endptr, 10);

    if (errno == ERANGE) {
        return false;
    }

    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        if ((long) l < 0) {
            /*
             * Only check for negative signs in the uncommon case when
             * the unsigned number is so big that it's negative as a
             * signed number
             */
            if (strchr(str, '-') != NULL) {
                return false;
            }
        }

        *out = l;

        return true;
    }

    return false;
}

bool ez_strtol(const char *str, int32_t *out) {
    char *endptr;
    long l;

    *out = 0L;
    errno = 0;

    l = strtol(str, &endptr, 10);

    if (errno == ERANGE) {
        return false;
    }

    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = l;
        return true;
    }

    return false;
}

bool ez_str2oct(const char *str, int32_t *out) {
    char *endptr;
    long l;

    *out = 0L;
    errno = 0;

    l = strtol(str, &endptr, 8);

    if (errno == ERANGE) {
        return false;
    }

    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = l;
        return true;
    }

    return false;
}

ssize_t ez_read_file(const char *file_name, uint8_t *buf, size_t len) {
    ssize_t size = 0;
    int fd = open(file_name, O_RDONLY, 0644);
    if (fd > 0) {
        size = read(fd, buf, len);
        if (size == 0) {
            log_warn("read file %s 0 bits.", file_name);
        } else if (size < 0) {
            log_error("read file %s error:[%s]", file_name, strerror(errno));
            size = 0;
        }
        // 打开成功后,关闭文件句柄.
        close(fd);
    } else {
        log_error("open file %s failed:[%s]", file_name, strerror(errno));
        size = 0;
    }
    return size;
}
