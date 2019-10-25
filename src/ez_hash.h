#ifndef __EZ_HASH_H
#define __EZ_HASH_H

#include <stdint.h>

/**
 * google code's provider MurmurHash3
 */
uint32_t MurmurHash3_x86_32(const void *key, int len, uint32_t seed);

// 128 分成 4个32
void MurmurHash3_x86_128(const void *key, int len, uint32_t seed, uint32_t *out);

// 128 分成 2个64
void MurmurHash3_x64_128(const void *key, int len, uint32_t seed, uint64_t *out);

typedef struct hash_s hash_t;

typedef int (*hash_compare_func)(const void *key, int len, const void *find_key, int find_len);

hash_t *hash_create(hash_compare_func func, int buckets);

int hash_put(const hash_t *h, const void *key, int len, const void *val);

void *hash_get(const hash_t *h, const void *key, int len);

void* hash_del(const hash_t *h, const void *key, int len);

#endif                /* __EZ_HASH_H */
