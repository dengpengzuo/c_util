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

#endif				/* __EZ_HASH_H */
