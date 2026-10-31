#ifndef PX_JSON_H
#define PX_JSON_H
#include <stddef.h>
typedef enum { JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_NUMBER, JSON_BOOL, JSON_NULL } JsonKind;
typedef struct {
    JsonKind kind;
    size_t offset, length;
    int end, children;
} JsonToken;
typedef struct {
    char *text;
    JsonToken *tokens;
    int count, capacity;
    char error[128];
} Json;
int json_parse(Json *doc, const char *text, size_t length);
int json_file(Json *doc, const char *path);
void json_free(Json *doc);
int json_get(const Json *doc, int object, const char *key);
int json_string(const Json *doc, int token, char *out, size_t capacity);
int json_equal(const Json *doc, int token, const char *value);
double json_number(const Json *doc, int token, double fallback);
#endif
