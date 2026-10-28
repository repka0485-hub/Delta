#include "Features.h"   // первым (UmbrellaMenu.h тянет imgui.h с math operators)
#include "backends/imgui_impl_opengl3.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO_FAILURE_STRINGS
#include "stb_image.h"

#include "surface.h"

static volatile sig_atomic_t g_run = 1;
static void on_sig(int) { g_run = 0; }

// ---- платформенный слой меню: GLES текстуры + stb_image ----
static bool tex_rgba(const unsigned char* px, int w, int h, umb::Texture& t) {
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    t.id = (ImTextureID)(intptr_t)id;
    t.native = (void*)(intptr_t)id;
    t.w = w; t.h = h;
    return true;
}
static bool tex_file(const char* path, umb::Texture& t) {
    int w, h, c;
    unsigned char* px = stbi_load(path, &w, &h, &c, 4);
    if (!px) return false;
    bool ok = tex_rgba(px, w, h, t);
    stbi_image_free(px);
    return ok;
}
static void tex_release(umb::Texture& t) {
    if (!t.native) return;
    GLuint id = (GLuint)(intptr_t)t.native;
    glDeleteTextures(1, &id);
    t = umb::Texture();
}
// на телефоне диалога нет: кнопка "папка" берёт /data/local/tmp/bg.png
static std::string bg_default() {
    return access("/data/local/tmp/bg.png", R_OK) == 0 ? "/data/local/tmp/bg.png" : "";
}

static ImFont* try_font(const char* const* paths, float px) {
    for (; *paths; ++paths)
        if (access(*paths, R_OK) == 0)
            if (ImFont* f = ImGui::GetIO().Fonts->AddFontFromFileTTF(*paths, px)) return f;
    return nullptr;
}

int main(int argc, char** argv) {
    const char* pkg = argc > 1 ? argv[1] : "FILL.package.name";   // FILL
    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);

    while (g_run && !esp_init(pkg)) sleep(1);
    if (!g_run) return 0;

    int W = 0, H = 0;
    ANativeWindow* win = create_overlay_window(&W, &H);
    if (!win || W <= 0 || H <= 0) { fprintf(stderr, "no overlay window\n"); return 1; }

    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(dpy, nullptr, nullptr);
    const EGLint cfg_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE };
    EGLConfig cfg; EGLint n = 0;
    if (!eglChooseConfig(dpy, cfg_attr, &cfg, 1, &n) || n < 1) { fprintf(stderr, "no egl cfg\n"); return 1; }
    const EGLint ctx_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctx_attr);
    EGLSurface surf = eglCreateWindowSurface(dpy, cfg, win, nullptr);
    if (ctx == EGL_NO_CONTEXT || surf == EGL_NO_SURFACE || !eglMakeCurrent(dpy, surf, surf, ctx)) {
        fprintf(stderr, "egl init failed\n"); return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui_ImplOpenGL3_Init("#version 300 es");

    static const char* reg[]  = { "/system/fonts/Roboto-Regular.ttf", "/system/fonts/RobotoStatic-Regular.ttf", "/system/fonts/DroidSans.ttf", nullptr };
    static const char* bold[] = { "/system/fonts/Roboto-Bold.ttf", "/system/fonts/RobotoStatic-Bold.ttf", "/system/fonts/DroidSans-Bold.ttf", nullptr };
    ImFont* fr = try_font(reg, 18.f);
    ImFont* fb = try_font(bold, 18.f);
    if (!fr) fr = io.Fonts->AddFontDefault();

    umb::Platform plat;
    plat.loadFile        = tex_file;
    plat.createRGBA      = tex_rgba;
    plat.release         = tex_release;
    plat.openImageDialog = bg_default;

    umb::Menu menu;
    feat::Register(menu);
    menu.Init(plat, fr, fb);
    menu.SetScale(170.f);                      // размер под телефон, меняется в Settings

    bool was_down = false;
    while (g_run) {
        float tx = 0, ty = 0; bool down = false;
        poll_touch(&tx, &ty, &down);
        if (down) {
            io.AddMousePosEvent(tx, ty);
            if (!was_down) io.AddMouseButtonEvent(0, true);
        } else {
            if (was_down) io.AddMouseButtonEvent(0, false);
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);   // палец убран: нет hover
        }
        was_down = down;

        io.DisplaySize = ImVec2((float)W, (float)H);
        io.DeltaTime = 1.f / 60.f;
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        feat::Update(io.DeltaTime);
        menu.Render();
        esp_render((float)W, (float)H);
        ImGui::Render();

        glViewport(0, 0, W, H);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(dpy, surf);
        usleep(16000);
    }

    menu.Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(dpy, surf);
    eglDestroyContext(dpy, ctx);
    eglTerminate(dpy);
    return 0;
}
