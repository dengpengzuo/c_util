#include <unistd.h>

#include "ez_bytebuf.h"
#include "ez_malloc.h"

bytebuf_t *new_bytebuf(size_t size) {
    bytebuf_t *b = ez_malloc(sizeof(bytebuf_t) + size);
    b->cap = size;
    b->r = b->w = 0;
    return b;
}

void free_bytebuf(bytebuf_t *b) {
    ez_free(b);
}

void bytebuf_reset(bytebuf_t *b) {
    b->r = b->w = 0;
    b->data[0]  = '\0';
}

bytebuf_t *bytebuf_resize(bytebuf_t *b, size_t size) {
    bytebuf_t *nw = ez_realloc(b, b->cap + size);
    nw->cap += size;
    return nw;
}

void bytebuf_write_int8(bytebuf_t *b, int8_t val) {
    b->data[b->w] = val;
    b->w++;
}

// big Endian 写入
void bytebuf_write_int16(bytebuf_t *b, int16_t val) {
    b->data[b->w + 0] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 1] = (uint8_t) ((val) & 0xff);
    b->w += 2;
}

void bytebuf_write_int32(bytebuf_t *b, int32_t val) {
    b->data[b->w + 0] = (uint8_t) ((val >> 24) & 0xff);
    b->data[b->w + 1] = (uint8_t) ((val >> 16) & 0xff);
    b->data[b->w + 2] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 3] = (uint8_t) ((val) & 0xff);
    b->w += 4;
}

void bytebuf_write_int64(bytebuf_t *b, int64_t val) {
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
void bytebuf_write_int16_le(bytebuf_t *b, int16_t val) {
    b->data[b->w + 1] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 0] = (uint8_t) ((val) & 0xff);
    b->w += 2;
}

void bytebuf_write_int32_le(bytebuf_t *b, int32_t val) {
    b->data[b->w + 3] = (uint8_t) ((val >> 24) & 0xff);
    b->data[b->w + 2] = (uint8_t) ((val >> 16) & 0xff);
    b->data[b->w + 1] = (uint8_t) ((val >> 8) & 0xff);
    b->data[b->w + 0] = (uint8_t) ((val) & 0xff);
    b->w += 4;
}

void bytebuf_write_int64_le(bytebuf_t *b, int64_t val) {
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
