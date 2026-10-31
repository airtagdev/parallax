#include "parallax.h"
#include <wincodec.h>

unsigned int textures[BLOCK_COUNT], font_base, chunk_lists[CHUNKS][CHUNKS];
int view_distance = 7;
static unsigned int grass_side, log_top;
static HFONT font;
static const char *texture_files[BLOCK_COUNT] = {
    NULL,         "grass_block_top", "dirt",     "stone",    "sand",        "oak_log",
    "oak_leaves", "oak_planks",      "coal_ore", "iron_ore", "diamond_ore", "water_still",
    "snow",       "bricks",          "glass",    "bedrock",  "glowstone"};
static const float palette[BLOCK_COUNT][3] = {
    {0, 0, 0},          {.35f, .69f, .22f}, {.52f, .34f, .2f}, {.6f, .62f, .65f}, {.88f, .81f, .54f},
    {.45f, .31f, .18f}, {.3f, .56f, .2f},   {.7f, .52f, .3f},  {.3f, .3f, .32f},  {.68f, .51f, .4f},
    {.21f, .85f, .84f}, {.15f, .42f, .76f}, {.9f, .95f, 1},    {.65f, .3f, .23f}, {.68f, .88f, .93f},
    {.22f, .23f, .26f}, {1, .79f, .3f}};
static GLuint load_texture(const char *name, int fallback) {
    unsigned char *pixels = NULL;
    UINT width = 16, height = 16;
    int imported = 0;
    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICFormatConverter *converter = NULL;
    char path[MAX_PATH];
    wchar_t wide[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\%s.png", asset_dir, name);
    MultiByteToWideChar(CP_ACP, 0, path, -1, wide, MAX_PATH);
    HRESULT hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                                  &IID_IWICImagingFactory, (void **)&factory);
    if (SUCCEEDED(hr))
        hr = IWICImagingFactory_CreateDecoderFromFilename(factory, wide, NULL, GENERIC_READ,
                                                          WICDecodeMetadataCacheOnLoad, &decoder);
    if (SUCCEEDED(hr))
        hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    if (SUCCEEDED(hr))
        hr = IWICBitmapFrameDecode_GetSize(frame, &width, &height);
    if (width > 1024 || height > 65536 || width == 0 || height == 0)
        hr = E_FAIL;
    if (SUCCEEDED(hr))
        hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    if (SUCCEEDED(hr))
        hr = IWICFormatConverter_Initialize(converter, (IWICBitmapSource *)frame,
                                            &GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 0,
                                            WICBitmapPaletteTypeCustom);
    if (SUCCEEDED(hr)) {
        if (height > width)
            height = width;
        pixels = (unsigned char *)malloc(width * height * 4);
        WICRect crop = {0, 0, (INT)width, (INT)height};
        if (!pixels)
            hr = E_OUTOFMEMORY;
        else
            hr = IWICFormatConverter_CopyPixels(converter, &crop, width * 4, width * height * 4, pixels);
        if (SUCCEEDED(hr))
            imported = 1;
    }
    if (converter)
        IWICFormatConverter_Release(converter);
    if (frame)
        IWICBitmapFrameDecode_Release(frame);
    if (decoder)
        IWICBitmapDecoder_Release(decoder);
    if (factory)
        IWICImagingFactory_Release(factory);
    if (!imported) {
        free(pixels);
        width = height = 16;
        pixels = (unsigned char *)malloc(width * height * 4);
        if (!pixels)
            return 0;
        for (UINT y = 0; y < height; y++)
            for (UINT x = 0; x < width; x++) {
                int i = (int)(y * width + x) * 4;
                float shade = .85f + (hash_coords(x, 0, y, 71) % 100) / 500.f;
                for (int c = 0; c < 3; c++)
                    pixels[i + c] = (unsigned char)(fminf(1, palette[fallback][c] * shade) * 255);
                pixels[i + 3] = 255;
            }
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    free(pixels);
    return tex;
}
void render_init(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, .45f);
    for (int b = 1; b < BLOCK_COUNT; b++)
        textures[b] = load_texture(texture_files[b], b);
    grass_side = load_texture("grass_block_side", DIRT);
    log_top = load_texture("oak_log_top", LOG);
    font = CreateFontA(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS,
                       CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, "Segoe UI");
    HDC dc = wglGetCurrentDC();
    HGDIOBJ old = SelectObject(dc, font);
    font_base = glGenLists(96);
    wglUseFontBitmapsA(dc, 32, 96, font_base);
    SelectObject(dc, old);
    for (int x = 0; x < CHUNKS; x++)
        for (int z = 0; z < CHUNKS; z++) {
            chunk_lists[x][z] = glGenLists(1);
            meshes_dirty[x][z] = 1;
        }
}
void render_shutdown(void) {
    for (int b = 1; b < BLOCK_COUNT; b++)
        glDeleteTextures(1, &textures[b]);
    glDeleteTextures(1, &grass_side);
    glDeleteTextures(1, &log_top);
    glDeleteLists(font_base, 96);
    for (int x = 0; x < CHUNKS; x++)
        for (int z = 0; z < CHUNKS; z++)
            glDeleteLists(chunk_lists[x][z], 1);
    if (font)
        DeleteObject(font);
}
static const int normals[6][3] = {{0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
static const float vertices[6][4][3] = {
    {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}}, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}},
    {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}, {{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}},
    {{1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}}, {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}};
