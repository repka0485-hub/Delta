#pragma once
// ============================================================================
//  UmbrellaFan - Dear ImGui menu (header-only)
//  ВАЖНО: подключай этот файл ПЕРВЫМ среди imgui-заголовков (нужен IMGUI_DEFINE_MATH_OPERATORS)
//  Работает с Dear ImGui 1.90 - 1.92, C++14 и выше.
// ============================================================================
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace umb {

constexpr float PI = 3.14159265f;

// ---------------------------------------------------------------- types ----
struct Texture {
    ImTextureID id = (ImTextureID)0;
    int w = 0, h = 0;
    void* native = nullptr;
    bool valid() const { return native != nullptr; }
};

// Платформенный слой (реализуется в main.cpp под DirectX11)
struct Platform {
    std::function<bool(const char*, Texture&)>                    loadFile;
    std::function<bool(const unsigned char*, int, int, Texture&)> createRGBA;
    std::function<void(Texture&)>                                 release;
    std::function<std::string()>                                  openImageDialog;
};

enum class Tab { Visual = 0, AimBot, Misc, Settings, Count };
enum class Icon { None, Eye, Crosshair, Cube, Gear, Palette, Image, Smoke, Resize, Bolt, Sparkle, Folder, Reset };

// -------------------------------------------------------------- helpers ----
inline float Sat(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float EaseOut(float t) { t = Sat(t); return 1.f - powf(1.f - t, 3.f); }
inline float Rnd() { return (float)rand() / (float)RAND_MAX; }
inline ImVec4 LerpC(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t), Lerp(a.w, b.w, t));
}
inline float& GS() { static float v = 1.f; return v; }        // текущий масштаб меню
inline float& AnimMul() { static float v = 1.f; return v; }   // скорость анимаций
inline float S(float v) { return v * GS(); }

// цвет с учётом глобальной прозрачности окна (fade in/out)
inline ImU32 Cl(ImVec4 c, float a = 1.f) {
    c.w *= a * ImGui::GetStyle().Alpha;
    return ImGui::ColorConvertFloat4ToU32(c);
}
inline ImVec4 cText() { return ImVec4(0.93f, 0.97f, 0.94f, 1.f); }
inline ImVec4 cDim()  { return ImVec4(0.50f, 0.62f, 0.54f, 1.f); }
inline ImVec4 cOff()  { return ImVec4(0.13f, 0.18f, 0.15f, 1.f); }

inline ImGuiID Sub(ImGuiID id, int k) { return ImHashData(&k, sizeof(k), id); }

// плавная анимация значения к цели (хранится в StateStorage окна)
inline float Anim(ImGuiID id, float target, float speed = 14.f) {
    ImGuiStorage* st = ImGui::GetStateStorage();
    float v = st->GetFloat(id, 0.f);
    float dt = ImGui::GetIO().DeltaTime;
    v += (target - v) * (1.f - expf(-speed * AnimMul() * dt));
    if (fabsf(target - v) < 0.0007f) v = target;
    st->SetFloat(id, v);
    return v;
}

// ---------------------------------------------------------------- icons ----
// Все иконки нарисованы векторно (без шрифтов и картинок)
inline void DrawIcon(ImDrawList* dl, Icon ic, ImVec2 c, float r, ImU32 col, float th = 1.6f) {
    switch (ic) {
    case Icon::Eye: {
        ImVec2 L(c.x - r, c.y), R(c.x + r, c.y);
        dl->AddBezierCubic(L, ImVec2(c.x - r * .5f, c.y - r * .95f), ImVec2(c.x + r * .5f, c.y - r * .95f), R, col, th);
        dl->AddBezierCubic(L, ImVec2(c.x - r * .5f, c.y + r * .95f), ImVec2(c.x + r * .5f, c.y + r * .95f), R, col, th);
        dl->AddCircle(c, r * .38f, col, 0, th);
        dl->AddCircleFilled(c, r * .15f, col);
    } break;
    case Icon::Crosshair: {
        dl->AddCircle(c, r * .62f, col, 0, th);
        dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y - r * .35f), col, th);
        dl->AddLine(ImVec2(c.x, c.y + r), ImVec2(c.x, c.y + r * .35f), col, th);
        dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x - r * .35f, c.y), col, th);
        dl->AddLine(ImVec2(c.x + r, c.y), ImVec2(c.x + r * .35f, c.y), col, th);
        dl->AddCircleFilled(c, r * .12f, col);
    } break;
    case Icon::Cube: {
        ImVec2 p[6];
        for (int k = 0; k < 6; k++) { float a = -PI / 2 + k * PI / 3; p[k] = ImVec2(c.x + cosf(a) * r, c.y + sinf(a) * r); }
        dl->AddPolyline(p, 6, col, ImDrawFlags_Closed, th);
        dl->AddLine(c, p[1], col, th); dl->AddLine(c, p[3], col, th); dl->AddLine(c, p[5], col, th);
    } break;
    case Icon::Gear: {
        for (int k = 0; k < 8; k++) {
            float a = k * PI / 4; ImVec2 d(cosf(a), sinf(a));
            dl->AddLine(c + d * (r * .72f), c + d * r, col, th * 1.9f);
        }
        dl->AddCircle(c, r * .68f, col, 0, th);
        dl->AddCircle(c, r * .27f, col, 0, th);
    } break;
    case Icon::Palette: {
        dl->AddCircle(c, r, col, 0, th);
        for (int k = 0; k < 3; k++) {
            float a = -PI * 0.85f + k * 0.8f;
            dl->AddCircleFilled(ImVec2(c.x + cosf(a) * r * .5f, c.y + sinf(a) * r * .5f), r * .15f, col);
        }
        dl->AddCircleFilled(ImVec2(c.x + r * .25f, c.y + r * .38f), r * .15f, col);
    } break;
    case Icon::Image: {
        dl->AddRect(ImVec2(c.x - r, c.y - r * .8f), ImVec2(c.x + r, c.y + r * .8f), col, r * .25f, 0, th);
        dl->AddCircleFilled(ImVec2(c.x - r * .4f, c.y - r * .22f), r * .17f, col);
        ImVec2 m[5] = { ImVec2(c.x - r * .8f, c.y + r * .5f), ImVec2(c.x - r * .2f, c.y - r * .05f), ImVec2(c.x + r * .2f, c.y + r * .3f),
                        ImVec2(c.x + r * .45f, c.y + r * .05f), ImVec2(c.x + r * .8f, c.y + r * .5f) };
        dl->AddPolyline(m, 5, col, ImDrawFlags_None, th);
    } break;
    case Icon::Smoke: {
        for (int j = -1; j <= 1; j++) {
            ImVec2 pts[9];
            for (int k = 0; k < 9; k++) {
                float u = k / 8.f;
                pts[k] = ImVec2(c.x + j * r * .6f + sinf(u * 6.f + j * 1.7f) * r * .2f, c.y + r - u * 2.f * r);
            }
            dl->AddPolyline(pts, 9, col, ImDrawFlags_None, th);
        }
    } break;
    case Icon::Resize: {
        dl->AddRect(ImVec2(c.x - r * .9f, c.y - r * .9f), ImVec2(c.x + r * .9f, c.y + r * .9f), col, r * .2f, 0, th);
        dl->AddLine(ImVec2(c.x - r * .3f, c.y + r * .3f), ImVec2(c.x + r * .5f, c.y - r * .5f), col, th);
        dl->AddLine(ImVec2(c.x + r * .5f, c.y - r * .5f), ImVec2(c.x + r * .5f, c.y - r * .1f), col, th);
        dl->AddLine(ImVec2(c.x + r * .5f, c.y - r * .5f), ImVec2(c.x + r * .1f, c.y - r * .5f), col, th);
    } break;
    case Icon::Bolt: {
        ImVec2 p[6] = { ImVec2(c.x + r * .15f, c.y - r), ImVec2(c.x - r * .5f, c.y + r * .1f), ImVec2(c.x, c.y + r * .1f),
                        ImVec2(c.x - r * .15f, c.y + r), ImVec2(c.x + r * .5f, c.y - r * .15f), ImVec2(c.x, c.y - r * .15f) };
        dl->AddPolyline(p, 6, col, ImDrawFlags_Closed, th);
    } break;
    case Icon::Sparkle: {
        ImVec2 a[4] = { ImVec2(c.x, c.y - r), ImVec2(c.x + r * .3f, c.y), ImVec2(c.x, c.y + r), ImVec2(c.x - r * .3f, c.y) };
        ImVec2 b[4] = { ImVec2(c.x - r, c.y), ImVec2(c.x, c.y - r * .3f), ImVec2(c.x + r, c.y), ImVec2(c.x, c.y + r * .3f) };
        dl->AddConvexPolyFilled(a, 4, col); dl->AddConvexPolyFilled(b, 4, col);
    } break;
    case Icon::Folder: {
        ImVec2 p[6] = { ImVec2(c.x - r, c.y - r * .6f), ImVec2(c.x - r * .3f, c.y - r * .6f), ImVec2(c.x - r * .05f, c.y - r * .3f),
                        ImVec2(c.x + r, c.y - r * .3f), ImVec2(c.x + r, c.y + r * .7f), ImVec2(c.x - r, c.y + r * .7f) };
        dl->AddPolyline(p, 6, col, ImDrawFlags_Closed, th);
    } break;
    case Icon::Reset: {
        float rr = r * .8f;
        dl->PathArcTo(c, rr, 0.6f, 5.6f);
        dl->PathStroke(col, 0, th);
        float a = 5.6f; ImVec2 e(c.x + cosf(a) * rr, c.y + sinf(a) * rr);
        ImVec2 tg(-sinf(a), cosf(a)), nm(cosf(a), sinf(a));
        dl->AddTriangleFilled(e + tg * (r * .45f), e + nm * (r * .3f), e - nm * (r * .3f), col);
    } break;
    default: break;
    }
}

