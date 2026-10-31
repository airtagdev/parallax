#include "compat.h"
#include <string.h>
#include <assert.h>
/* Independent implementation of the specified 48-bit java.util.Random stream.
 * All overflow is explicit unsigned arithmetic, never signed C overflow. */
static uint32_t bits(JavaRandom *r, int count) {
    r->state = (r->state * UINT64_C(0x5deece66d) + 11) & UINT64_C(0xffffffffffff);
    return (uint32_t)(r->state >> (48 - count));
}
void java_random_seed(JavaRandom *r, int64_t seed) {
    r->state = ((uint64_t)seed ^ UINT64_C(0x5deece66d)) & UINT64_C(0xffffffffffff);
}
int32_t java_random_int(JavaRandom *r) {
    uint32_t value = bits(r, 32);
    int32_t result;
    memcpy(&result, &value, 4);
    return result;
}
int32_t java_random_bound(JavaRandom *r, int32_t bound) {
    assert(bound > 0);
    if (bound <= 0)
        return 0;
    if ((bound & (bound - 1)) == 0)
        return (int32_t)(((uint64_t)(uint32_t)bound * bits(r, 31)) >> 31);
    for (;;) {
        uint32_t b = bits(r, 31), v = b % (uint32_t)bound;
        if (b - v + (uint32_t)(bound - 1) < UINT32_C(0x80000000))
            return (int32_t)v;
    }
}
int64_t java_random_long(JavaRandom *r) {
    int32_t hi = java_random_int(r), lo = java_random_int(r);
    uint64_t value = ((uint64_t)(int64_t)hi << 32) + (uint64_t)(int64_t)lo;
    int64_t result;
    memcpy(&result, &value, 8);
    return result;
}
float java_random_float(JavaRandom *r) {
    return (float)bits(r, 24) * (1.f / 16777216.f);
}
double java_random_double(JavaRandom *r) {
    uint64_t value = ((uint64_t)bits(r, 26) << 27) + bits(r, 27);
    return (double)value * (1. / 9007199254740992.);
}
