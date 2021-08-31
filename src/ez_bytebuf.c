#include "ez_bytebuf.h"

#include "ez_malloc.h"

#include <unistd.h>

bytebuf_t* new_bytebuf(size_t size)
{
    bytebuf_t* b = ez_malloc(sizeof(bytebuf_t) + size);
    b->cap = size;
    b->r = b->w = 0;
    return b;
}

void free_bytebuf(bytebuf_t* b)
{
    ez_free(b);
}

void bytebuf_reset(bytebuf_t* b)
{
    b->r = b->w = 0;
    b->data[0] = '\0';
}

bytebuf_t* bytebuf_resize(bytebuf_t* b, size_t size)
{
    bytebuf_t* nw = ez_realloc(b, b->cap + size);
    nw->cap += size;
    return nw;
}

void bytebuf_write_int8(bytebuf_t* b, int8_t val)
{
    b->data[b->w] = val;
    b->w++;
}

void bytebuf_read_int8(bytebuf_t* b, int8_t* val)
{
    *val = b->data[b->r];
    b->r++;
}

// big Endian 写入
void bytebuf_write_int16(bytebuf_t* b, int16_t val)
{
    b->data[b->w + 0] = (uint8_t)((val >> 8) & 0xff);
    b->data[b->w + 1] = (uint8_t)((val)&0xff);
    b->w += 2;
}

void bytebuf_write_int32(bytebuf_t* b, int32_t val)
{
    b->data[b->w + 0] = (uint8_t)((val >> 24) & 0xff);
    b->data[b->w + 1] = (uint8_t)((val >> 16) & 0xff);
    b->data[b->w + 2] = (uint8_t)((val >> 8) & 0xff);
    b->data[b->w + 3] = (uint8_t)((val)&0xff);
    b->w += 4;
}

void bytebuf_write_int64(bytebuf_t* b, int64_t val)
{
    b->data[b->w + 0] = (uint8_t)((val >> 56) & 0xff);
    b->data[b->w + 1] = (uint8_t)((val >> 48) & 0xff);
    b->data[b->w + 2] = (uint8_t)((val >> 40) & 0xff);
    b->data[b->w + 3] = (uint8_t)((val >> 32) & 0xff);
    b->data[b->w + 4] = (uint8_t)((val >> 24) & 0xff);
    b->data[b->w + 5] = (uint8_t)((val >> 16) & 0xff);
    b->data[b->w + 6] = (uint8_t)((val >> 8) & 0xff);
    b->data[b->w + 7] = (uint8_t)((val)&0xff);
    b->w += 8;
}

void bytebuf_read_int64(bytebuf_t* b, int64_t* val)
{
    *val = (int64_t)(b->data[b->r + 0]) << 56 |
           (int64_t)(b->data[b->r + 1]) << 48 |
           (int64_t)(b->data[b->r + 2]) << 40 |
           (int64_t)(b->data[b->r + 3]) << 32 |
           (int64_t)(b->data[b->r + 4]) << 24 |
           (int64_t)(b->data[b->r + 5]) << 16 |
           (int64_t)(b->data[b->r + 6]) <<  8 |
           (int64_t)(b->data[b->r + 7]);
    b->r += 8;
}

void bytebuf_read_int32(bytebuf_t* b, int32_t* val)
{
    *val = (int32_t)(b->data[b->r + 0]) << 24 |
           (int32_t)(b->data[b->r + 1]) << 16 |
           (int32_t)(b->data[b->r + 2]) <<  8 |
           (int32_t)(b->data[b->r + 3]);
    b->r += 4;
}

void bytebuf_read_int16(bytebuf_t* b, int16_t* val)
{
    *val = (int16_t)(b->data[b->r + 0]) << 8 |
           (int16_t)(b->data[b->r + 1]);
    b->r += 2;
}

// Little Endian 写入
void bytebuf_write_int16_le(bytebuf_t* b, int16_t val)
{
    b->data[b->w + 1] = (uint8_t)((val >> 8) & 0xff);
    b->data[b->w + 0] = (uint8_t)((val)&0xff);
    b->w += 2;
}

void bytebuf_write_int32_le(bytebuf_t* b, int32_t val)
{
    b->data[b->w + 3] = (uint8_t)((val >> 24) & 0xff);
    b->data[b->w + 2] = (uint8_t)((val >> 16) & 0xff);
    b->data[b->w + 1] = (uint8_t)((val >> 8) & 0xff);
    b->data[b->w + 0] = (uint8_t)((val)&0xff);
    b->w += 4;
}

void bytebuf_write_int64_le(bytebuf_t* b, int64_t val)
{
    b->data[b->w + 7] = (uint8_t)((val >> 56) & 0xff);
    b->data[b->w + 6] = (uint8_t)((val >> 48) & 0xff);
    b->data[b->w + 5] = (uint8_t)((val >> 40) & 0xff);
    b->data[b->w + 4] = (uint8_t)((val >> 32) & 0xff);
    b->data[b->w + 3] = (uint8_t)((val >> 24) & 0xff);
    b->data[b->w + 2] = (uint8_t)((val >> 16) & 0xff);
    b->data[b->w + 1] = (uint8_t)((val >> 8) & 0xff);
    b->data[b->w + 0] = (uint8_t)((val)&0xff);
    b->w += 8;
}
