#ifndef PX_COMPAT_H
#define PX_COMPAT_H
#include <stdint.h>
typedef struct {
    uint64_t state;
} JavaRandom;
void java_random_seed(JavaRandom *random, int64_t seed);
int32_t java_random_int(JavaRandom *random);
int32_t java_random_bound(JavaRandom *random, int32_t bound);
int64_t java_random_long(JavaRandom *random);
float java_random_float(JavaRandom *random);
double java_random_double(JavaRandom *random);
#endif
