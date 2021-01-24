#ifndef EZ_BYTEBUF_H
#define EZ_BYTEBUF_H

#include <stdint.h>
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
#define bytebuf_writable_size(b)  (b->cap - b->w)
#define bytebuf_writable_pos(b)   ((uint8_t*)&(b->data[b->w]))

// 可以从buf中读取的字节数
#define bytebuf_readable_size(b)  (b->w - b->r)
#define bytebuf_readable_pos(b)   ((uint8_t*)&(b->data[b->r]))

// bytebuf 写入 int8_t
inline void bytebuf_write_int8(bytebuf_t *b, int8_t val) {
    b->data[b->w] = val;
    b->w++;
}

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

#endif //EZ_BYTEBUF_H
