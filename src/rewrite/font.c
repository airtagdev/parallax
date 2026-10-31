#include "port.h"
static unsigned utf8_next(const char **p) {
    const unsigned char *s = (const unsigned char *)*p;
    unsigned cp = *s++;
    if (cp >= 0xc0 && cp < 0xe0) {
        if ((*s & 0xc0) != 0x80) {
            *p = (const char *)s;
            return '?';
        }
        cp = ((cp & 31) << 6) | (*s++ & 63);
    } else if (cp >= 0xe0 && cp < 0xf0) {
        for (int i = 0; i < 2; i++) {
            if ((*s & 0xc0) != 0x80) {
                *p = (const char *)s;
                return '?';
            }
            cp = (cp << 6) | (*s++ & 63);
        }
        cp &= 0xffff;
    } else if (cp >= 0xf0 && cp < 0xf8) {
        cp &= 7;
        for (int i = 0; i < 3; i++) {
            if ((*s & 0xc0) != 0x80) {
                *p = (const char *)s;
                return '?';
            }
            cp = (cp << 6) | (*s++ & 63);
        }
    }
    *p = (const char *)s;
    return cp;
}
static Glyph *glyph(unsigned cp) {
    for (int i = 0; i < resources.glyph_count; i++)
        if (resources.glyphs[i].codepoint == cp)
            return &resources.glyphs[i];
    return NULL;
}
int font_width(const char *text) {
    int w = 0;
    while (*text) {
        unsigned cp = utf8_next(&text);
        if (cp == ' ') {
            w += 4;
            continue;
        }
        Glyph *g = glyph(cp);
        w += g ? g->advance : 6;
    }
    return w;
}
void image_draw(Image *img, float x, float y, float w, float h, float u, float v, float uw, float vh,
                uint32_t tint) {
    if (!img)
        return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, img->texture);
    glColor4ub((GLubyte)(tint >> 16), (GLubyte)(tint >> 8), (GLubyte)tint, (GLubyte)(tint >> 24));
    glBegin(GL_QUADS);
    glTexCoord2f(u, v);
    glVertex2f(x, y);
    glTexCoord2f(u + uw, v);
    glVertex2f(x + w, y);
    glTexCoord2f(u + uw, v + vh);
    glVertex2f(x + w, y + h);
    glTexCoord2f(u, v + vh);
    glVertex2f(x, y + h);
    glEnd();
}
void fill(float x, float y, float w, float h, uint32_t color) {
    glDisable(GL_TEXTURE_2D);
    glColor4ub((GLubyte)(color >> 16), (GLubyte)(color >> 8), (GLubyte)color, (GLubyte)(color >> 24));
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}
static void text_layer(float x, float y, const char *text, uint32_t color) {
    while (*text) {
        unsigned cp = utf8_next(&text);
        if (cp == ' ') {
            x += 4;
            continue;
        }
        Glyph *g = glyph(cp);
        if (!g)
            g = glyph('?');
        if (!g) {
            x += 6;
            continue;
        }
        FontAtlas *a = &resources.atlases[g->atlas];
        int col = g->cell % a->cols, row = g->cell / a->cols;
        float height = (float)a->height,
              width = (float)a->image->width / a->cols * (height / ((float)a->image->height / a->rows));
        image_draw(a->image, x, y + 7 - a->ascent, width, height, (float)col / a->cols, (float)row / a->rows,
                   1.f / a->cols, 1.f / a->rows, color);
        x += g->advance;
    }
}
void font_draw(float x, float y, const char *text, uint32_t color, int shadow) {
    if (shadow) {
        uint32_t shade = (color & 0xff000000) | ((color & 0xfcfcfc) >> 2);
        text_layer(x + 1, y + 1, text, shade);
    }
    text_layer(x, y, text, color);
}
