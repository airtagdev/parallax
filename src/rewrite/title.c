#include "port.h"
static const char *texture_root = "assets/minecraft/textures/";
static Image *texture(const char *name) {
    char path[512];
    snprintf(path, sizeof(path), "%s%s.png", texture_root, name);
    return image_load(path);
}
static void nine_slice(Image *img, float x, float y, float w, float h) {
    if (!img)
        return;
    float border = 3;
    float xs[4] = {x, x + border, x + w - border, x + w}, ys[4] = {y, y + border, y + h - border, y + h};
    float us[4] = {0, border / img->width, 1 - border / img->width, 1},
          vs[4] = {0, border / img->height, 1 - border / img->height, 1};
    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 3; col++) {
            float source_w = (us[col + 1] - us[col]) * img->width;
            float source_h = (vs[row + 1] - vs[row]) * img->height;
            for (float yy = ys[row]; yy < ys[row + 1] - .001f; yy += source_h)
                for (float xx = xs[col]; xx < xs[col + 1] - .001f; xx += source_w) {
                    float tile_w = fminf(source_w, xs[col + 1] - xx);
                    float tile_h = fminf(source_h, ys[row + 1] - yy);
                    image_draw(img, xx, yy, tile_w, tile_h, us[col], vs[row], tile_w / img->width,
                               tile_h / img->height, 0xffffffff);
                }
        }
}
static int title_button(float x, float y, float w, const char *label, int mx, int my) {
    int hover = mx >= x && my >= y && mx < x + w && my < y + 20;
    nine_slice(texture(hover ? "gui/sprites/widget/button_highlighted" : "gui/sprites/widget/button"), x, y,
               w, 20);
    font_draw(x + (w - font_width(label)) * .5f, y + 6, label, 0xffffffff, 1);
    if (hover && click) {
        click = 0;
        return 1;
    }
    return 0;
}
static void not_implemented(const char *system) {
    char message[512];
    snprintf(message, sizeof(message),
             "%s has not been implemented in the C rewrite yet.\n\nThe old sandbox is retired. This build "
             "does not substitute its worlds or mechanics for Minecraft 26.3.",
             system);
    MessageBoxA(main_window, message, "Parallax - implementation gap", MB_OK | MB_ICONINFORMATION);
}
static void panorama(double time) {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double top = .1 * tan(85. * 3.141592653589793 / 360.);
    double side = top * window_width / window_height;
    glFrustum(-side, side, -top, top, .1, 10);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(10.f + (float)sin(time * .05) * 5, 1, 0, 0);
    glRotatef((float)time * -.5f, 0, 1, 0);
    const float faces[6][4][3] = {{{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}},
                                  {{1, -1, -1}, {1, -1, 1}, {1, 1, 1}, {1, 1, -1}},
                                  {{1, -1, 1}, {-1, -1, 1}, {-1, 1, 1}, {1, 1, 1}},
                                  {{-1, -1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}},
                                  {{-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1}},
                                  {{-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1}}};
    for (int face = 0; face < 6; face++) {
        char name[96];
        snprintf(name, sizeof(name), "gui/title/background/panorama_%d", face);
        Image *img = texture(name);
        if (!img)
            continue;
        glBindTexture(GL_TEXTURE_2D, img->texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glColor3f(1, 1, 1);
        glBegin(GL_QUADS);
        for (int v = 0; v < 4; v++) {
            glTexCoord2f(v == 1 || v == 2 ? 1.f : 0.f, v < 2 ? 1.f : 0.f);
            glVertex3fv(faces[face][v]);
        }
        glEnd();
    }
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
}
int title_frame(double time, int frozen) {
    int scale = 1;
    while (window_width / (scale + 1) >= 320 && window_height / (scale + 1) >= 240)
        scale++;
    int width = (window_width + scale - 1) / scale, height = (window_height + scale - 1) / scale,
        mx = mouse_x / scale, my = mouse_y / scale;
    panorama(frozen ? 0 : time);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    image_draw(texture("gui/title/background/panorama_overlay"), 0, 0, (float)width, (float)height, 0, 0, 1,
               1, 0xffffffff);
    image_draw(texture("gui/title/minecraft"), width * .5f - 128, 30, 256, 64, 0, 0, 1, 1, 0xffffffff);
    image_draw(texture("gui/title/edition"), width * .5f - 64, 67, 128, 16, 0, 0, 1, 1, 0xffffffff);
    int first = height / 4 + 48;
    float left = width * .5f - 100;
    if (title_button(left, (float)first, 200, translate("menu.singleplayer"), mx, my))
        not_implemented("Singleplayer");
    if (title_button(left, (float)first + 24, 200, translate("menu.multiplayer"), mx, my))
        not_implemented("Multiplayer");
    if (title_button(left, (float)first + 48, 200, translate("menu.online"), mx, my))
        not_implemented("Realms");
    if (title_button(left, (float)first + 96, 98, translate("menu.options"), mx, my))
        not_implemented("Options");
    if (title_button(left + 102, (float)first + 96, 98, translate("menu.quit"), mx, my))
        return 0;
    if (title_button(width * .5f - 34, (float)first + 72, 20, "", mx, my))
        not_implemented("Player appearance");
    if (title_button(width * .5f - 10, (float)first + 72, 20, "", mx, my))
        not_implemented("Language selection");
    if (title_button(width * .5f + 14, (float)first + 72, 20, "", mx, my))
        not_implemented("Accessibility options");
    Image *language = texture("gui/sprites/icon/language");
    image_draw(language, width * .5f - 7, (float)first + 75, 15, 15, 0, 0, 1, 1, 0xffffffff);
    Image *accessibility = texture("gui/sprites/icon/accessibility");
    image_draw(accessibility, width * .5f + 17, (float)first + 75, 15, 15, 0, 0, 1, 1, 0xffffffff);
    font_draw(2, (float)height - 10, "Minecraft 26.3", 0xffffffff, 1);
    const char *copyright = "Copyright Mojang AB. Do not distribute!";
    font_draw((float)width - font_width(copyright) - 2, (float)height - 10, copyright, 0xffffffff, 1);
    // Splash selection, intro fade and panorama's exact projection/blur are still parity gaps.
    return 1;
}