// ================================================================== MENU ====
class Menu {
public:
    enum class FT { Toggle, SliderF, SliderI, Combo, Color, Button, Custom };

    struct Feature {
        FT type = FT::Toggle;
        std::string label, group;
        Icon icon = Icon::None;
        bool* b = nullptr; float* f = nullptr; int* i = nullptr; float* col = nullptr;
        float mn = 0, mx = 1; std::string fmt = "%.1f";
        std::vector<std::string> items;
        std::function<void()> cb;
        float h = 40;                                        // для Custom
        std::function<void(ImVec2, float)> custom;           // для Custom
    };

    struct Settings {
        float sizePct = 100.f, animSpeed = 1.f;
        int   keyIdx = 0;
        bool  useBg = false; float bgDim = 0.45f;
        bool  smoke = true;  float smokeAmount = 0.65f;
        bool  network = true, aurora = true, logo3d = true;
        float logoSpeed = 1.1f;
        float accent[4] = { 0.16f, 0.80f, 0.40f, 1.f };
        int   preset = 0;
    } cfg;

    bool visible = true;

    // ------------------------------------------------------------ public API
    Menu() { RegisterBuiltin(); }

    void Init(const Platform& p, ImFont* regular, ImFont* bold) {
        plat = p;
        fontR = regular ? regular : ImGui::GetIO().Fonts->Fonts[0];
        fontB = bold ? bold : fontR;
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = false;   // таскаем окно за любое место
        BuildSoftTexture();
        if (plat.loadFile) {
            if (!plat.loadFile("/data/local/tmp/logo.png", logo)) plat.loadFile("logo.png", logo);
        }
    }

    // начальный размер меню в процентах (на телефоне ~170)
    void SetScale(float pct) { cfg.sizePct = pct; scaleTarget = scaleCur = pct / 100.f; }

    void Shutdown() {
        FlushTrash();
        if (plat.release) { plat.release(bg); plat.release(logo); plat.release(soft); }
    }

    // Регистрация функций: сюда подключаются твои фичи (см. Features.h)
    void Toggle(Tab t, const char* group, const char* name, bool* v, Icon ic = Icon::None, std::function<void()> cb = nullptr) {
        Feature f; f.type = FT::Toggle; f.group = group; f.label = name; f.b = v; f.icon = ic; f.cb = cb; Add(t, f);
    }
    void SliderFloat(Tab t, const char* group, const char* name, float* v, float mn, float mx, const char* fmt = "%.1f", Icon ic = Icon::None, std::function<void()> cb = nullptr) {
        Feature f; f.type = FT::SliderF; f.group = group; f.label = name; f.f = v; f.mn = mn; f.mx = mx; f.fmt = fmt; f.icon = ic; f.cb = cb; Add(t, f);
    }
    void SliderInt(Tab t, const char* group, const char* name, int* v, int mn, int mx, const char* fmt = "%d", Icon ic = Icon::None, std::function<void()> cb = nullptr) {
        Feature f; f.type = FT::SliderI; f.group = group; f.label = name; f.i = v; f.mn = (float)mn; f.mx = (float)mx; f.fmt = fmt; f.icon = ic; f.cb = cb; Add(t, f);
    }
    void Combo(Tab t, const char* group, const char* name, int* v, std::vector<std::string> items, Icon ic = Icon::None, std::function<void()> cb = nullptr) {
        Feature f; f.type = FT::Combo; f.group = group; f.label = name; f.i = v; f.items = std::move(items); f.icon = ic; f.cb = cb; Add(t, f);
    }
    void Color(Tab t, const char* group, const char* name, float* rgba4, Icon ic = Icon::None, std::function<void()> cb = nullptr) {
        Feature f; f.type = FT::Color; f.group = group; f.label = name; f.col = rgba4; f.icon = ic; f.cb = cb; Add(t, f);
    }
    void Button(Tab t, const char* group, const char* name, std::function<void()> cb, Icon ic = Icon::None) {
        Feature f; f.type = FT::Button; f.group = group; f.label = name; f.icon = ic; f.cb = cb; Add(t, f);
    }
    void Custom(Tab t, const char* group, float height, std::function<void(ImVec2, float)> draw) {
        Feature f; f.type = FT::Custom; f.group = group; f.h = height; f.custom = draw; Add(t, f);
    }
    // вызывается каждый кадр, даже когда меню скрыто (твоя логика)
    void AddUpdateCallback(std::function<void(float)> fn) { updates.push_back(fn); }

    bool LoadBackground(const std::string& path) {
        Texture t;
        if (plat.loadFile && plat.loadFile(path.c_str(), t)) {
            if (bg.valid()) trash.push_back(bg);
            bg = t; cfg.useBg = true;
            snprintf(bgPath, sizeof(bgPath), "%s", path.c_str());
            char b[96]; snprintf(b, sizeof(b), "Loaded: %dx%d", t.w, t.h); bgStatus = b;
            return true;
        }
        bgStatus = "Could not load image";
        return false;
    }
    void OnFileDropped(const char* path) {
        std::string p(path); std::string e = p.substr(p.find_last_of('.') == std::string::npos ? 0 : p.find_last_of('.'));
        for (auto& ch : e) ch = (char)tolower(ch);
        if (e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".bmp") LoadBackground(p);
    }

