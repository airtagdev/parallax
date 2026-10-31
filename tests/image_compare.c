#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <wincodec.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct {
    UINT width, height;
    unsigned char *rgba;
} Pixels;
static Pixels decode(const char *path) {
    Pixels p = {0};
    wchar_t wide[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wide, MAX_PATH);
    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICFormatConverter *converter = NULL;
    HRESULT result = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                                      &IID_IWICImagingFactory, (void **)&factory);
    if (SUCCEEDED(result))
        result = IWICImagingFactory_CreateDecoderFromFilename(factory, wide, NULL, GENERIC_READ,
                                                              WICDecodeMetadataCacheOnLoad, &decoder);
    if (SUCCEEDED(result))
        result = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    if (SUCCEEDED(result))
        result = IWICBitmapFrameDecode_GetSize(frame, &p.width, &p.height);
    if (p.width > 8192 || p.height > 8192)
        result = E_FAIL;
    if (SUCCEEDED(result))
        result = IWICImagingFactory_CreateFormatConverter(factory, &converter);
    if (SUCCEEDED(result))
        result = IWICFormatConverter_Initialize(converter, (IWICBitmapSource *)frame,
                                                &GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL,
                                                0, WICBitmapPaletteTypeCustom);
    if (SUCCEEDED(result)) {
        p.rgba = malloc((size_t)p.width * p.height * 4);
        if (!p.rgba)
            result = E_OUTOFMEMORY;
        else
            result =
                IWICFormatConverter_CopyPixels(converter, NULL, p.width * 4, p.width * p.height * 4, p.rgba);
    }
    if (converter)
        IWICFormatConverter_Release(converter);
    if (frame)
        IWICBitmapFrameDecode_Release(frame);
    if (decoder)
        IWICBitmapDecoder_Release(decoder);
    if (factory)
        IWICImagingFactory_Release(factory);
    if (FAILED(result)) {
        free(p.rgba);
        p.rgba = NULL;
    }
    return p;
}
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ParallaxImageCompare native-title.png rewrite-title.bmp\n");
        return 2;
    }
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    Pixels a = decode(argv[1]), b = decode(argv[2]);
    int failures = 0;
    if (!a.rgba || !b.rgba || a.width != 1440 || a.height != 900 || a.width != b.width ||
        a.height != b.height) {
        fprintf(stderr, "Both inputs must be 1440 x 900 frames.\n");
        free(a.rgba);
        free(b.rgba);
        CoUninitialize();
        return 2;
    }
    const int regions[][4] = {{420, 369, 600, 60}, {420, 441, 600, 60}, {420, 513, 600, 60},
                              {420, 657, 294, 60}, {726, 657, 294, 60}, {690, 585, 60, 60},
                              {762, 585, 60, 60}};
    const char *names[] = {"Singleplayer button", "Multiplayer button", "Realms button",
                           "Options button",      "Quit button",        "Language button",
                           "Accessibility button"};
    for (int region = 0; region < 7; region++) {
        unsigned mismatches = 0;
        int x = regions[region][0], y = regions[region][1], w = regions[region][2], h = regions[region][3];
        for (int yy = y; yy < y + h; yy++)
            for (int xx = x; xx < x + w; xx++) {
                size_t offset = ((size_t)yy * a.width + xx) * 4;
                if (a.rgba[offset] != b.rgba[offset] || a.rgba[offset + 1] != b.rgba[offset + 1] ||
                    a.rgba[offset + 2] != b.rgba[offset + 2])
                    mismatches++;
            }
        printf("%s: %u / %d pixels differ\n", names[region], mismatches, w * h);
        if (mismatches)
            failures++;
    }
    free(a.rgba);
    free(b.rgba);
    CoUninitialize();
    return failures ? 1 : 0;
}
