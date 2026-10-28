#pragma once
#include <sys/uio.h>
#include <unistd.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <dirent.h>
#include <cstdio>
#include <cstring>
#include "imgui.h"

// ACE-protected title: root + external read-only is the lowest-risk shape, still a ban risk on your account.

namespace off {                       // FILL all, dump from the game's SDK/GObjects
    inline uintptr_t GWorld = 0;              // static offset from libUE4.so base
    inline uintptr_t PersistentLevel = 0;     // UWorld
    inline uintptr_t OwningGameInstance = 0;  // UWorld
    inline uintptr_t LocalPlayers = 0;        // UGameInstance (TArray)
    inline uintptr_t PlayerController = 0;    // UPlayer
    inline uintptr_t AcknowledgedPawn = 0;    // APlayerController
    inline uintptr_t CameraManager = 0;       // APlayerController
    inline uintptr_t CameraPOV = 0;           // APlayerCameraManager -> CameraCachePrivate.POV
    inline uintptr_t Actors = 0;              // ULevel (TArray<AActor*>)
    inline uintptr_t RootComponent = 0;       // AActor
    inline uintptr_t RelLocation = 0;         // USceneComponent
    inline uintptr_t Health = 0;              // character class, float
    inline uintptr_t MaxHealth = 0;
    inline uintptr_t TeamId = 0;
    // FMinimalViewInfo layout: UE5 = doubles, UE4 = floats.
    inline bool ptr64_doubles = false;        // UE4 default; set true for UE5
}

namespace mem {
    inline pid_t pid = 0;
    inline uintptr_t base = 0;

    inline pid_t find_pid(const char* pkg) {
        DIR* d = opendir("/proc"); if (!d) return 0;
        dirent* e; pid_t out = 0;
        while ((e = readdir(d))) {
            int p = atoi(e->d_name); if (p <= 0) continue;
            char path[64], buf[256] = {};
            snprintf(path, sizeof path, "/proc/%d/cmdline", p);
            FILE* f = fopen(path, "r"); if (!f) continue;
            fread(buf, 1, sizeof buf - 1, f); fclose(f);
            if (!strcmp(buf, pkg)) { out = p; break; }
        }
        closedir(d); return out;
    }

    inline uintptr_t find_base(const char* lib) {
        char path[64]; snprintf(path, sizeof path, "/proc/%d/maps", pid);
        FILE* f = fopen(path, "r"); if (!f) return 0;
        char line[512]; uintptr_t b = 0;
        while (fgets(line, sizeof line, f))
            if (strstr(line, lib)) { b = strtoull(line, nullptr, 16); break; }
        fclose(f); return b;
    }

    inline bool rd(uintptr_t a, void* out, size_t n) {
        if (a < 0x10000 || a > 0x7fffffffffff) return false;
        iovec l{out, n}, r{(void*)a, n};
        return process_vm_readv(pid, &l, 1, &r, 1, 0) == (ssize_t)n;
    }
    template <class T> inline T rd(uintptr_t a) { T v{}; rd(a, &v, sizeof v); return v; }
}

struct Vec3 { double x, y, z; };
struct Rot3 { double p, y, r; };

struct Cam { Vec3 loc; Rot3 rot; float fov; };

inline bool read_cam(uintptr_t pov, Cam& c) {
    if (off::ptr64_doubles) {
        c.loc = mem::rd<Vec3>(pov);
        c.rot = mem::rd<Rot3>(pov + 0x18);
        c.fov = mem::rd<float>(pov + 0x30);
    } else {
        float b[7]; if (!mem::rd(pov, b, sizeof b)) return false;
        c.loc = {b[0], b[1], b[2]}; c.rot = {b[3], b[4], b[5]}; c.fov = b[6];
    }
    return c.fov > 1.f;
}

