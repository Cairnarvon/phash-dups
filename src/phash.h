#ifndef PHASH_H
#define PHASH_H

#include <stdint.h>

uint64_t phash_dct(const char*);

unsigned int hamming_uint64_t(uint64_t, uint64_t);

#define ep_uint64_t(x, y) 100 * (64 - hamming_uint64_t((x), (y))) / 64

#endif
