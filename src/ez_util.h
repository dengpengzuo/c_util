
#ifndef _EZ_UTIL_H_
#define _EZ_UTIL_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

int ez_atoi(uint8_t *line, size_t n);

/*
 * Wrappers around strtoull, strtoll, strtoul, strtol that are safer and
 * easier to use. Returns true if conversion succeeds.
 */
bool ez_strtoull_len(const char *str, uint64_t *out, size_t len);

bool ez_strtoull(const char *str, uint64_t *out);

bool ez_strtoll(const char *str, int64_t *out);

bool ez_strtoul(const char *str, uint32_t *out);

bool ez_strtol(const char *str, int32_t *out);

bool ez_str2oct(const char *str, int32_t *out);

int64_t ustime(void);

int64_t mstime(void);

void ez_localtime_r(const time_t *_time_t, struct tm *_tm);

ssize_t ez_read_file(const char *file_name, uint8_t *buf, size_t len);

// ==========================================
// netty bytebuf
//
typedef struct bytebuf_s {
    uint16_t r; // reader index
    uint16_t w; // writer index
    uint16_t cap;
    uint8_t data[0];
} bytebuf_t;

bytebuf_t *new_bytebuf(int size);

// 可以写入buf的字节数
inline size_t bytebuf_writable_size(bytebuf_t *b) {
    return b->cap - b->w;
}

// 可以从buf中读取的字节数
inline size_t bytebuf_readable_size(bytebuf_t *b) {
    return b->w - b->r;
}

inline void bytebuf_write_int8(bytebuf_t *b, int8_t val) {
    b->data[b->w] = val;
    b->w++;
}

// =======================================================
// big Endian 写入
inline void bytebuf_write_int16(bytebuf_t *b, int16_t val) {
    b->data[b->w + 0] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 1] = (uint8_t) ((val) & 0xff);
    b->w += 2;
}

inline void bytebuf_write_int32(bytebuf_t *b, int32_t val) {
    b->data[b->w + 0] = (uint8_t) ((val >> 24) & 0xff);
    b->data[b->w + 1] = (uint8_t) ((val >> 16) & 0xff);
    b->data[b->w + 2] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 3] = (uint8_t) ((val) & 0xff);
    b->w += 4;
}

inline void bytebuf_write_int64(bytebuf_t *b, int64_t val) {
    b->data[b->w + 0] = (uint8_t) ((val >> 56) & 0xff);
    b->data[b->w + 1] = (uint8_t) ((val >> 48) & 0xff);
    b->data[b->w + 2] = (uint8_t) ((val >> 40) & 0xff);
    b->data[b->w + 3] = (uint8_t) ((val >> 32) & 0xff);
    b->data[b->w + 4] = (uint8_t) ((val >> 24) & 0xff);
    b->data[b->w + 5] = (uint8_t) ((val >> 16) & 0xff);
    b->data[b->w + 6] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 7] = (uint8_t) ((val) & 0xff);
    b->w += 8;
}

// =======================================================
// Little Endian 写入
inline void bytebuf_write_int16_le(bytebuf_t *b, int16_t val) {
    b->data[b->w + 1] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 0] = (uint8_t) ((val) & 0xff);
    b->w += 2;
}

inline void bytebuf_write_int32_le(bytebuf_t *b, int32_t val) {
    b->data[b->w + 3] = (uint8_t) ((val >> 24) & 0xff);
    b->data[b->w + 2] = (uint8_t) ((val >> 16) & 0xff);
    b->data[b->w + 1] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 0] = (uint8_t) ((val) & 0xff);
    b->w += 4;
}

inline void bytebuf_write_int64_le(bytebuf_t *b, int64_t val) {
    b->data[b->w + 7] = (uint8_t) ((val >> 56) & 0xff);
    b->data[b->w + 6] = (uint8_t) ((val >> 48) & 0xff);
    b->data[b->w + 5] = (uint8_t) ((val >> 40) & 0xff);
    b->data[b->w + 4] = (uint8_t) ((val >> 32) & 0xff);
    b->data[b->w + 3] = (uint8_t) ((val >> 24) & 0xff);
    b->data[b->w + 2] = (uint8_t) ((val >> 16) & 0xff);
    b->data[b->w + 1] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 0] = (uint8_t) ((val) & 0xff);
    b->w += 8;
}

#endif
