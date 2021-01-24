#include <unistd.h>

#include "ez_bytebuf.h"
#include "ez_malloc.h"

bytebuf_t *new_bytebuf(int size) {
    bytebuf_t *b = ez_malloc(sizeof(bytebuf_t) + size);
    b->cap = size;
    b->r = b->w = 0;
    return b;
}
