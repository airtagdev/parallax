#include "port.h"
#include <wincodec.h>
Resources resources;
static char translation_buffers[8][2048];
static unsigned translation_cursor;
const char *translate(const char *key) {
    char *out = translation_buffers[translation_cursor++ % 8];
    int token = json_get(&resources.language, 0, key);
    if (!json_string(&resources.language, token, out, 2048))
        snprintf(out, 2048, "%s", key);
    return out;
}
Image *image_load(const char *path) {
    for (int i = 0; i < resources.image_count; i++)
        if (strcmp(resources.images[i].path, path) == 0)
            return &resources.images[i];
    if (resources.image_count >= 128)
        return NULL;
    char full[MAX_PATH];
    wchar_t wide[MAX_PATH];
    snprintf(full, sizeof(full), "%s/%s", resources.resources, path);
    MultiByteToWideChar(CP_UTF8, 0, full, -1, wide, MAX_PATH);
    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICFormatConverter *converter = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                                  &IID_IWICImagingFactory, (void **)&factory);
    if (SUCCEEDED(hr))
        hr = IWICImagingFactory_CreateDecoderFromFilename(factory, wide, NULL, GENERIC_READ,
                                                          WICDecodeMetadataCacheOnLoad, &decoder);
    if (SUCCEEDED(hr))
        hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    Image *img = &resources.images[resources.image_count];
    memset(img, 0, sizeof(*img));
    if (SUCCEEDED(hr))
        hr = IWICBitmapFrameDecode_GetSize(frame, &img->width, &img->height);
    if (img->width > 8192 || img->height > 8192 || !img->width || !img->height)
        hr = E_FAIL;
    if (SUCCEEDED(hr))
        hr = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    if (SUCCEEDED(hr))
        hr = IWICFormatConverter_Initialize(converter, (IWICBitmapSource *)frame,
                                            &GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 0,
                                            WICBitmapPaletteTypeCustom);
    if (SUCCEEDED(hr)) {
        img->pixels = malloc((size_t)img->width * img->height * 4);
        if (!img->pixels)
            hr = E_OUTOFMEMORY;
        else
            hr = IWICFormatConverter_CopyPixels(converter, NULL, img->width * 4, img->width * img->height * 4,
                                                img->pixels);
    }
    if (converter)
        IWICFormatConverter_Release(converter);
    if (frame)
        IWICBitmapFrameDecode_Release(frame);
    if (decoder)
        IWICBitmapDecoder_Release(decoder);
    if (factory)
        IWICImagingFactory_Release(factory);
    if (FAILED(hr)) {
        free(img->pixels);
        memset(img, 0, sizeof(*img));
        snprintf(resources.error, sizeof(resources.error), "Missing or invalid 26.3 image: %s", path);
        return NULL;
    }
    strncpy_s(img->path, sizeof(img->path), path, _TRUNCATE);
    glGenTextures(1, &img->texture);
    glBindTexture(GL_TEXTURE_2D, img->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 img->pixels);
    resources.image_count++;
    return img;
}
static int fonts_open(void) {
    char full[MAX_PATH];
    snprintf(full, sizeof(full), "%s/meta/font-atlases.tsv", resources.resources);
    FILE *f = fopen(full, "r");
    if (!f)
        return 0;
    int id, cols, rows, height, ascent;
    char path[512];
    while (fscanf(f, "%d %511s %d %d %d %d", &id, path, &cols, &rows, &height, &ascent) == 6) {
        if (id < 0 || id >= 8 || cols <= 0 || rows <= 0 || height <= 0) {
            fclose(f);
            return 0;
        }
        Image *img = image_load(path);
        if (!img) {
            fclose(f);
            return 0;
        }
        resources.atlases[id] = (FontAtlas){img, cols, rows, height, ascent};
        resources.atlas_count++;
    }
    fclose(f);
    snprintf(full, sizeof(full), "%s/meta/font-glyphs.tsv", resources.resources);
    f = fopen(full, "r");
    if (!f)
        return 0;
    unsigned cp;
    int cell;
    while (fscanf(f, "%u %d %d", &cp, &id, &cell) == 3) {
        if (resources.glyph_count >= 8192 || id < 0 || id >= resources.atlas_count) {
            fclose(f);
            return 0;
        }
        FontAtlas *a = &resources.atlases[id];
        int cw = (int)a->image->width / a->cols, ch = (int)a->image->height / a->rows;
        if (cell < 0 || cell >= a->cols * a->rows) {
            fclose(f);
            return 0;
        }
        int right = 0;
        for (int y = 0; y < ch; y++)
            for (int x = 0; x < cw; x++) {
                size_t pixel =
                    ((size_t)((cell / a->cols) * ch + y) * a->image->width + (cell % a->cols) * cw + x) * 4;
                if (a->image->pixels[pixel + 3] && x + 1 > right)
                    right = x + 1;
            }
        int advance = (int)floorf((float)right * (float)a->height / (float)ch + .5f) + 1;
        resources.glyphs[resources.glyph_count++] = (Glyph){id, cell, advance, cp};
    }
    fclose(f);
    return resources.glyph_count > 95;
}
int resources_open(const char *directory, int graphical) {
    memset(&resources, 0, sizeof(resources));
    strncpy_s(resources.resources, sizeof(resources.resources), directory, _TRUNCATE);
    char full[MAX_PATH];
    snprintf(full, sizeof(full), "%s/meta/import.json", directory);
    if (!json_file(&resources.metadata, full) ||
        !json_equal(&resources.metadata, json_get(&resources.metadata, 0, "target"), "26.3") ||
        json_number(&resources.metadata, json_get(&resources.metadata, 0, "data_version"), 0) != 5023) {
        snprintf(
            resources.error, sizeof(resources.error),
            "26.3 resources are not prepared. Run scripts/import-assets.ps1 with your owned installation.");
        return 0;
    }
    snprintf(full, sizeof(full), "%s/assets/minecraft/lang/en_us.json", directory);
    if (!json_file(&resources.language, full)) {
        strcpy_s(resources.error, sizeof(resources.error), "The 26.3 English language resource is missing.");
        return 0;
    }
    if (graphical && !fonts_open()) {
        if (!resources.error[0])
            strcpy_s(resources.error, sizeof(resources.error),
                     "Could not load the vanilla bitmap font providers.");
        return 0;
    }
    if (graphical) {
        const char *required[] = {"gui/title/minecraft",
                                  "gui/title/edition",
                                  "gui/title/background/panorama_overlay",
                                  "gui/sprites/widget/button",
                                  "gui/sprites/widget/button_highlighted",
                                  "gui/sprites/icon/language",
                                  "gui/sprites/icon/accessibility"};
        for (size_t i = 0; i < sizeof(required) / sizeof(required[0]); i++) {
            char path[256];
            snprintf(path, sizeof(path), "assets/minecraft/textures/%s.png", required[i]);
            if (!image_load(path))
                return 0;
        }
        for (int face = 0; face < 6; face++) {
            char path[256];
            snprintf(path, sizeof(path), "assets/minecraft/textures/gui/title/background/panorama_%d.png",
                     face);
            Image *image = image_load(path);
            if (!image)
                return 0;
            if (image->width < 16 || image->height < 16) {
                strcpy_s(resources.error, sizeof(resources.error),
                         "The full title panorama is missing from your local asset cache. A JAR placeholder "
                         "cannot substitute for it.");
                return 0;
            }
        }
    }
    return 1;
}
void resources_close(void) {
    for (int i = 0; i < resources.image_count; i++) {
        if (resources.images[i].texture)
            glDeleteTextures(1, &resources.images[i].texture);
        free(resources.images[i].pixels);
    }
    json_free(&resources.metadata);
    json_free(&resources.language);
}
static void catalog_directory(const char *relative, int advancements) {
    char wildcard[MAX_PATH];
    snprintf(wildcard, sizeof(wildcard), "%s/%s/*", resources.resources, relative);
    WIN32_FIND_DATAA item;
    HANDLE find = FindFirstFileA(wildcard, &item);
    if (find == INVALID_HANDLE_VALUE)
        return;
    do {
        if (item.cFileName[0] == '.')
            continue;
        char child[MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", relative, item.cFileName);
        if (item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            catalog_directory(child, advancements);
            continue;
        }
        size_t len = strlen(item.cFileName);
        if (len < 5 || strcmp(item.cFileName + len - 5, ".json"))
            continue;
        char full[MAX_PATH];
        snprintf(full, sizeof(full), "%s/%s", resources.resources, child);
        Json d;
        if (!json_file(&d, full)) {
            resources.missing_trigger_definitions++;
            json_free(&d);
            continue;
        }
        if (!advancements)
            resources.recipes++;
        else {
            resources.advancements++;
            if (json_get(&d, 0, "display") >= 0)
                resources.visible_advancements++;
            int criteria = json_get(&d, 0, "criteria");
            if (criteria < 0 || d.tokens[criteria].kind != JSON_OBJECT)
                resources.missing_trigger_definitions++;
            else
                for (int key = criteria + 1; key < d.tokens[criteria].end;) {
                    int v = key + 1;
                    resources.criteria++;
                    if (json_get(&d, v, "trigger") < 0)
                        resources.missing_trigger_definitions++;
                    key = d.tokens[v].end;
                }
        }
        json_free(&d);
    } while (FindNextFileA(find, &item));
    FindClose(find);
}
int verify_resource_catalog(void) {
    catalog_directory("data/minecraft/recipe", 0);
    catalog_directory("data/minecraft/advancement", 1);
    int counts = json_get(&resources.metadata, 0, "counts");
    int expected_recipes =
            (int)json_number(&resources.metadata, json_get(&resources.metadata, counts, "recipes"), -1),
        expected_advancements =
            (int)json_number(&resources.metadata, json_get(&resources.metadata, counts, "advancements"), -1);
    return resources.recipes == expected_recipes && resources.advancements == expected_advancements &&
           resources.missing_trigger_definitions == 0;
}
