#ifndef __EZ_HASH_H
#define __EZ_HASH_H

#include <stdint.h>

/*
 * Robert Jenkins' reversible 32 bit mix hash function
 */
uint32_t jenkins_rev_mix32(uint32_t key);

/*
 * Thomas Wang 64 bit mix hash function
 */
uint64_t twang_mix64(uint64_t key);

/*
 * Thomas Wang downscaling hash function
 */
uint32_t twang_32from64(uint64_t key);

/**
 * jenkins hash (memcached used.)
 */
uint32_t jenkins_hash(const char *key, size_t length);

/*
 * Fowler / Noll / Vo (FNV) Hash
 *     http://www.isthe.com/chongo/tech/comp/fnv/
 */
uint64_t fnv_hash(const char *key, size_t length);

/**
 * google code's provider MurmurHash3
 */
uint32_t MurmurHash3_x86_32(const void *key, int len, uint32_t seed);

// 128 分成 4个32
void MurmurHash3_x86_128(const void *key, int len, uint32_t seed, uint32_t *out);

// 128 分成 2个64
void MurmurHash3_x64_128(const void *key, int len, uint32_t seed, uint64_t *out);

#endif				/* __EZ_HASH_H */