// UE convention: left-handed, Z up, rotation in degrees (pitch,yaw,roll). Y-flip on screen: sy = cy - ...
inline bool w2s(const Cam& c, Vec3 w, float W, float H, ImVec2& out) {
    constexpr double D2R = 3.14159265358979 / 180.0;
    double sp = sin(c.rot.p * D2R), cp = cos(c.rot.p * D2R);
    double sy = sin(c.rot.y * D2R), cy = cos(c.rot.y * D2R);
    double sr = sin(c.rot.r * D2R), cr = cos(c.rot.r * D2R);
    Vec3 ax{cp * cy, cp * sy, sp};
    Vec3 ay{sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp};
    Vec3 az{-(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp};
    Vec3 d{w.x - c.loc.x, w.y - c.loc.y, w.z - c.loc.z};
    double tx = d.x * ay.x + d.y * ay.y + d.z * ay.z;
    double ty = d.x * az.x + d.y * az.y + d.z * az.z;
    double tz = d.x * ax.x + d.y * ax.y + d.z * ax.z;
    if (tz < 1.0) return false;
    double f = (W * 0.5) / tan(c.fov * 0.5 * D2R);
    out = {(float)(W * 0.5 + tx * f / tz), (float)(H * 0.5 - ty * f / tz)};
    return true;
}

struct EspCfg {
    bool on = true, box = true, hp = true, dist = true, team_check = true;
    float max_dist = 400.f;                 // meters
    ImU32 col = IM_COL32(255, 60, 60, 255);
};
inline EspCfg g_esp;

inline bool esp_ready() {
    return off::GWorld && off::PersistentLevel && off::OwningGameInstance && off::LocalPlayers &&
           off::PlayerController && off::AcknowledgedPawn && off::CameraManager && off::CameraPOV &&
           off::Actors && off::RootComponent && off::RelLocation && off::Health;
}

inline bool esp_init(const char* pkg, const char* lib = "libUE4.so") {
    mem::pid = mem::find_pid(pkg);
    if (!mem::pid) return false;
    mem::base = mem::find_base(lib);
    return mem::base != 0;
}

inline void esp_render(float W, float H) {
    if (!g_esp.on || !esp_ready()) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    uintptr_t world = mem::rd<uintptr_t>(mem::base + off::GWorld);
    uintptr_t gi    = mem::rd<uintptr_t>(world + off::OwningGameInstance);
    uintptr_t lp    = mem::rd<uintptr_t>(mem::rd<uintptr_t>(gi + off::LocalPlayers));
    uintptr_t pc    = mem::rd<uintptr_t>(lp + off::PlayerController);
    uintptr_t cm    = mem::rd<uintptr_t>(pc + off::CameraManager);
    Cam cam; if (!read_cam(cm + off::CameraPOV, cam)) return;

    uintptr_t lvl   = mem::rd<uintptr_t>(world + off::PersistentLevel);
    uintptr_t arr   = mem::rd<uintptr_t>(lvl + off::Actors);
    int       cnt   = mem::rd<int>(lvl + off::Actors + 8);
    if (!arr || cnt <= 0 || cnt > 20000) return;

    std::vector<uintptr_t> actors(cnt);
    if (!mem::rd(arr, actors.data(), cnt * sizeof(uintptr_t))) return;

    uintptr_t self = mem::rd<uintptr_t>(pc + off::AcknowledgedPawn);
    int myTeam = (off::TeamId && self) ? mem::rd<int>(self + off::TeamId) : -1;

    for (uintptr_t a : actors) {
        if (!a || a == self) continue;
        float hp0 = mem::rd<float>(a + off::Health);
        if (hp0 <= 0.f || hp0 > 10000.f) continue;      // also filters non-character actors

        if (g_esp.team_check && off::TeamId && mem::rd<int>(a + off::TeamId) == myTeam) continue;

        uintptr_t root = mem::rd<uintptr_t>(a + off::RootComponent);
        Vec3 p = mem::rd<Vec3>(root + off::RelLocation);   // FILL: swap for ComponentToWorld if parented
        double dm = sqrt(pow(p.x - cam.loc.x, 2) + pow(p.y - cam.loc.y, 2) + pow(p.z - cam.loc.z, 2)) / 100.0;
        if (dm > g_esp.max_dist) continue;

        ImVec2 head, feet;
        if (!w2s(cam, {p.x, p.y, p.z + 90}, W, H, head)) continue;   // 90uu head / 90uu feet offsets, tune per model
        if (!w2s(cam, {p.x, p.y, p.z - 90}, W, H, feet)) continue;

        float h = feet.y - head.y, w = h * 0.45f;
        if (h < 2.f) continue;

        if (g_esp.box)
            dl->AddRect({head.x - w / 2, head.y}, {head.x + w / 2, feet.y}, g_esp.col, 0, 0, 1.5f);

        if (g_esp.hp && off::MaxHealth) {
            float mx = mem::rd<float>(a + off::MaxHealth);
            if (mx > 0) {
                float f = hp0 / mx; if (f > 1) f = 1;
                float x = head.x - w / 2 - 4;
                dl->AddRectFilled({x - 2, head.y}, {x, feet.y}, IM_COL32(0, 0, 0, 160));
                dl->AddRectFilled({x - 2, feet.y - h * f}, {x, feet.y}, IM_COL32(60, 255, 60, 255));
            }
        }
        if (g_esp.dist) {
            char t[16]; snprintf(t, sizeof t, "%.0fm", dm);
            dl->AddText({head.x - w / 2, feet.y + 2}, IM_COL32_WHITE, t);
        }
    }
}

inline void esp_menu() {
    ImGui::Begin("ESP");
    ImGui::Checkbox("Enable", &g_esp.on);
    ImGui::Checkbox("Box", &g_esp.box);
    ImGui::Checkbox("Health", &g_esp.hp);
    ImGui::Checkbox("Distance", &g_esp.dist);
    ImGui::Checkbox("Team check", &g_esp.team_check);
    ImGui::SliderFloat("Max dist (m)", &g_esp.max_dist, 50, 1000);
    ImGui::End();
}
