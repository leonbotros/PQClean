#ifndef SORT_H
#define SORT_H

#include <stddef.h>
#include <stdint.h>

#define int32 int32_t

#define int32_MINMAX(a,b) \
    do { \
        int32 ab = (b) ^ (a); \
        int32 c = (b) - (a); \
        c ^= ab & (c ^ (b)); \
        c >>= 31; \
        c &= ab; \
        (a) ^= c; \
        (b) ^= c; \
    } while(0)

void PQCLEAN_LEDAKEMLT32_CLEAN_uint32_sort(uint32_t *x, size_t n);

#endif
