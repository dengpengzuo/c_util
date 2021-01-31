#ifndef EZ_BYTEBUF_H
#define EZ_BYTEBUF_H

#include <stdint.h>
//
// netty bytebuf
//
typedef struct bytebuf_s {
    uint16_t r; // reader index
    uint16_t w; // writer index
    uint16_t cap;
    uint8_t  data[0];
} bytebuf_t;

bytebuf_t *new_bytebuf(size_t size);

void free_bytebuf(bytebuf_t *b) ;

void bytebuf_reset(bytebuf_t *b);

bytebuf_t *bytebuf_resize(bytebuf_t *b, size_t size);

// =====================================================================
// 可以写入buf的字节数
#define bytebuf_writeable_size(b)        ((b)->cap - (b)->w)

// writer pos
#define bytebuf_writer_pos(b)            ((uint8_t*)&((b)->data[(b)->w]))

// writer index
#define bytebuf_writer_index(b)          ((b)->w)

// can write ?
#define bytebuf_is_writeable(b)          ((b)->cap > (b)->w)

#define bytebuf_reset_writer_index(b)    ((b)->w=0)

// =====================================================================
// 可以从buf中读取的字节数
#define bytebuf_readable_size(b)         ((b)->w - (b)->r)

// reader pos
#define bytebuf_reader_pos(b)            ((uint8_t*)&((b)->data[(b)->r]))

// reader index
#define bytebuf_reader_index(b)          ((b)->r)

// can read ?
#define bytebuf_is_readable(b)           ((b)->w > (b)->r)

#define bytebuf_reset_reader_index(b)    ((b)->r=0)

// =====================================================================
// bytebuf 写入 int8_t
void bytebuf_write_int8(bytebuf_t *b, int8_t val);

// big endian 写入
void bytebuf_write_int16(bytebuf_t *b, int16_t val) ;

void bytebuf_write_int32(bytebuf_t *b, int32_t val) ;

void bytebuf_write_int64(bytebuf_t *b, int64_t val) ;

// little endian 写入
void bytebuf_write_int16_le(bytebuf_t *b, int16_t val) ;

void bytebuf_write_int32_le(bytebuf_t *b, int32_t val) ;

void bytebuf_write_int64_le(bytebuf_t *b, int64_t val) ;

#endif //EZ_BYTEBUF_H
