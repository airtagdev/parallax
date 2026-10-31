#include "port.h"
int window_width = 1440, window_height = 900, mouse_x, mouse_y, click;
HWND main_window;
static int running = 1;
static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM w, LPARAM l) {
    switch (message) {
    case WM_CLOSE:
        running = 0;
        return 0;
    case WM_DESTROY:
        running = 0;
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        window_width = max(1, LOWORD(l));
        window_height = max(1, HIWORD(l));
        if (wglGetCurrentContext())
            glViewport(0, 0, window_width, window_height);
        return 0;
    case WM_MOUSEMOVE:
        mouse_x = (short)LOWORD(l);
        mouse_y = (short)HIWORD(l);
        return 0;
    case WM_LBUTTONDOWN:
        click = 1;
        return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO *limits = (MINMAXINFO *)l;
        limits->ptMinTrackSize.x = 336;
        limits->ptMinTrackSize.y = 279;
        return 0;
    }
    }
    return DefWindowProcA(window, message, w, l);
}
void bmp_capture(const char *path) {
    size_t stride = ((size_t)window_width * 3 + 3) & ~(size_t)3, size = stride * window_height;
    unsigned char *pixels = calloc(1, size);
    if (!pixels)
        return;
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, window_width, window_height, 0x80e0, GL_UNSIGNED_BYTE, pixels);
    BITMAPFILEHEADER file = {0};
    BITMAPINFOHEADER info = {0};
    file.bfType = 0x4d42;
    file.bfOffBits = sizeof(file) + sizeof(info);
    file.bfSize = file.bfOffBits + (DWORD)size;
    info.biSize = sizeof(info);
    info.biWidth = window_width;
    info.biHeight = window_height;
    info.biPlanes = 1;
    info.biBitCount = 24;
    info.biSizeImage = (DWORD)size;
    FILE *out = fopen(path, "wb");
    if (out) {
        fwrite(&file, sizeof(file), 1, out);
        fwrite(&info, sizeof(info), 1, out);
        fwrite(pixels, 1, size, out);
        fclose(out);
    }
    free(pixels);
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR args, int show) {
    (void)previous;
    SetProcessDPIAware();
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    char directory[MAX_PATH];
    GetModuleFileNameA(NULL, directory, MAX_PATH);
    char *slash = strrchr(directory, '\\');
    if (slash)
        *slash = 0;
    strcat_s(directory, sizeof(directory), "/resources");
    int verify = strstr(args, "--verify-resources") != NULL,
        capture = strstr(args, "--capture-title") != NULL;
    if (verify) {
        int ok = resources_open(directory, 0) && verify_resource_catalog();
        FILE *report = fopen("resource-verification.json", "w");
        if (report) {
            fprintf(report,
                    "{\"target\":\"26.3\",\"parsed_recipes\":%d,\"parsed_advancements\":%d,\"displayed_"
                    "advancements\":%d,\"criteria\":%d,\"invalid_definitions\":%d,\"passed\":%s,\"gameplay_"
                    "implemented\":false}\n",
                    resources.recipes, resources.advancements, resources.visible_advancements,
                    resources.criteria, resources.missing_trigger_definitions, ok ? "true" : "false");
            fclose(report);
        }
        resources_close();
        CoUninitialize();
        return ok ? 0 : 1;
    }
    WNDCLASSA wc = {0};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "Parallax26_3Rewrite";
    RegisterClassA(&wc);
    RECT area = {0, 0, 1440, 900};
    AdjustWindowRect(&area, WS_OVERLAPPEDWINDOW, FALSE);
    main_window =
        CreateWindowA(wc.lpszClassName, "Parallax | Minecraft 26.3 C rewrite | Incomplete reference build",
                      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, area.right - area.left,
                      area.bottom - area.top, NULL, NULL, instance, NULL);
    if (!main_window)
        return 1;
    HDC dc = GetDC(main_window);
    PIXELFORMATDESCRIPTOR pixel = {0};
    pixel.nSize = sizeof(pixel);
    pixel.nVersion = 1;
    pixel.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixel.iPixelType = PFD_TYPE_RGBA;
    pixel.cColorBits = 32;
    pixel.cDepthBits = 24;
    pixel.iLayerType = PFD_MAIN_PLANE;
    int format = ChoosePixelFormat(dc, &pixel);
    if (!format || !SetPixelFormat(dc, format, &pixel))
        return 1;
    HGLRC context = wglCreateContext(dc);
    if (!context || !wglMakeCurrent(dc, context))
        return 1;
    glViewport(0, 0, window_width, window_height);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (!resources_open(directory, 1)) {
        if (!capture)
            MessageBoxA(main_window, resources.error, "Parallax - resource preparation required",
                        MB_ICONERROR);
        resources_close();
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(context);
        ReleaseDC(main_window, dc);
        DestroyWindow(main_window);
        CoUninitialize();
        return 1;
    }
    ShowWindow(main_window, capture ? SW_SHOWNOACTIVATE : show);
    UpdateWindow(main_window);
    LARGE_INTEGER frequency, start, now;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    int frames = 0;
    while (running) {
        MSG message;
        while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT)
                running = 0;
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        if (!running)
            break;
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / frequency.QuadPart;
        running = title_frame(elapsed, capture);
        click = 0;
        if (capture && ++frames == 3) {
            bmp_capture("rewrite-title.bmp");
            running = 0;
        }
        SwapBuffers(dc);
        Sleep(8);
    }
    int ok = resources.error[0] == 0;
    resources_close();
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(context);
    ReleaseDC(main_window, dc);
    DestroyWindow(main_window);
    CoUninitialize();
    return ok ? 0 : 1;
}
