#include "json.h"
#include "compat.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
static int failures;
#define CHECK(x)                                                                                             \
    do {                                                                                                     \
        if (!(x)) {                                                                                          \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                                     \
            failures++;                                                                                      \
        }                                                                                                    \
    } while (0)
int main(int argc, char **argv) {
    Json d;
    const char *fixture =
        "{\"target\":\"26.3\",\"array\":[1,true,null,{\"x\":-2.5e2}],\"text\":\"A\\u00a9\\ud83d\\ude00\"}";
    CHECK(json_parse(&d, fixture, strlen(fixture)));
    CHECK(json_equal(&d, json_get(&d, 0, "target"), "26.3"));
    CHECK(json_get(&d, 0, "missing") == -1);
    int array = json_get(&d, 0, "array");
    CHECK(d.tokens[array].children == 4);
    int nested = d.tokens[array + 3].end;
    CHECK(json_number(&d, json_get(&d, nested, "x"), 0) == -250);
    char text[32];
    CHECK(json_string(&d, json_get(&d, 0, "text"), text, sizeof(text)));
    CHECK(strcmp(text, "A\xc2\xa9\xf0\x9f\x98\x80") == 0);
    json_free(&d);
    const char *bad[] = {"{\"a\":}",     "[1,]",    "{\"a\" 3}",     "01", "1.", "1e+",
                         "true garbage", "\"\\q\"", "\"unterminated"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        CHECK(!json_parse(&d, bad[i], strlen(bad[i])));
        json_free(&d);
    }
    JavaRandom r;
    java_random_seed(&r, 0);
    CHECK(java_random_int(&r) == -1155484576);
    CHECK(java_random_int(&r) == -723955400);
    java_random_seed(&r, 0);
    CHECK(java_random_long(&r) == INT64_C(-4962768465676381896));
    if (argc == 2) {
        FILE *f = fopen(argv[1], "r");
        CHECK(f != NULL);
        if (f) {
            int64_t seed, expected_long;
            int32_t expected_int, expected_bound;
            uint32_t expected_float;
            uint64_t expected_double;
            int cases = 0;
            while (fscanf(f, "%" SCNd64 " %" SCNd32 " %" SCNd32 " %" SCNd64 " %" SCNx32 " %" SCNx64, &seed,
                          &expected_int, &expected_bound, &expected_long, &expected_float,
                          &expected_double) == 6) {
                java_random_seed(&r, seed);
                CHECK(java_random_int(&r) == expected_int);
                CHECK(java_random_bound(&r, 1073741825) == expected_bound);
                CHECK(java_random_long(&r) == expected_long);
                float fval = java_random_float(&r);
                double dval = java_random_double(&r);
                uint32_t fb;
                uint64_t db;
                memcpy(&fb, &fval, 4);
                memcpy(&db, &dval, 8);
                CHECK(fb == expected_float);
                CHECK(db == expected_double);
                cases++;
            }
            CHECK(cases >= 1000);
            printf("Java reference sequences checked: %d\n", cases);
            fclose(f);
        }
    }
    printf("Rewrite core tests: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
