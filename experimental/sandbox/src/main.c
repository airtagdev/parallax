#include "parallax.h"
#include <shlobj.h>
#include <mmsystem.h>

HWND window;
int keys[256], screen = MENU, win_w = 1440, win_h = 900, mouse_x, mouse_y, mouse_click;
static int active_slot = 1, mouse_captured, mode_creative = 0, input_focus = 0, debug_hud = 0, smoke_mode = 0,
           hotbar_index = 0;
static HDC device;
static HGLRC context;
static int running = 1;
static char world_name[48] = "My world", seed_input[32] = "12345";
static const int hotbar[9] = {LOG, PLANKS, STONE, DIRT, SAND, GLASS, BRICK, TORCH, GRASS};
static LARGE_INTEGER frequency;
static float auto_save;
int current_slot(void) {
    return active_slot;
}
void play_sound(int kind) {
    static unsigned char waves[3][16044];
    static int ready;
    if (!ready) {
        for (int k = 0; k < 3; k++) {
            int samples = k == 2 ? 7700 : k == 1 ? 3300 : 1300;
            unsigned char *p = waves[k];
            uint32_t size = 36 + (uint32_t)samples * 2, rate = 22050, bytes = 44100,
                     data_size = (uint32_t)samples * 2;
            uint16_t fmt = 1, channels = 1, align = 2, bits = 16;
            memcpy(p, "RIFF", 4);
            memcpy(p + 4, &size, 4);
            memcpy(p + 8, "WAVEfmt ", 8);
            size = 16;
            memcpy(p + 16, &size, 4);
            memcpy(p + 20, &fmt, 2);
            memcpy(p + 22, &channels, 2);
            memcpy(p + 24, &rate, 4);
            memcpy(p + 28, &bytes, 4);
            memcpy(p + 32, &align, 2);
            memcpy(p + 34, &bits, 2);
            memcpy(p + 36, "data", 4);
            memcpy(p + 40, &data_size, 4);
            for (int i = 0; i < samples; i++) {
                float t = (float)i / 22050.f, envelope = (1.f - (float)i / samples) * .12f, value;
                if (k == 0)
                    value = ((int)(hash_coords(i, 0, 0, 91) % 1000) / 500.f - 1) * envelope;
                else if (k == 1)
                    value = sinf(t * 2 * (float)PI * (160 - t * 500)) * envelope;
                else {
                    float tone_frequency = i < 2500 ? 523.25f : i < 5000 ? 659.25f : 783.99f;
                    value = sinf(t * 2 * (float)PI * tone_frequency) * envelope;
                }
                int16_t sample = (int16_t)(value * 32767);
                memcpy(p + 44 + i * 2, &sample, 2);
            }
        }
        ready = 1;
    }
    if (!smoke_mode)
        PlaySoundA((LPCSTR)waves[kind % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}
void capture_mouse(int capture) {
    if (smoke_mode)
        return;
    if (capture == mouse_captured)
        return;
    mouse_captured = capture;
    if (capture) {
        while (ShowCursor(FALSE) >= 0) {
        }
        RECT r;
        GetClientRect(window, &r);
        POINT a = {r.left, r.top}, b = {r.right, r.bottom};
        ClientToScreen(window, &a);
        ClientToScreen(window, &b);
        r = (RECT){a.x, a.y, b.x, b.y};
        ClipCursor(&r);
        SetCapture(window);
    } else {
        ClipCursor(NULL);
        ReleaseCapture();
        while (ShowCursor(TRUE) < 0) {
        }
    }
}
void switch_screen(int next) {
    screen = next;
    capture_mouse(next == GAME);
    memset(keys, 0, sizeof(keys));
    if (next == MENU)
        status_time = 0;
}
void start_world(int slot) {
    if (load_world(slot)) {
        active_slot = slot;
        selected = LOG;
        auto_save = 0;
        switch_screen(GAME);
    }
}
void create_world(int slot, const char *name, uint32_t seed, int creative) {
    generate_world(seed, name, creative);
    active_slot = slot;
    selected = PLANKS;
    hotbar_index = 1;
    auto_save = 0;
    save_world(slot);
    switch_screen(GAME);
}
static void center_text(float y, const char *s) {
    text_at(win_w * .5f - (float)strlen(s) * 4.1f, y, s, .68f, .77f, .84f);
}
static void heading(float x, float y, const char *s) {
    text_at(x, y, s, .35f, .9f, .79f);
}
static void backdrop(void) {
    rect(0, 0, (float)win_w, (float)win_h, .028f, .044f, .067f, 1);
    for (int i = 0; i < 12; i++) {
        float x = win_w * .58f + i * 48.f, y = 60 + (i % 4) * 80.f;
        rect(x, y, 34, 34, .045f, .085f, .115f, 1);
    }
    rect(0, 0, (float)win_w, 4, .27f, .84f, .73f, 1);
    text_at(30, 22, "P A R A L L A X", .86f, .95f, .97f);
    text_at(30, 46, "NATIVE C / EARLY ACCESS 0.1", .35f, .65f, .7f);
    text_at(30, (float)win_h - 36, "Singleplayer sandbox  |  Local worlds  |  No sign-in required", .4f, .51f,
            .60f);
}
static void item_icon(float x, float y, int b, float size) {
    glBindTexture(GL_TEXTURE_2D, textures[b]);
    if (b == GRASS || b == LEAVES)
        glColor3f(.45f, .78f, .3f);
    else
        glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(x, y);
    glTexCoord2f(1, 0);
    glVertex2f(x + size, y);
    glTexCoord2f(1, 1);
    glVertex2f(x + size, y + size);
    glTexCoord2f(0, 1);
    glVertex2f(x, y + size);
    glEnd();
}
void draw_ui(void) {
    char line[256];
    float mid = win_w * .5f;
    if (screen == MENU) {
        backdrop();
        float x = mid - 280, y = win_h * .25f;
        heading(x, y, "BUILD SOMETHING. GO SOMEWHERE.");
        text_at(x, y + 40, "A familiar world. A different engine.", .93f, .96f, .98f);
        text_at(x, y + 76, "Explore generated terrain, make a home, and find what lies below.", .57f, .67f,
                .75f);
        if (button(x, y + 135, 560, 58, "SINGLEPLAYER WORLDS"))
            switch_screen(WORLDS);
        if (button(x, y + 207, 560, 48, "QUIT"))
            PostMessageA(window, WM_CLOSE, 0, 0);
        text_at(x, y + 283, "WASD to move   |   Mouse to look   |   E for inventory", .42f, .57f, .64f);
        text_at(x, y + 311, "Original world format and progression. Not an official Minecraft product.", .42f,
                .50f, .57f);
    }
    if (screen == WORLDS) {
        backdrop();
        float x = mid - 330, y = 125;
        heading(x, y, "YOUR WORLDS");
        text_at(x, y + 32, "Five save slots. Each world remembers its blocks, inventory, and achievements.",
                .57f, .67f, .75f);
        for (int i = 1; i <= 5; i++) {
            WorldState info;
            int exists = save_info(i, &info);
            float yy = y + 78 + (i - 1) * 76.f;
            snprintf(line, sizeof(line), "%d  /  %s", i, exists ? info.name : "Empty world slot");
            if (button(x, yy, 660, 62, line)) {
                active_slot = i;
                if (exists)
                    start_world(i);
                else {
                    snprintf(seed_input, sizeof(seed_input), "%lu", GetTickCount());
                    switch_screen(CREATE);
                }
            }
            if (exists) {
                snprintf(line, sizeof(line), "Seed %u  -  %s", info.seed,
                         info.creative ? "Creative" : "Survival");
                text_at(x + 310, yy + 22, line, .46f, .66f, .70f);
            }
        }
        if (button(x, y + 478, 200, 44, "BACK"))
            switch_screen(MENU);
    }
    if (screen == CREATE) {
        backdrop();
        float x = mid - 290, y = 145;
        heading(x, y, "CREATE A WORLD");
        text_at(x, y + 45, "World name", .6f, .72f, .78f);
        rect(x, y + 74, 580, 46, .08f, .13f, .18f, 1);
        text_at(x + 14, y + 85, world_name, .9f, .96f, .98f);
        if (mouse_click && mouse_x > x && mouse_x < x + 580 && mouse_y > y + 74 && mouse_y < y + 120)
            input_focus = 0;
        text_at(x, y + 144, "World seed", .6f, .72f, .78f);
        rect(x, y + 173, 580, 46, .08f, .13f, .18f, 1);
        text_at(x + 14, y + 184, seed_input, .9f, .96f, .98f);
        if (mouse_click && mouse_x > x && mouse_x < x + 580 && mouse_y > y + 173 && mouse_y < y + 219)
            input_focus = 1;
        rect(x, y + (input_focus ? 217 : 118), 580, 2, .26f, .84f, .74f, 1);
        if (button(x, y + 250, 580, 46,
                   mode_creative ? "MODE: CREATIVE  /  unlimited blocks, flight"
                                 : "MODE: SURVIVAL  /  collect, craft, explore"))
            mode_creative = !mode_creative;
        if (button(x, y + 321, 580, 54, "GENERATE & PLAY")) {
            create_world(active_slot, world_name[0] ? world_name : "My world",
                         (uint32_t)strtoul(seed_input, NULL, 10), mode_creative);
        }
        if (button(x, y + 391, 160, 42, "BACK"))
            switch_screen(WORLDS);
        text_at(x, y + 455, "256 x 256 blocks  /  96 blocks high  /  seeded terrain, caves and ores", .43f,
                .57f, .65f);
    }
    if (screen == GAME) {
        rect(20, 20, 245, 66, .025f, .045f, .06f, .8f);
        heading(34, 28, "PARALLAX / SINGLEPLAYER");
        text_at(34, 52, world.name, .8f, .88f, .92f);
        if (debug_hud) {
            snprintf(line, sizeof(line), "XYZ %.1f / %.1f / %.1f   Seed %u", world.pos.x, world.pos.y,
                     world.pos.z, world.seed);
            text_at(26, 103, line, 1, 1, 1);
            snprintf(line, sizeof(line), "View %d chunks  |  Day %d  |  %s", view_distance,
                     1 + (int)world.time / 24000, flying ? "Flying" : "Walking");
            text_at(26, 128, line, 1, 1, 1);
        }
        rect(mid - 8, win_h * .5f - 1, 16, 2, 1, 1, 1, .9f);
        rect(mid - 1, win_h * .5f - 8, 2, 16, 1, 1, 1, .9f);
        float bx = mid - 9 * 31.f, by = (float)win_h - 82;
        for (int i = 0; i < 9; i++) {
            int b = hotbar[i];
            rect(bx + i * 62, by, 58, 58, .04f, .07f, .09f, .9f);
            if (b == selected) {
                rect(bx + i * 62, by, 58, 3, .3f, .94f, .78f, 1);
                rect(bx + i * 62, by + 55, 58, 3, .3f, .94f, .78f, 1);
            }
            item_icon(bx + i * 62 + 15, by + 9, b, 28);
            snprintf(line, sizeof(line), "%d", i + 1);
            text_at(bx + i * 62 + 4, by + 2, line, .7f, .8f, .85f);
            snprintf(line, sizeof(line), "%u", world.inventory[b]);
            text_at(bx + i * 62 + 14, by + 36, world.creative ? "INF" : line, .95f, .96f, .98f);
        }
        snprintf(line, sizeof(line), "%s%s", block_names[selected], flying ? "  /  Flying" : "");
        center_text((float)win_h - 113, line);
        if (!world.creative) {
            for (int i = 0; i < 10; i++) {
                rect(bx + i * 16, by - 25, 12, 9, i < world.health / 2 ? .9f : .2f, .21f, .27f, 1);
                rect(bx + 397 + i * 16, by - 25, 12, 9, i < world.hunger / 2 ? .88f : .2f, .61f, .2f, 1);
            }
        }
        if (has_target) {
            snprintf(line, sizeof(line), "%s", block_names[block_at(target[0], target[1], target[2])]);
            text_at((float)win_w - 225, 27, line, .85f, .94f, .96f);
        }
        text_at(26, (float)win_h - 43, "E Inventory   L Achievements   F Flight   F3 Info   Esc Pause", .73f,
                .83f, .88f);
    }
    if (screen == PAUSE) {
        rect(0, 0, (float)win_w, (float)win_h, .015f, .025f, .04f, .78f);
        float x = mid - 220, y = win_h * .2f;
        heading(x, y, "PAUSED");
        if (button(x, y + 45, 440, 48, "RESUME"))
            switch_screen(GAME);
        if (button(x, y + 106, 440, 48, "INVENTORY & CRAFTING"))
            switch_screen(INVENTORY);
        if (button(x, y + 167, 440, 48, "ACHIEVEMENTS"))
            switch_screen(ACHIEVEMENTS);
        snprintf(line, sizeof(line), "RENDER DISTANCE: %d CHUNKS", view_distance);
        if (button(x, y + 228, 440, 48, line)) {
            view_distance = view_distance >= 10 ? 4 : view_distance + 1;
        }
        if (button(x, y + 289, 440, 48, "SAVE WORLD")) {
            if (save_world(active_slot))
                notify("World saved.");
        }
        if (button(x, y + 350, 440, 48, "SAVE & RETURN TO MENU")) {
            if (save_world(active_slot))
                switch_screen(MENU);
        }
    }
    if (screen == INVENTORY) {
        rect(0, 0, (float)win_w, (float)win_h, .015f, .025f, .04f, .94f);
        float x = mid - 500, y = 90;
        heading(x, y, "INVENTORY");
        text_at(x, y + 30, "Select a block. Press E or Escape to return.", .62f, .74f, .8f);
        for (int b = 1; b < BLOCK_COUNT; b++) {
            int k = b - 1;
            float xx = x + (k % 4) * 136, yy = y + 86 + (k / 4) * 118;
            rect(xx, yy, 126, 108, b == selected ? .10f : .055f, b == selected ? .25f : .10f,
                 b == selected ? .27f : .14f, 1);
            item_icon(xx + 43, yy + 10, b, 40);
            text_at(xx + 7, yy + 56, block_names[b], .87f, .93f, .95f);
            snprintf(line, sizeof(line), "%u", world.inventory[b]);
            text_at(xx + 10, yy + 81, world.creative ? "Unlimited" : line, .4f, .82f, .76f);
            if (mouse_click && mouse_x > xx && mouse_x < xx + 126 && mouse_y > yy && mouse_y < yy + 108 &&
                b != BEDROCK && b != WATER)
                selected = b;
        }
        float rx = x + 590;
        heading(rx, y, "CRAFTING");
        text_at(rx, y + 30, "Simple recipes, no crafting table required.", .62f, .74f, .8f);
        const char *recipes[] = {"4 PLANKS   /   1 oak log", "4 BRICKS   /   4 stone",
                                 "4 GLASS    /   4 sand + 1 coal ore", "8 LANTERNS /   1 plank + 1 coal ore"};
        for (int i = 0; i < 4; i++)
            if (button(rx, y + 86 + i * 72.f, 410, 58, recipes[i]))
                craft_item(i);
        text_at(rx, y + 403, "Survival: collect blocks to build and craft.", .53f, .66f, .74f);
        text_at(rx, y + 432, "Defeat mobs to recover food and health.", .53f, .66f, .74f);
        if (button(rx, y + 489, 410, 44, "BACK TO GAME"))
            switch_screen(GAME);
    }
    if (screen == ACHIEVEMENTS) {
        rect(0, 0, (float)win_w, (float)win_h, .015f, .025f, .04f, .96f);
        float x = mid - 510, y = 55;
        int count = 0;
        for (int i = 0; i < ACH_COUNT; i++)
            if (world.achievements & (1u << i))
                count++;
        snprintf(line, sizeof(line), "ACHIEVEMENTS  /  %d OF %d", count, ACH_COUNT);
        heading(x, y, line);
        text_at(x, y + 30,
                "Parallax progression. Vanilla advancement triggers are not implemented in this alpha.", .57f,
                .70f, .78f);
        for (int i = 0; i < ACH_COUNT; i++) {
            float xx = x + (i / 12) * 530.f, yy = y + 82 + (i % 12) * 43.f;
            int done = (world.achievements & (1u << i)) != 0;
            rect(xx, yy, 510, 39, done ? .065f : .04f, done ? .18f : .07f, done ? .18f : .1f, 1);
            text_at(xx + 9, yy + 8, done ? "+" : "-", done ? .35f : .4f, done ? .95f : .5f, .7f);
            text_at(xx + 32, yy - 1, achievements[i].title, done ? .6f : .77f, done ? .94f : .81f, .83f);
            text_at(xx + 32, yy + 18, achievements[i].description, .45f, .63f, .70f);
        }
        if (button(x, y + 620, 210, 43, "BACK TO GAME"))
            switch_screen(GAME);
    }
    if (status_time > 0) {
        float width = fminf((float)win_w - 40, 700);
        rect(mid - width / 2, (float)win_h - 162, width, 40, .04f, .16f, .18f, .96f);
        text_at(mid - width / 2 + 16, (float)win_h - 153, status_text, .65f, .96f, .86f);
    }
}
static void screenshot(const char *name) {
    size_t stride = ((size_t)win_w * 3 + 3) & ~(size_t)3, n = stride * win_h;
    unsigned char *pixels = (unsigned char *)calloc(1, n);
    if (!pixels)
        return;
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, win_w, win_h, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels);
    BITMAPFILEHEADER file = {0};
    BITMAPINFOHEADER info = {0};
    file.bfType = 0x4D42;
    file.bfOffBits = sizeof(file) + sizeof(info);
    file.bfSize = file.bfOffBits + (DWORD)n;
    info.biSize = sizeof(info);
    info.biWidth = win_w;
    info.biHeight = win_h;
    info.biPlanes = 1;
    info.biBitCount = 24;
    info.biSizeImage = (DWORD)n;
    FILE *f = NULL;
    if (!fopen_s(&f, name, "wb") && f) {
        fwrite(&file, sizeof(file), 1, f);
        fwrite(&info, sizeof(info), 1, f);
        fwrite(pixels, 1, n, f);
        fclose(f);
    }
    free(pixels);
}
static LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_GETMINMAXINFO: {
        MINMAXINFO *limits = (MINMAXINFO *)l;
        limits->ptMinTrackSize.x = 1160;
        limits->ptMinTrackSize.y = 800;
        return 0;
    }
    case WM_CLOSE:
        if (screen == GAME || screen == PAUSE || screen == INVENTORY || screen == ACHIEVEMENTS) {
            if (!save_world(active_slot)) {
                switch_screen(PAUSE);
                return 0;
            }
        }
        running = 0;
        return 0;
    case WM_DESTROY:
        running = 0;
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        win_w = max(1, LOWORD(l));
        win_h = max(1, HIWORD(l));
        if (context)
            glViewport(0, 0, win_w, win_h);
        return 0;
    case WM_KILLFOCUS:
        if (screen == GAME)
            switch_screen(PAUSE);
        return 0;
    case WM_MOUSEMOVE:
        mouse_x = (short)LOWORD(l);
        mouse_y = (short)HIWORD(l);
        return 0;
    case WM_INPUT:
        if (screen == GAME && mouse_captured) {
            RAWINPUT data;
            UINT size = sizeof(data);
            if (GetRawInputData((HRAWINPUT)l, RID_INPUT, &data, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
                break;
            if (data.header.dwType == RIM_TYPEMOUSE) {
                world.yaw += data.data.mouse.lLastX * .12f;
                world.pitch += data.data.mouse.lLastY * .12f;
                world.yaw = fmodf(world.yaw, 360);
                world.pitch = fmaxf(-89, fminf(89, world.pitch));
            }
        }
        break;
    case WM_LBUTTONDOWN:
        if (screen == GAME) {
            keys[VK_LBUTTON] = 1;
            update_target();
            interact(0);
        } else
            mouse_click = 1;
        return 0;
    case WM_LBUTTONUP:
        keys[VK_LBUTTON] = 0;
        return 0;
    case WM_RBUTTONDOWN:
        if (screen == GAME) {
            keys[VK_RBUTTON] = 1;
            update_target();
            interact(1);
        }
        return 0;
    case WM_RBUTTONUP:
        keys[VK_RBUTTON] = 0;
        return 0;
    case WM_MOUSEWHEEL:
        if (screen == GAME) {
            hotbar_index = (hotbar_index + (GET_WHEEL_DELTA_WPARAM(w) > 0 ? 8 : 1)) % 9;
            selected = hotbar[hotbar_index];
        }
        return 0;
    case WM_KEYUP:
        if (w < 256)
            keys[w] = 0;
        return 0;
    case WM_KEYDOWN:
        if (w < 256)
            keys[w] = 1;
        if (l & (1 << 30))
            return 0;
        if (w == VK_ESCAPE) {
            if (screen == GAME)
                switch_screen(PAUSE);
            else if (screen == PAUSE || screen == INVENTORY || screen == ACHIEVEMENTS)
                switch_screen(GAME);
            else if (screen == CREATE)
                switch_screen(WORLDS);
            else
                switch_screen(MENU);
        } else if (w == 'E' && (screen == GAME || screen == INVENTORY))
            switch_screen(screen == GAME ? INVENTORY : GAME);
        else if (w == 'L' && (screen == GAME || screen == ACHIEVEMENTS))
            switch_screen(screen == GAME ? ACHIEVEMENTS : GAME);
        else if (w == 'F' && screen == GAME) {
            flying = !flying;
            notify(flying ? "Flight enabled: Space up, Shift down." : "Flight disabled.");
        } else if (w == VK_F3)
            debug_hud = !debug_hud;
        else if (w >= '1' && w <= '9' && screen == GAME) {
            hotbar_index = (int)(w - '1');
            selected = hotbar[hotbar_index];
        } else if (w == VK_F5 && screen == GAME) {
            if (save_world(active_slot))
                notify("World saved.");
        } else if (w == VK_F2)
            screenshot("parallax-screenshot.bmp");
        else if (w == VK_TAB && screen == CREATE)
            input_focus = !input_focus;
        return 0;
    case WM_CHAR:
        if (screen == CREATE) {
            char *str = input_focus ? seed_input : world_name;
            size_t cap = input_focus ? sizeof(seed_input) : sizeof(world_name), n = strlen(str);
            if (w == 8 && n)
                str[n - 1] = 0;
            else if (w >= 32 && w <= 126 && n < cap - 1 && (!input_focus || (w >= '0' && w <= '9'))) {
                str[n] = (char)w;
                str[n + 1] = 0;
            }
        }
        return 0;
    }
    return DefWindowProcA(hwnd, msg, w, l);
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR args, int show) {
    (void)previous;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    SetProcessDPIAware();
    char exe[MAX_PATH], base[MAX_PATH];
    GetModuleFileNameA(NULL, exe, MAX_PATH);
    strcpy_s(base, sizeof(base), exe);
    char *slash = strrchr(base, '\\');
    if (slash)
        *slash = 0;
    snprintf(asset_dir, sizeof(asset_dir), "%s\\assets", base);
    snprintf(data_dir, sizeof(data_dir), "%s\\saves", base);
    CreateDirectoryA(data_dir, NULL);
    blocks = (unsigned char *)calloc((size_t)WORLD_X * WORLD_Z * WORLD_Y, 1);
    if (!blocks) {
        MessageBoxA(NULL, "Not enough memory to create a world.", "Parallax", MB_ICONERROR);
        return 1;
    }
    if (strstr(args, "--self-test")) {
        int result = self_test();
        free(blocks);
        CoUninitialize();
        return result;
    }
    smoke_mode = strstr(args, "--smoke-test") != NULL;
    WNDCLASSA wc = {0};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "ParallaxNativeC";
    RegisterClassA(&wc);
    RECT size = {0, 0, 1440, 900};
    AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, FALSE);
    window = CreateWindowA(wc.lpszClassName, "Parallax - Experimental Sandbox (not Minecraft 26.3)", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                           CW_USEDEFAULT, size.right - size.left, size.bottom - size.top, NULL, NULL,
                           instance, NULL);
    if (!window)
        return 1;
    device = GetDC(window);
    PIXELFORMATDESCRIPTOR pfd = {sizeof(pfd),
                                 1,
                                 PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                                 PFD_TYPE_RGBA,
                                 32,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 24,
                                 8,
                                 0,
                                 PFD_MAIN_PLANE,
                                 0,
                                 0,
                                 0,
                                 0};
    int format = ChoosePixelFormat(device, &pfd);
    if (!format || !SetPixelFormat(device, format, &pfd)) {
        MessageBoxA(window, "Could not create an OpenGL surface.", "Parallax", MB_ICONERROR);
        return 1;
    }
    context = wglCreateContext(device);
    if (!context || !wglMakeCurrent(device, context)) {
        MessageBoxA(window, "OpenGL could not start. Check your graphics driver.", "Parallax", MB_ICONERROR);
        return 1;
    }
    typedef BOOL(WINAPI * SwapInterval)(int);
    SwapInterval swap_interval = (SwapInterval)wglGetProcAddress("wglSwapIntervalEXT");
    if (swap_interval)
        swap_interval(1);
    RAWINPUTDEVICE rid = {1, 2, 0, window};
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    render_init();
    glViewport(0, 0, win_w, win_h);
    ShowWindow(window, smoke_mode ? SW_SHOWNOACTIVATE : show);
    UpdateWindow(window);
    QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER last;
    QueryPerformanceCounter(&last);
    int frames = 0;
    int smoke_actions = 0;
    float repeat_clock = 0;
    char smoke_original_dir[MAX_PATH] = {0};
    int smoke_failures = 0;
    FILE *smoke_report = NULL;
    if (smoke_mode) {
        strcpy_s(smoke_original_dir, sizeof(smoke_original_dir), data_dir);
        snprintf(data_dir, sizeof(data_dir), "%s\\render-test-%lu", smoke_original_dir,
                 GetCurrentProcessId());
        CreateDirectoryA(data_dir, NULL);
        fopen_s(&smoke_report, "render-test-results.txt", "w");
        // Exercise the same immediate-mode button handling used by real clicks.
        mouse_x = win_w / 2;
        mouse_y = (int)(win_h * .25f) + 155;
        mouse_click = 1;
        render_frame(0);
        if (screen != WORLDS)
            smoke_failures++;
        mouse_y = 230;
        mouse_click = 1;
        render_frame(0);
        if (screen != CREATE)
            smoke_failures++;
        strcpy_s(world_name, sizeof(world_name), "Render test");
        strcpy_s(seed_input, sizeof(seed_input), "8675309");
        mouse_y = 490;
        mouse_click = 1;
        render_frame(0);
        if (screen != GAME)
            smoke_failures++;
        mouse_click = 0;
        world.yaw = 25;
        world.pitch = 12;
        debug_hud = 1;
    }
    while (running) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (!running)
            break;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = (float)((double)(now.QuadPart - last.QuadPart) / frequency.QuadPart);
        last = now;
        if (dt > .05f)
            dt = .05f;
        if (status_time > 0)
            status_time -= dt;
        if (screen == GAME) {
            float left = dt;
            while (left > 0) {
                float tick = fminf(left, .0166667f);
                update_game(tick);
                left -= tick;
            }
            auto_save += dt;
            if (auto_save > 60 && !smoke_mode) {
                save_world(active_slot);
                auto_save = 0;
            }
            if (keys[VK_LBUTTON] || keys[VK_RBUTTON]) {
                repeat_clock += dt;
                if (repeat_clock > .23f) {
                    interact(keys[VK_RBUTTON] != 0);
                    repeat_clock = 0;
                }
            } else
                repeat_clock = 0;
        }
        render_frame(dt);
        if (smoke_mode) {
            frames++;
            if (frames == 180) {
                screenshot("smoke-game.bmp");
                proc(window, WM_KEYDOWN, 'E', 0);
                if (screen != INVENTORY)
                    smoke_failures++;
            }
            if (frames == 190) {
                screenshot("smoke-inventory.bmp");
                proc(window, WM_KEYDOWN, VK_ESCAPE, 0);
                proc(window, WM_KEYDOWN, 'L', 0);
                if (screen != ACHIEVEMENTS)
                    smoke_failures++;
            }
            if (frames == 200) {
                screenshot("smoke-achievements.bmp");
                proc(window, WM_KEYDOWN, VK_ESCAPE, 0);
                proc(window, WM_KEYDOWN, VK_ESCAPE, 0);
                if (screen != PAUSE)
                    smoke_failures++;
            }
            if (frames == 202) {
                screenshot("smoke-pause.bmp");
                mouse_x = win_w / 2;
                mouse_y = (int)(win_h * .2f) + 374;
                mouse_click = 1;
                render_frame(0);
                if (screen != MENU)
                    smoke_failures++;
                WorldState info;
                if (!save_info(1, &info) || strcmp(info.name, "Render test") != 0)
                    smoke_failures++;
            }
            if (frames == 210) {
                screenshot("smoke-menu.bmp");
                smoke_actions = smoke_failures == 0;
                running = 0;
            }
        }
        SwapBuffers(device);
        mouse_click = 0;
        if (IsIconic(window))
            Sleep(20);
    }
    capture_mouse(0);
    if (smoke_mode) {
        if (smoke_report) {
            fprintf(smoke_report,
                    "Menu -> world slot -> create -> play -> inventory -> achievements -> pause -> save and "
                    "menu: %s\nFailures: %d\n",
                    smoke_failures ? "FAIL" : "PASS", smoke_failures);
            fclose(smoke_report);
        }
        char temp_path[MAX_PATH];
        snprintf(temp_path, sizeof(temp_path), "%s\\world-1.pxw", data_dir);
        DeleteFileA(temp_path);
        RemoveDirectoryA(data_dir);
        strcpy_s(data_dir, sizeof(data_dir), smoke_original_dir);
    }
    render_shutdown();
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(context);
    ReleaseDC(window, device);
    DestroyWindow(window);
    free(blocks);
    CoUninitialize();
    return smoke_mode && !smoke_actions ? 2 : 0;
}
