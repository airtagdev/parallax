/* Original C resource-data reader. No game implementation is embedded here. */
#include "json.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

typedef struct {
    Json *doc;
    size_t at, length;
} Parser;
static void whitespace(Parser *p) {
    while (p->at < p->length && strchr(" \t\n\r", p->doc->text[p->at]))
        p->at++;
}
static int fail(Parser *p, const char *why) {
    snprintf(p->doc->error, sizeof(p->doc->error), "%s at byte %zu", why, p->at);
    return -1;
}
static int hex_digit(unsigned char c) {
    return c >= '0' && c <= '9'   ? c - '0'
           : c >= 'a' && c <= 'f' ? c - 'a' + 10
           : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                  : -1;
}
static int token(Parser *p, JsonKind kind, size_t begin) {
    Json *d = p->doc;
    if (d->count == d->capacity) {
        int cap = d->capacity ? d->capacity * 2 : 256;
        JsonToken *t = realloc(d->tokens, (size_t)cap * sizeof(*t));
        if (!t)
            return fail(p, "Out of memory");
        d->tokens = t;
        d->capacity = cap;
    }
    int n = d->count++;
    d->tokens[n] = (JsonToken){kind, begin, 0, n + 1, 0};
    return n;
}
static int value(Parser *p, int depth) {
    if (depth > 128)
        return fail(p, "JSON nesting limit exceeded");
    whitespace(p);
    if (p->at >= p->length)
        return fail(p, "Expected value");
    size_t begin = p->at;
    char c = p->doc->text[p->at++];
    int n = -1;
    if (c == '{' || c == '[') {
        n = token(p, c == '{' ? JSON_OBJECT : JSON_ARRAY, begin);
        if (n < 0)
            return -1;
        whitespace(p);
        char close = c == '{' ? '}' : ']';
        if (p->at < p->length && p->doc->text[p->at] == close)
            p->at++;
        else
            while (1) {
                if (c == '{') {
                    whitespace(p);
                    if (p->at >= p->length || p->doc->text[p->at] != '"')
                        return fail(p, "Object key must be a string");
                    if (value(p, depth + 1) < 0)
                        return -1;
                    p->doc->tokens[n].children++;
                    whitespace(p);
                    if (p->at >= p->length || p->doc->text[p->at++] != ':')
                        return fail(p, "Expected colon");
                }
                if (value(p, depth + 1) < 0)
                    return -1;
                p->doc->tokens[n].children++;
                whitespace(p);
                if (p->at >= p->length)
                    return fail(p, "Unterminated collection");
                char next = p->doc->text[p->at++];
                if (next == close)
                    break;
                if (next != ',')
                    return fail(p, "Expected comma");
            }
    } else if (c == '"') {
        n = token(p, JSON_STRING, p->at);
        if (n < 0)
            return -1;
        int closed = 0;
        while (p->at < p->length) {
            unsigned char x = (unsigned char)p->doc->text[p->at++];
            if (x == '"') {
                closed = 1;
                break;
            }
            if (x < 32)
                return fail(p, "Control character in string");
            if (x == '\\') {
                if (p->at >= p->length)
                    return fail(p, "Unterminated escape");
                char e = p->doc->text[p->at++];
                if (e == 'u') {
                    for (int k = 0; k < 4; k++)
                        if (p->at >= p->length || hex_digit((unsigned char)p->doc->text[p->at++]) < 0)
                            return fail(p, "Invalid Unicode escape");
                } else if (!strchr("\"\\/bfnrt", e))
                    return fail(p, "Invalid escape");
            }
        }
        if (!closed)
            return fail(p, "Unterminated string");
        p->doc->tokens[n].length = p->at - p->doc->tokens[n].offset - 1;
    } else if (c == 't' || c == 'f' || c == 'n') {
        const char *literal = c == 't' ? "true" : c == 'f' ? "false" : "null";
        size_t len = strlen(literal);
        if (begin + len > p->length || memcmp(p->doc->text + begin, literal, len))
            return fail(p, "Invalid literal");
        p->at = begin + len;
        n = token(p, c == 'n' ? JSON_NULL : JSON_BOOL, begin);
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        p->at = begin;
        if (p->doc->text[p->at] == '-')
            p->at++;
        if (p->at >= p->length)
            return fail(p, "Invalid number");
        if (p->doc->text[p->at] == '0')
            p->at++;
        else {
            if (p->doc->text[p->at] < '1' || p->doc->text[p->at] > '9')
                return fail(p, "Invalid number");
            while (p->at < p->length && isdigit((unsigned char)p->doc->text[p->at]))
                p->at++;
        }
        if (p->at < p->length && p->doc->text[p->at] == '.') {
            p->at++;
            size_t digits = p->at;
            while (p->at < p->length && isdigit((unsigned char)p->doc->text[p->at]))
                p->at++;
            if (digits == p->at)
                return fail(p, "Invalid fraction");
        }
        if (p->at < p->length && (p->doc->text[p->at] == 'e' || p->doc->text[p->at] == 'E')) {
            p->at++;
            if (p->at < p->length && (p->doc->text[p->at] == '+' || p->doc->text[p->at] == '-'))
                p->at++;
            size_t digits = p->at;
            while (p->at < p->length && isdigit((unsigned char)p->doc->text[p->at]))
                p->at++;
            if (digits == p->at)
                return fail(p, "Invalid exponent");
        }
        n = token(p, JSON_NUMBER, begin);
    } else
        return fail(p, "Unexpected character");
    if (n < 0)
        return -1;
    if (p->doc->tokens[n].kind != JSON_STRING)
        p->doc->tokens[n].length = p->at - begin;
    p->doc->tokens[n].end = p->doc->count;
    return n;
}
int json_parse(Json *d, const char *text, size_t length) {
    memset(d, 0, sizeof(*d));
    if (length > 64 * 1024 * 1024) {
        strcpy(d->error, "JSON file exceeds size limit");
        return 0;
    }
    d->text = malloc(length + 1);
    if (!d->text)
        return 0;
    memcpy(d->text, text, length);
    d->text[length] = 0;
    Parser p = {d, 0, length};
    if (length >= 3 && (unsigned char)d->text[0] == 0xef && (unsigned char)d->text[1] == 0xbb &&
        (unsigned char)d->text[2] == 0xbf)
        p.at = 3;
    if (value(&p, 0) < 0)
        return 0;
    whitespace(&p);
    if (p.at != length) {
        fail(&p, "Trailing data");
        return 0;
    }
    return 1;
}
int json_file(Json *d, const char *path) {
    memset(d, 0, sizeof(*d));
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    if (fseek(f, 0, SEEK_END)) {
        fclose(f);
        return 0;
    }
    long size = ftell(f);
    if (size < 0 || size > 64 * 1024 * 1024) {
        fclose(f);
        return 0;
    }
    rewind(f);
    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return 0;
    }
    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);
    int ok = got == (size_t)size && json_parse(d, buf, got);
    free(buf);
    return ok;
}
void json_free(Json *d) {
    free(d->text);
    free(d->tokens);
    memset(d, 0, sizeof(*d));
}
static int append_utf8(uint32_t cp, char *out, size_t cap, size_t *n) {
    unsigned char bytes[4];
    int count;
    if (cp < 0x80) {
        bytes[0] = (unsigned char)cp;
        count = 1;
    } else if (cp < 0x800) {
        bytes[0] = (unsigned char)(0xc0 | (cp >> 6));
        bytes[1] = (unsigned char)(0x80 | (cp & 63));
        count = 2;
    } else if (cp < 0x10000) {
        bytes[0] = (unsigned char)(0xe0 | (cp >> 12));
        bytes[1] = (unsigned char)(0x80 | ((cp >> 6) & 63));
        bytes[2] = (unsigned char)(0x80 | (cp & 63));
        count = 3;
    } else {
        bytes[0] = (unsigned char)(0xf0 | (cp >> 18));
        bytes[1] = (unsigned char)(0x80 | ((cp >> 12) & 63));
        bytes[2] = (unsigned char)(0x80 | ((cp >> 6) & 63));
        bytes[3] = (unsigned char)(0x80 | (cp & 63));
        count = 4;
    }
    if (*n + (size_t)count >= cap)
        return 0;
    memcpy(out + *n, bytes, (size_t)count);
    *n += (size_t)count;
    return 1;
}
int json_string(const Json *d, int t, char *out, size_t cap) {
    if (!cap)
        return 0;
    out[0] = 0;
    if (t < 0 || t >= d->count || d->tokens[t].kind != JSON_STRING)
        return 0;
    size_t n = 0;
    const JsonToken *token = &d->tokens[t];
    const char *s = d->text + token->offset;
    for (size_t i = 0; i < token->length; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c != '\\') {
            if (n + 1 >= cap)
                return 0;
            out[n++] = (char)c;
            continue;
        }
        c = (unsigned char)s[++i];
        if (c == 'u') {
            uint32_t cp = 0;
            for (int k = 0; k < 4; k++)
                cp = cp * 16 + (uint32_t)hex_digit((unsigned char)s[++i]);
            if (cp >= 0xd800 && cp <= 0xdbff) {
                if (i + 6 >= token->length || s[i + 1] != '\\' || s[i + 2] != 'u')
                    return 0;
                i += 2;
                uint32_t low = 0;
                for (int k = 0; k < 4; k++)
                    low = low * 16 + (uint32_t)hex_digit((unsigned char)s[++i]);
                if (low < 0xdc00 || low > 0xdfff)
                    return 0;
                cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
            } else if (cp >= 0xdc00 && cp <= 0xdfff)
                return 0;
            if (!append_utf8(cp, out, cap, &n))
                return 0;
        } else {
            char decoded = c == 'b'   ? '\b'
                           : c == 'f' ? '\f'
                           : c == 'n' ? '\n'
                           : c == 'r' ? '\r'
                           : c == 't' ? '\t'
                                      : (char)c;
            if (n + 1 >= cap)
                return 0;
            out[n++] = decoded;
        }
    }
    out[n] = 0;
    return 1;
}
int json_equal(const Json *d, int t, const char *s) {
    char buf[1024];
    return json_string(d, t, buf, sizeof(buf)) && strcmp(buf, s) == 0;
}
int json_get(const Json *d, int object, const char *key) {
    if (object < 0 || object >= d->count || d->tokens[object].kind != JSON_OBJECT)
        return -1;
    for (int k = object + 1; k < d->tokens[object].end;) {
        int v = k + 1;
        if (v >= d->tokens[object].end)
            return -1;
        if (json_equal(d, k, key))
            return v;
        k = d->tokens[v].end;
    }
    return -1;
}
double json_number(const Json *d, int t, double fallback) {
    if (t < 0 || t >= d->count || d->tokens[t].kind != JSON_NUMBER)
        return fallback;
    return strtod(d->text + d->tokens[t].offset, NULL);
}
