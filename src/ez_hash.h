#ifndef __EZ_HASH_H
#define __EZ_HASH_H

#include <stdint.h>

/**
 * google code's provider MurmurHash3
 */
uint32_t MurmurHash3_x86_32(const void* key, int len, uint32_t seed);

// 128 分成 4个32
void MurmurHash3_x86_128(const void* key, int len, uint32_t seed, uint32_t* out);

// 128 分成 2个64
void MurmurHash3_x64_128(const void* key, int len, uint32_t seed, uint64_t* out);

typedef struct hash_s hash_t;

// key 比较函数
typedef int (*hash_compare_func)(const void* key, const void* find_key);

// key 的hash函数
typedef uint32_t (*key_hash_func)(const void* key);

hash_t* hash_create(hash_compare_func func, key_hash_func key_func, int buckets);

int hash_put(const hash_t* h, const void* key, const void* val);

void* hash_get(const hash_t* h, const void* key);

void* hash_del(const hash_t* h, const void* key);

#endif /* __EZ_HASH_H */
