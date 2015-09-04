/*
 * By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
 * code any way you wish, private, educational, or commercial.  It's free.
 * Use for hash table lookup, or anything where one collision in 2^^32 is
 * acceptable.  Do NOT use for cryptographic purposes.
 * http://burtleburtle.net/bob/hash/index.html
 *
 * Modified by Brian Pontz for libmemcached
 * TODO:
 * Add big endian support
 */

#include <stdint.h>
#include <sys/param.h>
#include "ez_hash.h"

/*
 * My best guess at if you are big-endian or little-endian.  This may
 * need adjustment.
 */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
#define WORDS_LITTLE_ENDIAN 1
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
#define WORDS_BIGENDIAN 1
#else
#error "not find cpu byte_order define."
#endif

#define hashsize(n) ((uint32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x, k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a, b, c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a, b, c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

#define JENKINS_INITVAL 13

/*
 * jenkins_hash() -- hash a variable-length key into a 32-bit value
 *  k       : the key (the unaligned variable-length array of bytes)
 *  length  : the length of the key, counting by bytes
 *  initval : can be any 4-byte value
 * Returns a 32-bit value.  Every bit of the key affects every bit of
 * the return value.  Two keys differing by one or two bits will have
 * totally different hash values.

 * The best hash table sizes are powers of 2.  There is no need to do
 * mod a prime (mod is sooo slow!).  If you need less than 32 bits,
 * use a bitmask.  For example, if you need only 10 bits, do
 *   h = (h & hashmask(10));
 * In which case, the hash table should have hashsize(10) elements.
 */

uint32_t jenkins_hash(const char *key, size_t length) {
    uint32_t a, b, c;    /* internal state */
    union {
        const void *ptr;
        size_t i;
    } u;            /* needed for Mac Powerbook G4 */

    /* Set up the internal state */
    a = b = c = 0xdeadbeef + ((uint32_t) length) + JENKINS_INITVAL;

    u.ptr = key;
#ifndef WORDS_BIGENDIAN
    if ((u.i & 0x3) == 0) {
        const uint32_t *k = (const uint32_t *) key;    /* read 32-bit chunks */

        /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
        while (length > 12) {
            a += k[0];
            b += k[1];
            c += k[2];
            mix(a, b, c);
            length -= 12;
            k += 3;
        }

        /*----------------------------- handle the last (probably partial) block */
        /*
         * "k[2]&0xffffff" actually reads beyond the end of the string, but
         * then masks off the part it's not allowed to read.  Because the
         * string is aligned, the masked-off tail is in the same word as the
         * rest of the string.  Every machine with memory protection I've seen
         * does it on word boundaries, so is OK with this.  But VALGRIND will
         * still catch it and complain.  The masking trick does make the hash
         * noticably faster for short strings (like English words).
         */
        switch (length) {
            case 12:
                c += k[2];
                b += k[1];
                a += k[0];
                break;
            case 11:
                c += k[2] & 0xffffff;
                b += k[1];
                a += k[0];
                break;
            case 10:
                c += k[2] & 0xffff;
                b += k[1];
                a += k[0];
                break;
            case 9:
                c += k[2] & 0xff;
                b += k[1];
                a += k[0];
                break;
            case 8:
                b += k[1];
                a += k[0];
                break;
            case 7:
                b += k[1] & 0xffffff;
                a += k[0];
                break;
            case 6:
                b += k[1] & 0xffff;
                a += k[0];
                break;
            case 5:
                b += k[1] & 0xff;
                a += k[0];
                break;
            case 4:
                a += k[0];
                break;
            case 3:
                a += k[0] & 0xffffff;
                break;
            case 2:
                a += k[0] & 0xffff;
                break;
            case 1:
                a += k[0] & 0xff;
                break;
            case 0:
                return c;    /* zero length strings require no mixing */
            default:
                return c;
        }

    } else if ((u.i & 0x1) == 0) {
        const uint16_t *k = (const uint16_t *) key;    /* read 16-bit chunks */
        const uint8_t *k8;

        /*--------------- all but last block: aligned reads and different mixing */
        while (length > 12) {
            a += k[0] + (((uint32_t) k[1]) << 16);
            b += k[2] + (((uint32_t) k[3]) << 16);
            c += k[4] + (((uint32_t) k[5]) << 16);
            mix(a, b, c);
            length -= 12;
            k += 6;
        }

        /*----------------------------- handle the last (probably partial) block */
        k8 = (const uint8_t *) k;
        switch (length) {
            case 12:
                c += k[4] + (((uint32_t) k[5]) << 16);
                b += k[2] + (((uint32_t) k[3]) << 16);
                a += k[0] + (((uint32_t) k[1]) << 16);
                break;
            case 11:
                c += ((uint32_t) k8[10]) << 16;    /* fall through */
            case 10:
                c += k[4];
                b += k[2] + (((uint32_t) k[3]) << 16);
                a += k[0] + (((uint32_t) k[1]) << 16);
                break;
            case 9:
                c += k8[8];    /* fall through */
            case 8:
                b += k[2] + (((uint32_t) k[3]) << 16);
                a += k[0] + (((uint32_t) k[1]) << 16);
                break;
            case 7:
                b += ((uint32_t) k8[6]) << 16;    /* fall through */
            case 6:
                b += k[2];
                a += k[0] + (((uint32_t) k[1]) << 16);
                break;
            case 5:
                b += k8[4];    /* fall through */
            case 4:
                a += k[0] + (((uint32_t) k[1]) << 16);
                break;
            case 3:
                a += ((uint32_t) k8[2]) << 16;    /* fall through */
            case 2:
                a += k[0];
                break;
            case 1:
                a += k8[0];
                break;
            case 0:
                return c;    /* zero length requires no mixing */
            default:
                return c;
        }

    } else {        /* need to read the key one byte at a time */
#endif				/* little endian */
        const uint8_t *k = (const uint8_t *) key;

        /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
        while (length > 12) {
            a += k[0];
            a += ((uint32_t) k[1]) << 8;
            a += ((uint32_t) k[2]) << 16;
            a += ((uint32_t) k[3]) << 24;
            b += k[4];
            b += ((uint32_t) k[5]) << 8;
            b += ((uint32_t) k[6]) << 16;
            b += ((uint32_t) k[7]) << 24;
            c += k[8];
            c += ((uint32_t) k[9]) << 8;
            c += ((uint32_t) k[10]) << 16;
            c += ((uint32_t) k[11]) << 24;
            mix(a, b, c);
            length -= 12;
            k += 12;
        }

        /*-------------------------------- last block: affect all 32 bits of (c) */
        switch (length) {    /* all the case statements fall through */
            case 12:
                c += ((uint32_t) k[11]) << 24;
            case 11:
                c += ((uint32_t) k[10]) << 16;
            case 10:
                c += ((uint32_t) k[9]) << 8;
            case 9:
                c += k[8];
            case 8:
                b += ((uint32_t) k[7]) << 24;
            case 7:
                b += ((uint32_t) k[6]) << 16;
            case 6:
                b += ((uint32_t) k[5]) << 8;
            case 5:
                b += k[4];
            case 4:
                a += ((uint32_t) k[3]) << 24;
            case 3:
                a += ((uint32_t) k[2]) << 16;
            case 2:
                a += ((uint32_t) k[1]) << 8;
            case 1:
                a += k[0];
                break;
            case 0:
                return c;
            default:
                return c;
        }
#ifndef WORDS_BIGENDIAN
    }
#endif

    final(a, b, c);
    return c;
}