    // ------------------------------------------------------------ per frame
    void Render() {
        Tick();
        DrawFab();
        if (visT < 0.01f) return;
        ImGuiIO& io = ImGui::GetIO();
        float dt = Dt();
        scaleCur += (scaleTarget - scaleCur) * (1.f - expf(-10.f * dt * cfg.animSpeed));
        GS() = scaleCur * (0.94f + 0.06f * EaseOut(visT));
        pageT = std::min(pageT + dt * 2.2f * cfg.animSpeed, 1.8f);

        ImVec2 size(BASE_W * GS(), BASE_H * GS());
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - size.x) * .5f, (io.DisplaySize.y - size.y) * .5f), ImGuiCond_Once);

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, EaseOut(visT));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGuiWindowFlags fl = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse |
                              ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin("UmbrellaFan##main", nullptr, fl)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p0 = ImGui::GetWindowPos(), sz = ImGui::GetWindowSize();
            DrawBackground(dl, p0, sz);
            DrawEffects(dl, p0, sz);
            DrawSidebar(dl, p0, sz);
            DrawHeader(dl, p0, sz);
            DrawContent(p0, sz);
        }
        ImGui::End();
        ImGui::PopStyleVar(4);
    }

private:
    // кнопка-логотип, пока меню скрыто: тап открывает меню
    void DrawFab() {
        float a = 1.f - visT;
        if (a < 0.01f) return;
        float fs = S(72);
        ImGui::SetNextWindowPos(ImVec2(S(28), S(28)), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(fs, fs), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, a);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGuiWindowFlags fl = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse |
                              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
        if (ImGui::Begin("##umbfab", nullptr, fl)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p0 = ImGui::GetWindowPos(), c = p0 + ImVec2(fs, fs) * .5f;
            ImGui::SetCursorScreenPos(p0);
            if (ImGui::InvisibleButton("##fab", ImVec2(fs, fs))) { visible = true; pageT = 0.f; }
            float hv = Anim(ImGui::GetItemID(), ImGui::IsItemHovered() ? 1.f : 0.f, 14.f);
            dl->AddCircleFilled(c, fs * .5f, Cl(ImVec4(0.025f, 0.05f, 0.035f, 0.85f)));
            dl->AddCircle(c, fs * .5f, Cl(Acc(.45f + .4f * hv)), 0, S(1.6f));
            DrawLogo3D(dl, c, S(44));
        }
        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    static constexpr float BASE_W = 860.f, BASE_H = 560.f, SIDE_W = 212.f;

    Platform plat;
    ImFont *fontR = nullptr, *fontB = nullptr;
    Texture logo, bg, soft;
    std::vector<Texture> trash;
    std::vector<Feature> feats[(int)Tab::Count];
    std::vector<std::function<void(float)>> updates;
    char bgPath[260] = {};
    std::string bgStatus = "Drop a PNG/JPG onto the window";

    int   tab = 0;
    float pageT = 0.f, indY = -1000.f, visT = 0.f, scaleCur = 1.f, scaleTarget = 1.f;
    float logoAng = 0.f, tiltAmp = 1.f, glow = 0.f;
    bool  keyPrev = false;

    struct Puff { ImVec2 p, v; float life, max, size, rot, rotv, seed; };
    struct Dot { ImVec2 p, v; };
    std::vector<Puff> puffs; std::vector<Dot> dots; std::vector<ImVec2> dotPx;

    static float Dt() { return std::min(ImGui::GetIO().DeltaTime, 0.05f); }
    ImVec4 Acc(float a = 1.f) const { return ImVec4(cfg.accent[0], cfg.accent[1], cfg.accent[2], a); }
    ImVec4 AccL(float mixWhite, float a = 1.f) const {
        return ImVec4(Lerp(cfg.accent[0], 1, mixWhite), Lerp(cfg.accent[1], 1, mixWhite), Lerp(cfg.accent[2], 1, mixWhite), a);
    }
    void Add(Tab t, const Feature& f) { feats[(int)t].push_back(f); }

    void FlushTrash() { if (plat.release) for (auto& t : trash) plat.release(t); trash.clear(); }

    void Tick() {
        float dt = Dt();
        AnimMul() = cfg.animSpeed;
        FlushTrash();
        visT += ((visible ? 1.f : 0.f) - visT) * (1.f - expf(-12.f * dt * cfg.animSpeed));
        if (!visible && visT < 0.01f) visT = 0.f;
        // вращение лого
        if (cfg.logo3d) logoAng += dt * cfg.logoSpeed;
        else { float a = fmodf(logoAng, 2 * PI); if (a > PI) a -= 2 * PI; logoAng = a * expf(-6.f * dt); }
        tiltAmp += ((cfg.logo3d ? 1.f : 0.f) - tiltAmp) * (1.f - expf(-4.f * dt));
        for (auto& cb : updates) cb(dt);
    }

    // ------------------------------------------------------ drawing helpers
    void Txt(ImDrawList* dl, ImFont* f, float sz, ImVec2 p, ImU32 c, const char* t) { dl->AddText(f, sz, p, c, t); }
    ImVec2 TSz(ImFont* f, float sz, const char* t) { return f->CalcTextSizeA(sz, FLT_MAX, 0.f, t); }

    void BuildSoftTexture() {
        const int N = 128; std::vector<unsigned char> px(N * N * 4);
        for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
            float dx = (x + .5f - N / 2) / (N / 2), dy = (y + .5f - N / 2) / (N / 2);
            float r = sqrtf(dx * dx + dy * dy), a = r >= 1.f ? 0.f : (1.f - r);
            a = a * a * (3.f - 2.f * a);
            unsigned char* q = &px[(y * N + x) * 4]; q[0] = q[1] = q[2] = 255; q[3] = (unsigned char)(a * 255.f);
        }
        if (plat.createRGBA) plat.createRGBA(px.data(), N, N, soft);
    }
    void Soft(ImDrawList* dl, ImVec2 c, float rad, ImU32 col) {
        if (soft.valid()) { dl->AddImage(soft.id, ImVec2(c.x - rad, c.y - rad), ImVec2(c.x + rad, c.y + rad), ImVec2(0, 0), ImVec2(1, 1), col); return; }
        unsigned a = (col >> 24) & 0xFF;
        for (int k = 0; k < 4; k++) dl->AddCircleFilled(c, rad * (1.f - k * .22f), (col & 0x00FFFFFF) | ((a / 4) << 24));
    }
    void SoftRot(ImDrawList* dl, ImVec2 c, float rad, float rot, ImU32 col) {
        if (!soft.valid()) { Soft(dl, c, rad, col); return; }
        float cs = cosf(rot), sn = sinf(rot);
        auto P = [&](float x, float y) { return ImVec2(c.x + (x * cs - y * sn) * rad, c.y + (x * sn + y * cs) * rad); };
        dl->AddImageQuad(soft.id, P(-1, -1), P(1, -1), P(1, 1), P(-1, 1), ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), col);
    }
    void StartRipple(ImGuiID id) {
        ImGuiStorage* st = ImGui::GetStateStorage(); ImVec2 m = ImGui::GetIO().MousePos;
        st->SetFloat(Sub(id, 10), m.x); st->SetFloat(Sub(id, 11), m.y); st->SetFloat(Sub(id, 12), (float)ImGui::GetTime());
    }
    void DrawRipple(ImDrawList* dl, ImGuiID id, ImVec2 mn, ImVec2 mx) {
        ImGuiStorage* st = ImGui::GetStateStorage();
        float age = (float)ImGui::GetTime() - st->GetFloat(Sub(id, 12), -10.f);
        if (age < 0.f || age > 0.7f) return;
        float k = EaseOut(age / 0.7f), rad = k * std::max(mx.x - mn.x, mx.y - mn.y) * 0.9f;
        dl->PushClipRect(mn, mx, true);
        dl->AddCircleFilled(ImVec2(st->GetFloat(Sub(id, 10), 0.f), st->GetFloat(Sub(id, 11), 0.f)), rad, Cl(Acc(.22f * (1.f - age / 0.7f))));
        dl->PopClipRect();
    }

    // ------------------------------------------------------- 3D logo (top-left)
    void DrawLogo3D(ImDrawList* dl, ImVec2 c, float size) {
        float t = (float)ImGui::GetTime();
        float tilt = sinf(t * 0.9f) * 0.22f * tiltAmp, ang = logoAng, h = size * .5f, persp = size * 3.2f;
        auto P = [&](float x, float y) {
            float x1 = x * cosf(ang), z = x * sinf(ang) + y * sinf(tilt) * .6f, y1 = y * cosf(tilt);
            float k = 1.f / (1.f - z / persp);
            return ImVec2(c.x + x1 * k, c.y + y1 * k);
        };
        float lit = 0.55f + 0.45f * fabsf(cosf(ang));
        Soft(dl, c, size * 1.3f, Cl(Acc(.30f)));
        if (logo.valid()) {
            dl->AddImageQuad(logo.id, P(-h, -h), P(h, -h), P(h, h), P(-h, h), ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), Cl(ImVec4(lit, lit, lit, 1)));
        } else {   // запасной вариант: нарисованный зонтик
            ImVec2 v[8], m[8];
            for (int k = 0; k < 8; k++) {
                float a0 = -PI / 2 + k * PI / 4, a1 = a0 + PI / 4, am = (a0 + a1) * .5f;
                v[k] = ImVec2(cosf(a0) * h, sinf(a0) * h); (void)a1;
                m[k] = ImVec2(cosf(am) * h * .86f, sinf(am) * h * .86f);
            }
            for (int k = 0; k < 8; k++) {
                int n = (k + 1) % 8;
                ImVec4 col = (k & 1) ? ImVec4(.82f * lit, 1.f * lit, .82f * lit, 1) : ImVec4(.13f * lit, .55f * lit, .10f * lit, 1);
                ImVec2 a = P(0, 0), b = P(v[k].x, v[k].y), mm = P(m[k].x, m[k].y), d = P(v[n].x, v[n].y);
                dl->AddTriangleFilled(a, b, mm, Cl(col)); dl->AddTriangleFilled(a, mm, d, Cl(col));
            }
        }
    }

    // ------------------------------------------------------------ background
    void DrawBackground(ImDrawList* dl, ImVec2 p0, ImVec2 sz) {
        float R = S(14); ImVec2 p1 = p0 + sz;
        dl->AddRectFilled(p0, p1, Cl(ImVec4(0.025f, 0.05f, 0.035f, 0.97f)), R);
        if (cfg.useBg && bg.valid()) {      // своя картинка: заполняет всё окно (cover) и подстраивается под размер
            float wa = sz.x / sz.y, ia = (float)bg.w / (float)bg.h;
            ImVec2 uv0(0, 0), uv1(1, 1);
            if (ia > wa) { float u = wa / ia; uv0.x = (1 - u) * .5f; uv1.x = 1 - uv0.x; }
            else         { float v = ia / wa; uv0.y = (1 - v) * .5f; uv1.y = 1 - uv0.y; }
            dl->AddImageRounded(bg.id, p0, p1, uv0, uv1, Cl(ImVec4(1, 1, 1, 1)), R);
            dl->AddRectFilled(p0, p1, Cl(ImVec4(0.01f, 0.03f, 0.02f, cfg.bgDim)), R);
        }
    }

    void DrawEffects(ImDrawList* dl, ImVec2 p0, ImVec2 sz) {
        ImGuiIO& io = ImGui::GetIO(); float dt = Dt(), t = (float)ImGui::GetTime();
        ImVec2 m = io.MousePos;
        bool mIn = m.x >= p0.x && m.y >= p0.y && m.x <= p0.x + sz.x && m.y <= p0.y + sz.y;
        dl->PushClipRect(p0 + ImVec2(1, 1), p0 + sz - ImVec2(1, 1), true);

        // 1) aurora: большие мягкие световые пятна
        if (cfg.aurora) for (int k = 0; k < 3; k++) {
            ImVec2 c(p0.x + sz.x * (.5f + .35f * sinf(t * .21f + k * 2.1f)), p0.y + sz.y * (.5f + .35f * cosf(t * .17f + k * 1.7f)));
            ImVec4 col = (k == 1) ? ImVec4(0.10f, 0.70f, 0.65f, 1) : Acc();
            Soft(dl, c, std::max(sz.x, sz.y) * (.55f + .1f * k), Cl(ImVec4(col.x, col.y, col.z, .11f)));
        }

        // 2) сеть частиц
        if (cfg.network) {
            if (dots.empty()) for (int i = 0; i < 46; i++) dots.push_back({ ImVec2(Rnd(), Rnd()), ImVec2((Rnd() - .5f) * .035f, (Rnd() - .5f) * .035f) });
            dotPx.resize(dots.size());
            for (size_t i = 0; i < dots.size(); i++) {
                Dot& d = dots[i]; d.p.x += d.v.x * dt; d.p.y += d.v.y * dt;
                if (d.p.x < 0 || d.p.x > 1) d.v.x = -d.v.x;
                if (d.p.y < 0 || d.p.y > 1) d.v.y = -d.v.y;
                dotPx[i] = ImVec2(p0.x + d.p.x * sz.x, p0.y + d.p.y * sz.y);
            }
            float L = S(115);
            for (size_t i = 0; i < dotPx.size(); i++) {
                for (size_t j = i + 1; j < dotPx.size(); j++) {
                    float dx = dotPx[i].x - dotPx[j].x, dy = dotPx[i].y - dotPx[j].y, d = sqrtf(dx * dx + dy * dy);
                    if (d < L) dl->AddLine(dotPx[i], dotPx[j], Cl(AccL(.2f, .20f * (1.f - d / L))), 1.f);
                }
                if (mIn) {
                    float dx = dotPx[i].x - m.x, dy = dotPx[i].y - m.y, d = sqrtf(dx * dx + dy * dy);
                    if (d < S(150)) dl->AddLine(dotPx[i], m, Cl(AccL(.4f, .35f * (1.f - d / S(150)))), 1.f);
                }
                dl->AddCircleFilled(dotPx[i], S(1.7f), Cl(AccL(.4f, .55f)));
            }
        }

        // 3) дым, который втягивается курсором и светится рядом с ним
        if (cfg.smoke) {
            int want = (int)(14 + cfg.smokeAmount * 56);
            while ((int)puffs.size() < want) puffs.push_back(NewPuff(sz, true));
            while ((int)puffs.size() > want) puffs.pop_back();
            float R = S(280), absorb = 0.f, density = 0.f;
            for (auto& pf : puffs) {
                pf.life += dt;
                float tt = t * .35f + pf.seed;
                pf.v.x += sinf(tt * 1.7f) * 7.f * dt;
                pf.v.y += cosf(tt * 1.3f) * 4.f * dt - 5.f * dt;
                float lit = 0.f;
                if (mIn) {
                    float dx = m.x - (p0.x + pf.p.x), dy = m.y - (p0.y + pf.p.y);
                    float dist = sqrtf(dx * dx + dy * dy) + 0.001f;
                    if (dist < R) {
                        lit = 1.f - dist / R;
                        float nx = dx / dist, ny = dy / dist, pull = (260.f * lit * lit + 30.f * lit) * GS(), swirl = 60.f * lit * GS();
                        pf.v.x += (nx * pull - ny * swirl) * dt; pf.v.y += (ny * pull + nx * swirl) * dt;
                        if (dist < S(30)) { pf.life += dt * 4.f; absorb += dt * (1.f - dist / S(30)); }   // впитывается
                        density += lit;
                    }
                }
                float damp = expf(-.55f * dt); pf.v.x *= damp; pf.v.y *= damp;
                float sp = sqrtf(pf.v.x * pf.v.x + pf.v.y * pf.v.y), mx = 260.f * GS();
                if (sp > mx) { pf.v.x *= mx / sp; pf.v.y *= mx / sp; }
                pf.p.x += pf.v.x * dt; pf.p.y += pf.v.y * dt; pf.rot += pf.rotv * dt;

                float f = Sat(pf.life / pf.max), fade = sinf(PI * f);
                float alpha = fade * (.07f + .05f * cfg.smokeAmount) * (1.f + lit * 2.4f);
                ImVec4 c = LerpC(ImVec4(.55f, .78f, .62f, 1), AccL(.35f), Sat(lit * 1.4f));
                SoftRot(dl, ImVec2(p0.x + pf.p.x, p0.y + pf.p.y), pf.size * (.8f + .6f * f), pf.rot, Cl(ImVec4(c.x, c.y, c.z, alpha)));

                if (pf.life > pf.max || pf.p.y < -S(200) || pf.p.x < -S(250) || pf.p.x > sz.x + S(250) || pf.p.y > sz.y + S(250))
                    pf = NewPuff(sz, false);
            }
            float tgt = Sat(absorb * 5.f + density * .012f);
            glow += (tgt - glow) * (1.f - expf(-5.f * dt));
            if (mIn) {   // свечение курсора
                Soft(dl, m, S(50) + glow * S(120), Cl(Acc(.10f + .40f * glow)));
                Soft(dl, m, S(24) + glow * S(30), Cl(AccL(.6f, .10f + .35f * glow)));
            }
        }
        dl->PopClipRect();

        // световая полоска по верхнему краю + рамка
        float cx = p0.x + (fmodf(t * .18f, 1.3f) - .15f) * sz.x, hw = S(170);
        dl->PushClipRect(p0, ImVec2(p0.x + sz.x, p0.y + S(4)), true);
        ImU32 z = Cl(Acc(0.f)), a = Cl(Acc(.9f));
        dl->AddRectFilledMultiColor(ImVec2(cx - hw, p0.y), ImVec2(cx, p0.y + S(2)), z, a, a, z);
        dl->AddRectFilledMultiColor(ImVec2(cx, p0.y), ImVec2(cx + hw, p0.y + S(2)), a, z, z, a);
        dl->PopClipRect();
        dl->AddRect(p0, p0 + sz, Cl(Acc(.30f)), S(14), 0, S(1.2f));
    }

    Puff NewPuff(ImVec2 sz, bool initial) {
        Puff p; p.size = S(70.f + Rnd() * 90.f); p.max = 7.f + Rnd() * 8.f; p.seed = Rnd() * 100.f;
        p.rot = Rnd() * 6.28f; p.rotv = (Rnd() - .5f) * .4f;
        p.v = ImVec2((Rnd() - .5f) * 16.f, -(12.f + Rnd() * 26.f));
        if (initial) { p.p = ImVec2(Rnd() * sz.x, Rnd() * sz.y); p.life = Rnd() * p.max; }
        else {
            int e = rand() % 3; p.life = 0.f;
            if (e == 0) p.p = ImVec2(Rnd() * sz.x, sz.y + p.size * .4f);
            else if (e == 1) { p.p = ImVec2(-p.size * .3f, sz.y * (.3f + Rnd() * .7f)); p.v.x += 12.f; }
            else { p.p = ImVec2(sz.x + p.size * .3f, sz.y * (.3f + Rnd() * .7f)); p.v.x -= 12.f; }
        }
        return p;
    }

    // --------------------------------------------------------------- sidebar
    void DrawSidebar(ImDrawList* dl, ImVec2 p0, ImVec2 sz) {
        float sw = S(SIDE_W), dt = Dt();
        dl->AddRectFilled(p0, ImVec2(p0.x + sw, p0.y + sz.y), Cl(ImVec4(0.0f, 0.02f, 0.01f, 0.58f)), S(14), ImDrawFlags_RoundCornersLeft);
        dl->AddLine(ImVec2(p0.x + sw, p0.y + S(16)), ImVec2(p0.x + sw, p0.y + sz.y - S(16)), Cl(ImVec4(1, 1, 1, .06f)));

        // логотип (вращается в 3D) + название
        DrawLogo3D(dl, ImVec2(p0.x + S(44), p0.y + S(48)), S(52));
        ImVec2 tp(p0.x + S(82), p0.y + S(26));
        Txt(dl, fontB, S(21), tp, Cl(cText()), "Umbrella");
        float uw = TSz(fontB, S(21), "Umbrella").x;
        Txt(dl, fontB, S(21), ImVec2(tp.x + uw, tp.y), Cl(AccL(.15f)), "Fan");
        float pulse = .5f + .5f * sinf((float)ImGui::GetTime() * 3.f);
        ImVec2 dc(p0.x + S(88), p0.y + S(60));
        Soft(dl, dc, S(9), Cl(Acc(.5f * pulse)));
        dl->AddCircleFilled(dc, S(3), Cl(Acc()));
        Txt(dl, fontR, S(12.5f), ImVec2(p0.x + S(98), p0.y + S(53)), Cl(cDim()), "offline build");

        static const char* names[] = { "Visual", "AimBot", "Misc", "Settings" };
        static const Icon icons[] = { Icon::Eye, Icon::Crosshair, Icon::Cube, Icon::Gear };
        float th = S(44), gap = S(6), x = p0.x + S(12), w = sw - S(24), yTop = p0.y + S(108);
        auto tabY = [&](int i) { return i == 3 ? p0.y + sz.y - S(66) - th : yTop + i * (th + gap); };

        // скользящий индикатор выбранной вкладки
        float target = (tabY(tab) - p0.y) / GS();
        if (indY < -500.f) indY = target;
        indY += (target - indY) * (1.f - expf(-16.f * dt * cfg.animSpeed));
        float iy = p0.y + indY * GS();
        dl->AddRectFilled(ImVec2(x, iy), ImVec2(x + w, iy + th), Cl(Acc(.13f)), S(10));
        dl->AddRect(ImVec2(x, iy), ImVec2(x + w, iy + th), Cl(Acc(.28f)), S(10));
        Soft(dl, ImVec2(x + S(2), iy + th * .5f), S(26), Cl(Acc(.35f)));
        dl->AddRectFilled(ImVec2(x, iy + S(10)), ImVec2(x + S(3), iy + th - S(10)), Cl(Acc()), S(2));

        for (int i = 0; i < 4; i++) {
            ImVec2 tp0(x, tabY(i));
            ImGui::PushID(i);
            ImGui::SetCursorScreenPos(tp0);
            bool pr = ImGui::InvisibleButton("tab", ImVec2(w, th));
            ImGuiID id = ImGui::GetItemID(); bool hov = ImGui::IsItemHovered();
            if (pr) { StartRipple(id); if (tab != i) { tab = i; pageT = 0.f; } }
            float sel = Anim(Sub(id, 1), tab == i ? 1.f : 0.f, 12.f), hv = Anim(Sub(id, 2), hov ? 1.f : 0.f, 14.f);
            dl->AddRectFilled(tp0, tp0 + ImVec2(w, th), Cl(ImVec4(1, 1, 1, .045f * hv * (1.f - sel))), S(10));
            DrawRipple(dl, id, tp0, tp0 + ImVec2(w, th));
            float off = S(3) * hv + S(3) * sel;
            ImVec4 ic = LerpC(cDim(), AccL(.1f), Sat(sel + hv * .5f));
            DrawIcon(dl, icons[i], ImVec2(tp0.x + S(28) + off, tp0.y + th * .5f), S(9.5f), Cl(ic), S(1.7f));
            Txt(dl, fontR, S(16), ImVec2(tp0.x + S(52) + off, tp0.y + th * .5f - S(8.5f)), Cl(LerpC(cDim(), cText(), Sat(sel + hv * .6f))), names[i]);
            ImGui::PopID();
        }

        // футер
        ImVec2 fp(x, p0.y + sz.y - S(54)), fs(w, S(40));
        dl->AddRectFilled(fp, fp + fs, Cl(ImVec4(1, 1, 1, .04f)), S(10));
        dl->AddRect(fp, fp + fs, Cl(ImVec4(1, 1, 1, .06f)), S(10));
        Txt(dl, fontR, S(13), ImVec2(fp.x + S(12), fp.y + S(12)), Cl(cDim()), "Offline mode");
        char fps[24]; snprintf(fps, sizeof(fps), "%.0f FPS", ImGui::GetIO().Framerate);
        ImVec2 fz = TSz(fontB, S(13), fps);
        Txt(dl, fontB, S(13), ImVec2(fp.x + fs.x - fz.x - S(12), fp.y + S(12)), Cl(AccL(.1f)), fps);
    }

    void DrawHeader(ImDrawList* dl, ImVec2 p0, ImVec2 sz) {
        static const char* names[] = { "Visual", "AimBot", "Misc", "Settings" };
        static const char* subs[] = { "Rendering options", "Targeting options", "Miscellaneous options", "Menu, theme & effects" };
        float x0 = p0.x + S(SIDE_W) + S(24);
        Txt(dl, fontB, S(25), ImVec2(x0, p0.y + S(22)), Cl(cText()), names[tab]);
        Txt(dl, fontR, S(13.5f), ImVec2(x0, p0.y + S(52)), Cl(cDim()), subs[tab]);
        const char* hint = "HIDE";
        ImVec2 hs = TSz(fontB, S(13), hint);
        ImVec2 hsz(hs.x + S(30), S(34)), hp(p0.x + sz.x - hsz.x - S(20), p0.y + S(20));
        ImGui::SetCursorScreenPos(hp);
        bool hpr = ImGui::InvisibleButton("##hide", hsz);
        ImGuiID hid = ImGui::GetItemID();
        if (hpr) { StartRipple(hid); visible = false; }
        float hhv = Anim(Sub(hid, 1), ImGui::IsItemHovered() ? 1.f : 0.f, 14.f);
        dl->AddRectFilled(hp, hp + hsz, Cl(Acc(.12f + .25f * hhv)), hsz.y * .5f);
        dl->AddRect(hp, hp + hsz, Cl(Acc(.35f + .4f * hhv)), hsz.y * .5f);
        DrawRipple(dl, hid, hp, hp + hsz);
        Txt(dl, fontB, S(13), ImVec2(hp.x + S(15), hp.y + (hsz.y - hs.y) * .5f), Cl(AccL(.2f)), hint);
    }

    // --------------------------------------------------------------- content
    float RowH(const Feature& f) const {
        switch (f.type) {
        case FT::Toggle: return 36; case FT::SliderF: case FT::SliderI: return 52; case FT::Combo: return 40;
        case FT::Color: return 38; case FT::Button: return 44; default: return f.h;
        }
    }

    void DrawContent(ImVec2 p0, ImVec2 sz) {
        float x0 = S(SIDE_W) + S(22), y0 = S(88);
        ImVec2 csz(sz.x - x0 - S(16), sz.y - y0 - S(16));
        ImGui::SetCursorScreenPos(p0 + ImVec2(x0, y0));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, S(5));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, S(3));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, Acc(.45f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, Acc(.7f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, Acc(.9f));
        ImGui::BeginChild("##content", csz, 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetWindowFontScale(GS());
        ImDrawList* dl = ImGui::GetWindowDrawList();
        std::vector<Feature>& fl = feats[tab];
        ImVec2 origin = ImGui::GetCursorScreenPos();

        if (fl.empty()) {
            static const Icon icons[] = { Icon::Eye, Icon::Crosshair, Icon::Cube, Icon::Gear };
            float e = EaseOut(pageT), bob = sinf((float)ImGui::GetTime() * 1.6f) * S(4);
            ImVec2 c(origin.x + csz.x * .5f - S(8), origin.y + csz.y * .5f - S(30) + bob);
            Soft(dl, c, S(80), Cl(Acc(.25f * e)));
            DrawIcon(dl, icons[tab], c, S(26), Cl(AccL(.1f, e)), S(2.2f));
            const char* t1 = "Nothing here yet"; const char* t2 = "Register your features in Features.h";
            ImVec2 a = TSz(fontB, S(19), t1), b = TSz(fontR, S(13.5f), t2);
            Txt(dl, fontB, S(19), ImVec2(c.x - a.x * .5f, c.y + S(46)), Cl(cText(), e), t1);
            Txt(dl, fontR, S(13.5f), ImVec2(c.x - b.x * .5f, c.y + S(74)), Cl(cDim(), e), t2);
            ImGui::Dummy(ImVec2(1, 1));
        } else {
            struct G { std::string name; std::vector<int> idx; float h = 0; };
            std::vector<G> groups;
            for (int i = 0; i < (int)fl.size(); i++) {
                G* g = nullptr; for (auto& q : groups) if (q.name == fl[i].group) { g = &q; break; }
                if (!g) { groups.push_back(G()); g = &groups.back(); g->name = fl[i].group; }
                g->idx.push_back(i); g->h += S(RowH(fl[i]));
            }
            for (auto& g : groups) g.h += S(40) + S(8);

            float W = ImGui::GetContentRegionAvail().x, gap = S(14);
            bool two = groups.size() > 1;
            float colW = two ? (W - gap) * .5f : W, colY[2] = { 0, 0 }, baseA = ImGui::GetStyle().Alpha;
            for (int gi = 0; gi < (int)groups.size(); gi++) {
                G& g = groups[gi];
                int c = two ? (colY[0] <= colY[1] ? 0 : 1) : 0;
                ImVec2 gp(origin.x + c * (colW + gap), origin.y + colY[c]);
                colY[c] += g.h + gap;
                float ge = EaseOut(Sat((pageT - gi * .12f) / .7f));
                gp.y += (1.f - ge) * S(16);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, baseA * ge);
                DrawGroup(dl, gp, colW, g.h, g.name, g.idx, fl);
                ImGui::PopStyleVar();
            }
            ImGui::SetCursorScreenPos(origin);
            ImGui::Dummy(ImVec2(1, std::max(colY[0], colY[1]) + S(4)));
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);
    }

    void DrawGroup(ImDrawList* dl, ImVec2 gp, float w, float h, const std::string& name, const std::vector<int>& idx, std::vector<Feature>& fl) {
        float R = S(11);
        dl->AddRectFilled(gp, gp + ImVec2(w, h), Cl(ImVec4(0.0f, 0.03f, 0.015f, 0.58f)), R);
        dl->AddRect(gp, gp + ImVec2(w, h), Cl(ImVec4(1, 1, 1, .06f)), R);
        ImU32 z = Cl(Acc(0.f)), a = Cl(Acc(.45f));
        dl->AddRectFilledMultiColor(ImVec2(gp.x + R, gp.y), ImVec2(gp.x + w * .5f, gp.y + 1), z, a, a, z);
        dl->AddRectFilledMultiColor(ImVec2(gp.x + w * .5f, gp.y), ImVec2(gp.x + w - R, gp.y + 1), a, z, z, a);
        dl->AddRectFilled(ImVec2(gp.x + S(14), gp.y + S(14)), ImVec2(gp.x + S(17), gp.y + S(28)), Cl(Acc()), S(2));
        Txt(dl, fontB, S(15), ImVec2(gp.x + S(26), gp.y + S(12)), Cl(cText()), name.c_str());
        float y = gp.y + S(40);
        for (int i : idx) {
            Feature& f = fl[i];
            Row(dl, f, ImVec2(gp.x + S(6), y), w - S(12), tab * 1000 + i);
            y += S(RowH(f));
        }
    }

    void RowLabel(ImDrawList* dl, ImVec2 p, float cy, const Feature& f, float hv, float act) {
        float x = p.x + S(14);
        if (f.icon != Icon::None) {
            DrawIcon(dl, f.icon, ImVec2(p.x + S(25), p.y + cy), S(8), Cl(LerpC(cDim(), AccL(.1f), Sat(act * .8f + hv * .4f))), S(1.5f));
            x = p.x + S(44);
        }
        ImVec4 c = LerpC(ImVec4(.72f, .80f, .75f, 1), cText(), Sat(hv * .7f + act * .3f));
        Txt(dl, fontR, S(15), ImVec2(x, p.y + cy - S(7.5f)), Cl(c), f.label.c_str());
    }

    void Row(ImDrawList* dl, Feature& f, ImVec2 p, float w, int uid) {
        float rh = S(RowH(f)); bool changed = false;
        ImGuiIO& io = ImGui::GetIO();
        ImGui::PushID(uid);
        switch (f.type) {
        case FT::Toggle: {
            ImGui::SetCursorScreenPos(p);
            bool pr = ImGui::InvisibleButton("r", ImVec2(w, rh));
            ImGuiID id = ImGui::GetItemID(); bool hov = ImGui::IsItemHovered();
            if (pr) { *f.b = !*f.b; changed = true; StartRipple(id); }
            float t = Anim(Sub(id, 1), *f.b ? 1.f : 0.f, 12.f), hv = Anim(Sub(id, 2), hov ? 1.f : 0.f, 14.f);
            dl->AddRectFilled(p, p + ImVec2(w, rh), Cl(ImVec4(1, 1, 1, .04f * hv)), S(8));
            DrawRipple(dl, id, p, p + ImVec2(w, rh));
            RowLabel(dl, p, rh * .5f, f, hv, t);
            ImVec2 swz(S(38), S(20)), sp(p.x + w - swz.x - S(12), p.y + (rh - swz.y) * .5f);
            Soft(dl, sp + swz * .5f, S(34), Cl(Acc(.22f * t)));
            dl->AddRectFilled(sp, sp + swz, Cl(LerpC(cOff(), Acc(), t)), swz.y * .5f);
            dl->AddRect(sp, sp + swz, Cl(ImVec4(1, 1, 1, .08f * (1.f - t))), swz.y * .5f);
            float kr = S(7.f) + S(1.2f) * hv, kx = Lerp(sp.x + S(10), sp.x + swz.x - S(10), t), ky = sp.y + swz.y * .5f;
            dl->AddCircleFilled(ImVec2(kx + S(.5f), ky + S(1)), kr, Cl(ImVec4(0, 0, 0, .25f)));
            dl->AddCircleFilled(ImVec2(kx, ky), kr, Cl(LerpC(ImVec4(.70f, .76f, .72f, 1), ImVec4(1, 1, 1, 1), t)));
        } break;

        case FT::SliderF: case FT::SliderI: {
            bool isInt = f.type == FT::SliderI;
            float v = isInt ? (float)*f.i : *f.f;
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + S(22)));
            ImGui::InvisibleButton("r", ImVec2(w, rh - S(22)));
            ImGuiID id = ImGui::GetItemID(); bool hov = ImGui::IsItemHovered(), act = ImGui::IsItemActive();
            float tx0 = p.x + S(14), tx1 = p.x + w - S(14), ty = p.y + S(38);
            if (act) {
                float nv = f.mn + Sat((io.MousePos.x - tx0) / (tx1 - tx0)) * (f.mx - f.mn);
                if (isInt) nv = roundf(nv);
                if (nv != v) { v = nv; changed = true; if (isInt) *f.i = (int)nv; else *f.f = nv; }
            }
            float frac = (v - f.mn) / (f.mx - f.mn);
            float sh = Anim(Sub(id, 1), frac, 20.f), hv = Anim(Sub(id, 2), (hov || act) ? 1.f : 0.f, 14.f), ac = Anim(Sub(id, 3), act ? 1.f : 0.f, 16.f);
            RowLabel(dl, p, S(13), f, hv, 0.f);
            char buf[48]; if (isInt) snprintf(buf, sizeof(buf), f.fmt.c_str(), (int)v); else snprintf(buf, sizeof(buf), f.fmt.c_str(), v);
            ImVec2 vs = TSz(fontB, S(14), buf);
            Txt(dl, fontB, S(14), ImVec2(tx1 - vs.x, p.y + S(13) - S(7)), Cl(AccL(.1f)), buf);
            float kx = Lerp(tx0, tx1, sh);
            dl->AddRectFilled(ImVec2(tx0, ty - S(3)), ImVec2(tx1, ty + S(3)), Cl(cOff()), S(3));
            dl->AddRectFilled(ImVec2(tx0, ty - S(5)), ImVec2(kx + S(1), ty + S(5)), Cl(Acc(.14f + .12f * hv)), S(5));
            dl->AddRectFilled(ImVec2(tx0, ty - S(3)), ImVec2(kx + S(1), ty + S(3)), Cl(Acc()), S(3));
            float kr = S(6) + S(2) * hv + S(1) * ac;
            Soft(dl, ImVec2(kx, ty), S(22), Cl(Acc(.35f * hv)));
            dl->AddCircleFilled(ImVec2(kx, ty), kr + S(2.5f), Cl(Acc(.35f)));
            dl->AddCircleFilled(ImVec2(kx, ty), kr, Cl(ImVec4(1, 1, 1, 1)));
            if (ac > .01f) {   // всплывающий пузырь со значением
                ImVec2 bs = TSz(fontB, S(13), buf), bc(kx, ty - S(24) - ac * S(3));
                ImVec2 bmin(bc.x - bs.x * .5f - S(7), bc.y - bs.y * .5f - S(4)), bmax(bc.x + bs.x * .5f + S(7), bc.y + bs.y * .5f + S(4));
                dl->AddRectFilled(bmin, bmax, Cl(Acc(), ac), S(7));
                Txt(dl, fontB, S(13), ImVec2(bc.x - bs.x * .5f, bc.y - bs.y * .5f), Cl(ImVec4(.02f, .06f, .03f, 1), ac), buf);
            }
        } break;

        case FT::Combo: {
            RowLabel(dl, p, rh * .5f, f, 0.f, 0.f);
            float cw = S(142);
            ImGui::SetCursorScreenPos(ImVec2(p.x + w - cw - S(12), p.y + (rh - S(28)) * .5f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(8));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(10), S(4)));
            ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, S(8));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(.08f, .13f, .10f, .9f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(.12f, .20f, .15f, .95f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.12f, .20f, .15f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(.04f, .07f, .05f, .97f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, .08f));
            ImGui::PushStyleColor(ImGuiCol_Header, Acc(.35f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Acc(.5f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, Acc(.6f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.10f, .16f, .12f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Acc(.4f));
            ImGui::SetNextItemWidth(cw);
            int cur = std::max(0, std::min(*f.i, (int)f.items.size() - 1));
            if (ImGui::BeginCombo("##c", f.items[cur].c_str())) {
                ImGui::SetWindowFontScale(GS());
                for (int k = 0; k < (int)f.items.size(); k++)
                    if (ImGui::Selectable(f.items[k].c_str(), *f.i == k)) { *f.i = k; changed = true; }
                ImGui::EndCombo();
            }
            ImGui::PopStyleColor(10); ImGui::PopStyleVar(3);
        } break;

        case FT::Color: {
            RowLabel(dl, p, rh * .5f, f, 0.f, 0.f);
            ImGui::SetCursorScreenPos(ImVec2(p.x + w - S(44), p.y + (rh - S(22)) * .5f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(7));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(8), S(3)));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(.04f, .07f, .05f, .97f));
            if (ImGui::ColorEdit4("##c", f.col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_PickerHueWheel)) changed = true;
            ImGui::PopStyleColor(); ImGui::PopStyleVar(2);
        } break;

        case FT::Button: {
            ImVec2 bp(p.x + S(8), p.y + S(5)), bs(w - S(16), rh - S(10));
            ImGui::SetCursorScreenPos(bp);
            bool pr = ImGui::InvisibleButton("r", bs);
            ImGuiID id = ImGui::GetItemID(); bool hov = ImGui::IsItemHovered(), dn = ImGui::IsItemActive();
            if (pr) { changed = true; StartRipple(id); }
            float hv = Anim(Sub(id, 1), hov ? 1.f : 0.f, 14.f), pd = Anim(Sub(id, 2), dn ? 1.f : 0.f, 20.f);
            ImVec2 mn(bp.x + S(1) * pd, bp.y - S(1) * hv + S(1) * pd), mx(bp.x + bs.x - S(1) * pd, bp.y + bs.y - S(1) * hv - S(1) * pd);
            Soft(dl, (mn + mx) * .5f, bs.x * .45f, Cl(Acc(.18f * hv)));
            dl->AddRectFilled(mn, mx, Cl(Acc(.12f + .28f * hv)), S(9));
            dl->AddRect(mn, mx, Cl(Acc(.35f + .45f * hv)), S(9));
            DrawRipple(dl, id, mn, mx);
            ImVec2 ts = TSz(fontB, S(14.5f), f.label.c_str());
            float iw = f.icon != Icon::None ? S(24) : 0.f, tx = (mn.x + mx.x) * .5f - (ts.x + iw) * .5f, cy = (mn.y + mx.y) * .5f;
            if (f.icon != Icon::None) DrawIcon(dl, f.icon, ImVec2(tx + S(8), cy), S(8), Cl(AccL(.3f * hv + .1f)), S(1.6f));
            Txt(dl, fontB, S(14.5f), ImVec2(tx + iw, cy - ts.y * .5f), Cl(LerpC(AccL(.2f), ImVec4(1, 1, 1, 1), hv)), f.label.c_str());
        } break;

        case FT::Custom: if (f.custom) f.custom(p, w); break;
        }
        ImGui::PopID();
        if (changed && f.cb) f.cb();
    }

    // ----------------------------------------------- встроенные настройки
    void ApplyPreset() {
        static const float P[4][3] = { { .16f, .80f, .40f }, { .35f, .95f, .65f }, { .60f, .92f, .20f }, { .10f, .80f, .75f } };
        for (int k = 0; k < 3; k++) cfg.accent[k] = P[cfg.preset][k];
    }

    void DrawBgPathRow(ImVec2 p, float w) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float bw = S(34), bh = S(30);
        ImGui::SetCursorScreenPos(ImVec2(p.x + S(8), p.y + S(4)));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(8));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(10), S(6)));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(.08f, .13f, .10f, .9f));
        ImGui::SetNextItemWidth(w - bw - S(24));
        if (ImGui::InputTextWithHint("##bgpath", "path to .png / .jpg", bgPath, sizeof(bgPath), ImGuiInputTextFlags_EnterReturnsTrue)) LoadBackground(bgPath);
        ImGui::PopStyleColor(); ImGui::PopStyleVar(2);

        ImVec2 bp(p.x + w - bw - S(8), p.y + S(4));
        ImGui::SetCursorScreenPos(bp);
        bool pr = ImGui::InvisibleButton("##browse", ImVec2(bw, bh));
        ImGuiID id = ImGui::GetItemID();
        if (pr) StartRipple(id);
        float hv = Anim(Sub(id, 1), ImGui::IsItemHovered() ? 1.f : 0.f, 14.f);
        dl->AddRectFilled(bp, bp + ImVec2(bw, bh), Cl(Acc(.12f + .3f * hv)), S(8));
        dl->AddRect(bp, bp + ImVec2(bw, bh), Cl(Acc(.4f + .4f * hv)), S(8));
        DrawRipple(dl, id, bp, bp + ImVec2(bw, bh));
        DrawIcon(dl, Icon::Folder, bp + ImVec2(bw, bh) * .5f, S(8), Cl(AccL(.2f + .5f * hv)), S(1.6f));
        if (pr && plat.openImageDialog) { std::string s = plat.openImageDialog(); if (!s.empty()) LoadBackground(s); }
        Txt(dl, fontR, S(12), ImVec2(p.x + S(14), p.y + S(38)), Cl(cDim()), bgStatus.c_str());
    }

    void RegisterBuiltin() {
        Tab T = Tab::Settings;
        SliderFloat(T, "Menu", "Menu size", &cfg.sizePct, 70.f, 260.f, "%.0f%%", Icon::Resize, [this]() { scaleTarget = cfg.sizePct / 100.f; });
        SliderFloat(T, "Menu", "Animation speed", &cfg.animSpeed, 0.5f, 2.f, "%.1fx", Icon::Sparkle);
        Button(T, "Menu", "Reset size", [this]() { cfg.sizePct = 100.f; scaleTarget = 1.f; }, Icon::Reset);

        Toggle(T, "Background", "Custom background", &cfg.useBg, Icon::Image);
        Custom(T, "Background", 58.f, [this](ImVec2 p, float w) { DrawBgPathRow(p, w); });
        SliderFloat(T, "Background", "Image dim", &cfg.bgDim, 0.f, 0.9f, "%.2f");
        Button(T, "Background", "Remove background", [this]() { if (bg.valid()) { trash.push_back(bg); bg = Texture(); } cfg.useBg = false; bgPath[0] = 0; bgStatus = "Drop a PNG/JPG onto the window"; }, Icon::Reset);

        Toggle(T, "Effects", "Smoke + cursor glow", &cfg.smoke, Icon::Smoke);
        SliderFloat(T, "Effects", "Smoke amount", &cfg.smokeAmount, 0.1f, 1.f, "%.2f");
        Toggle(T, "Effects", "Network particles", &cfg.network, Icon::Sparkle);
        Toggle(T, "Effects", "Aurora glow", &cfg.aurora, Icon::Palette);
        Toggle(T, "Effects", "3D rotating logo", &cfg.logo3d, Icon::Cube);
        SliderFloat(T, "Effects", "Logo spin speed", &cfg.logoSpeed, 0.2f, 3.f, "%.1fx");

        Combo(T, "Theme", "Accent preset", &cfg.preset, { "Emerald", "Mint", "Lime", "Teal" }, Icon::Palette, [this]() { ApplyPreset(); });
        Color(T, "Theme", "Accent color", cfg.accent, Icon::Palette);
    }
};

} // namespace umb