static void rebuild_chunk(int cx, int cz) {
    glNewList(chunk_lists[cx][cz], GL_COMPILE);
    for (int b = 1; b < BLOCK_COUNT; b++) {
        if (b == WATER)
            continue;
        for (int face = 0; face < 6; face++) {
            GLuint tex = textures[b];
            if (b == GRASS && face > 0)
                tex = face == 1 ? textures[DIRT] : grass_side;
            if (b == LOG && face < 2)
                tex = log_top;
            glBindTexture(GL_TEXTURE_2D, tex);
            glBegin(GL_QUADS);
            glNormal3f((float)normals[face][0], (float)normals[face][1], (float)normals[face][2]);
            for (int x = cx * CHUNK; x < (cx + 1) * CHUNK; x++)
                for (int z = cz * CHUNK; z < (cz + 1) * CHUNK; z++)
                    for (int y = 0; y < WORLD_Y; y++) {
                        if (block_at(x, y, z) != b)
                            continue;
                        int neighbor =
                            block_at(x + normals[face][0], y + normals[face][1], z + normals[face][2]);
                        if (solid(neighbor) && !(neighbor == GLASS && b != GLASS) &&
                            !(neighbor == LEAVES && b != LEAVES))
                            continue;
                        float shade = face == 0 ? 1.f : face == 1 ? .48f : face < 4 ? .78f : .64f;
                        shade *= .82f + .18f * (float)y / WORLD_Y;
                        if (b == GRASS && face == 0)
                            glColor3f(.49f * shade, .79f * shade, .32f * shade);
                        else if (b == LEAVES)
                            glColor3f(.42f * shade, .72f * shade, .30f * shade);
                        else if (b == TORCH)
                            glColor3f(1, .9f, .65f);
                        else
                            glColor3f(shade, shade, shade);
                        float size = b == TORCH ? .25f : 1, off = b == TORCH ? .375f : 0;
                        for (int v = 0; v < 4; v++) {
                            glTexCoord2f((v == 1 || v == 2) ? 1.f : 0.f, (v >= 2) ? 0.f : 1.f);
                            glVertex3f(x + off + vertices[face][v][0] * size,
                                       y + vertices[face][v][1] * (b == TORCH ? .7f : 1),
                                       z + off + vertices[face][v][2] * size);
                        }
                    }
            glEnd();
        }
    }
    glEndList();
    meshes_dirty[cx][cz] = 0;
}
static void cube(float x, float y, float z, float w, float h, float d, float r, float g, float b) {
    glBegin(GL_QUADS);
    for (int f = 0; f < 6; f++) {
        float shade = f == 0 ? 1 : f < 2 ? .65f : .82f;
        glColor3f(r * shade, g * shade, b * shade);
        glNormal3f((float)normals[f][0], (float)normals[f][1], (float)normals[f][2]);
        for (int v = 0; v < 4; v++)
            glVertex3f(x + vertices[f][v][0] * w, y + vertices[f][v][1] * h, z + vertices[f][v][2] * d);
    }
    glEnd();
}
static void draw_mobs(void) {
    glDisable(GL_TEXTURE_2D);
    for (int i = 0; i < MOB_COUNT; i++) {
        Mob *m = &world.mobs[i];
        if (m->hp <= 0 || fabsf(m->x - world.pos.x) > 100 || fabsf(m->z - world.pos.z) > 100)
            continue;
        glPushMatrix();
        glTranslatef(m->x, m->y, m->z);
        glRotatef(m->yaw * 180 / (float)PI, 0, 1, 0);
        float r = m->kind == 0   ? .95f
                  : m->kind == 1 ? .33f
                  : m->kind == 2 ? .93f
                                 : .32f,
              g = m->kind == 0   ? .94f
                  : m->kind == 1 ? .23f
                  : m->kind == 2 ? .6f
                                 : .65f,
              b = m->kind == 0   ? .87f
                  : m->kind == 1 ? .17f
                  : m->kind == 2 ? .62f
                                 : .31f;
        float stride = sinf((float)world.time * .3f + i) * .10f;
        if (m->kind == 3) {
            cube(-.27f, .6f, -.15f, .54f, .65f, .30f, .23f, .43f, .65f);
            cube(-.25f, 1.25f, -.25f, .5f, .5f, .5f, r, g, b);
            cube(-.27f, 0, -.13f, .22f, .6f, .26f, .27f, .23f, .42f);
            cube(.05f, 0, -.13f, .22f, .6f, .26f, .27f, .23f, .42f);
            cube(-.43f, .95f, -.1f, .16f, .18f, .65f, r, g, b);
            cube(.27f, .95f, -.1f, .16f, .18f, .65f, r, g, b);
        } else {
            cube(-.42f, .4f, -.65f, .84f, .64f, 1.2f, r, g, b);
            cube(-.25f, .75f, .4f, .5f, .5f, .5f, r * .9f, g * .9f, b * .9f);
            for (int j = 0; j < 4; j++)
                cube(j % 2 ? .20f : -.37f, .06f + (j % 2 ? stride : -stride), j < 2 ? -.5f : .3f, .17f, .45f,
                     .18f, r * .6f, g * .6f, b * .6f);
        }
        float eye_y = m->kind == 3 ? 1.55f : 1.08f, eye_z = m->kind == 3 ? .255f : .91f;
        cube(-.17f, eye_y, eye_z, .09f, .08f, .01f, .06f, .06f, .07f);
        cube(.08f, eye_y, eye_z, .09f, .08f, .01f, .06f, .06f, .07f);
        glPopMatrix();
    }
    glEnable(GL_TEXTURE_2D);
}
void rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    glDisable(GL_TEXTURE_2D);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}
void text_at(float x, float y, const char *text, float r, float g, float b) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(r, g, b);
    glRasterPos2f(x, y + 17);
    glListBase(font_base - 32);
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
    glEnable(GL_TEXTURE_2D);
}
int button(float x, float y, float w, float h, const char *label) {
    int hover = mouse_x >= x && mouse_y >= y && mouse_x < x + w && mouse_y < y + h;
    rect(x, y, w, h, hover ? .14f : .09f, hover ? .25f : .15f, hover ? .33f : .21f, 1);
    rect(x, y, 3, h, .28f, .82f, .74f, 1);
    text_at(x + 18, y + (h - 20) / 2, label, .88f, .94f, .96f);
    return hover && mouse_click;
}
void render_frame(float dt) {
    static float elapsed;
    elapsed += dt;
    float day = .5f + .5f * cosf(((float)world.time - 6000.f) / 24000.f * 2 * (float)PI);
    day = fmaxf(.12f, day);
    int inworld = screen == GAME || screen == PAUSE || screen == INVENTORY || screen == ACHIEVEMENTS;
    if (!inworld) {
        glClearColor(.035f, .055f, .085f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        float sky[4] = {.32f * day, .57f * day, .78f * day, 1};
        glClearColor(sky[0], sky[1], sky[2], 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double clip_near = .07, top = clip_near * tan(75 * PI / 360), right = top * (double)win_w / win_h;
        glFrustum(-right, right, -top, top, clip_near, 220);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(world.pitch, 1, 0, 0);
        glRotatef(world.yaw, 0, 1, 0);
        glTranslatef(-world.pos.x, -world.pos.y - 1.62f, -world.pos.z);
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, sky);
        glFogf(GL_FOG_START, (view_distance - 2) * 16.f);
        glFogf(GL_FOG_END, (view_distance + .2f) * 16.f);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        float ambient[4] = {.28f + day * .6f, .28f + day * .6f, .32f + day * .55f, 1},
              diffuse[4] = {day * .3f, day * .3f, day * .3f, 1}, lightpos[4] = {.4f, 1, .2f, 0};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
        glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
        int cx = (int)world.pos.x / CHUNK, cz = (int)world.pos.z / CHUNK, budget = 3;
        // Nearest meshes first, with a bounded rebuild budget to keep input responsive.
        for (int ring = 0; ring <= view_distance; ring++)
            for (int x = cx - ring; x <= cx + ring; x++)
                for (int z = cz - ring; z <= cz + ring; z++) {
                    if (abs(x - cx) != ring && abs(z - cz) != ring)
                        continue;
                    if (x < 0 || z < 0 || x >= CHUNKS || z >= CHUNKS)
                        continue;
                    if (meshes_dirty[x][z] && budget > 0) {
                        rebuild_chunk(x, z);
                        budget--;
                    }
                    if (!meshes_dirty[x][z])
                        glCallList(chunk_lists[x][z]);
                }
        draw_mobs();
        glDisable(GL_LIGHTING);
        glDisable(GL_COLOR_MATERIAL);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(.14f * day, .43f * day, .68f * day, .65f);
        glBegin(GL_QUADS);
        for (int x = max(0, cx - view_distance) * 16; x < min(WORLD_X, (cx + view_distance + 1) * 16); x++)
            for (int z = max(0, cz - view_distance) * 16; z < min(WORLD_Z, (cz + view_distance + 1) * 16);
                 z++)
                if (block_at(x, WATER_LEVEL, z) == WATER) {
                    float y = WATER_LEVEL + .82f;
                    glVertex3f((float)x, y, (float)z + 1);
                    glVertex3f((float)x + 1, y, (float)z + 1);
                    glVertex3f((float)x + 1, y, (float)z);
                    glVertex3f((float)x, y, (float)z);
                }
        glEnd();
        glDisable(GL_BLEND);
        if (has_target) {
            glDisable(GL_DEPTH_TEST);
            glColor3f(.95f, 1, 1);
            glLineWidth(2);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glPushMatrix();
            glTranslatef(target[0] - .003f, target[1] - .003f, target[2] - .003f);
            glScalef(1.006f, 1.006f, 1.006f);
            cube(0, 0, 0, 1, 1, 1, .85f, .97f, 1);
            glPopMatrix();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_DEPTH_TEST);
        }
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_FOG);
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, win_w, win_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    draw_ui();
    glDisable(GL_BLEND);
}