/*
 * Robert Jenkins' reversible 32 bit mix hash function
 */
uint32_t jenkins_rev_mix32(uint32_t key) {
    key += (key << 12);  // key *= (1 + (1 << 12))
    key ^= (key >> 22);
    key += (key << 4);   // key *= (1 + (1 << 4))
    key ^= (key >> 9);
    key += (key << 10);  // key *= (1 + (1 << 10))
    key ^= (key >> 2);
    // key *= (1 + (1 << 7)) * (1 + (1 << 12))
    key += (key << 7);
    key += (key << 12);
    return key;
}

uint64_t twang_mix64(uint64_t key) {
    key = (~key) + (key << 21);  // key *= (1 << 21) - 1; key -= 1;
    key = key ^ (key >> 24);
    key = key + (key << 3) + (key << 8);  // key *= 1 + (1 << 3) + (1 << 8)
    key = key ^ (key >> 14);
    key = key + (key << 2) + (key << 4);  // key *= 1 + (1 << 2) + (1 << 4)
    key = key ^ (key >> 28);
    key = key + (key << 31);  // key *= 1 + (1 << 31)
    return key;
}

uint32_t twang_32from64(uint64_t key) {
    key = (~key) + (key << 18);
    key = key ^ (key >> 31);
    key = key * 21;
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    return (uint32_t) key;
}

static const uint64_t FNV_64_HASH_START = 14695981039346656037ULL;

uint64_t fnv_hash(const char *key, size_t length) {
    uint64_t hash = FNV_64_HASH_START;
    for (size_t i = 0; i < length; ++i) {
        hash += (hash << 1) + (hash << 4) + (hash << 5) + (hash << 7) +
                (hash << 8) + (hash << 40);
        hash ^= key[i];
    }
    return hash;
}
