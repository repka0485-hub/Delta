#include "surface.h"
#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <vector>
#include <cstdio>

// size of the overlay window in pixels; create_overlay_window must set these
int g_surface_w = 0, g_surface_h = 0;

// panel is portrait, game is landscape: FILL 90 or 270 (0 if the overlay is portrait)
static constexpr int kRotation = 90;

namespace {

struct Dev {
    int fd;
    input_absinfo ax, ay;
    int slot = 0, rx = 0, ry = 0;
    bool down = false;
};

std::atomic<float> g_x{0.f}, g_y{0.f};
std::atomic<bool>  g_down{false}, g_have{false}, g_started{false};

bool bit(const unsigned long* a, int b) {
    return (a[b / (8 * sizeof(long))] >> (b % (8 * sizeof(long)))) & 1UL;
}

float norm(int v, const input_absinfo& a) {
    float d = (float)(a.maximum - a.minimum);
    if (d <= 0.f) return 0.f;
    float n = (float)(v - a.minimum) / d;
    return n < 0.f ? 0.f : (n > 1.f ? 1.f : n);
}

void publish(const Dev& d) {
    float nx = norm(d.rx, d.ax), ny = norm(d.ry, d.ay);
    float W = (float)g_surface_w, H = (float)g_surface_h, sx, sy;
    if (kRotation == 90)       { sx = ny * W;        sy = (1.f - nx) * H; }
    else if (kRotation == 270) { sx = (1.f - ny) * W; sy = nx * H; }
    else                       { sx = nx * W;        sy = ny * H; }
    g_x = sx; g_y = sy; g_down = d.down;
}

void worker() {
    std::vector<Dev> devs;
    for (int i = 0; i < 32; i++) {
        char p[32]; snprintf(p, sizeof p, "/dev/input/event%d", i);
        int fd = open(p, O_RDONLY | O_CLOEXEC);
        if (fd < 0) continue;
        unsigned long abs[(ABS_MAX + 64) / 64] = {};
        Dev d{}; d.fd = fd;
        if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof abs), abs) < 0 ||
            !bit(abs, ABS_MT_POSITION_X) || !bit(abs, ABS_MT_POSITION_Y) ||
            ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &d.ax) < 0 ||
            ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &d.ay) < 0) { close(fd); continue; }
        d.rx = d.ax.value; d.ry = d.ay.value;
        devs.push_back(d);
    }
    if (devs.empty()) return;
    g_have = true;

    std::vector<pollfd> pf(devs.size());
    for (size_t i = 0; i < devs.size(); i++) pf[i] = { devs[i].fd, POLLIN, 0 };

    for (;;) {
        if (poll(pf.data(), pf.size(), 1000) <= 0) continue;
        for (size_t i = 0; i < devs.size(); i++) {
            if (!(pf[i].revents & POLLIN)) continue;
            Dev& d = devs[i];
            input_event ev[64];
            ssize_t n = read(d.fd, ev, sizeof ev);
            if (n <= 0) continue;
            for (ssize_t k = 0; k < n / (ssize_t)sizeof(input_event); k++) {
                const input_event& e = ev[k];
                if (e.type == EV_ABS) {
                    if (e.code == ABS_MT_SLOT) d.slot = e.value;
                    else if (d.slot == 0) {                  // first finger only
                        if (e.code == ABS_MT_TRACKING_ID) d.down = e.value >= 0;
                        else if (e.code == ABS_MT_POSITION_X) d.rx = e.value;
                        else if (e.code == ABS_MT_POSITION_Y) d.ry = e.value;
                    }
                } else if (e.type == EV_SYN && e.code == SYN_REPORT) {
                    publish(d);
                }
            }
        }
    }
}

} // namespace

bool poll_touch(float* x, float* y, bool* down) {
    if (!g_started.exchange(true)) std::thread(worker).detach();
    *x = g_x; *y = g_y; *down = g_down;
    return g_have;
}

// ---------------------------------------------------------------------------
// create_overlay_window: бэкенд окна ещё не написан (нужны реальные символы libgui
// с этого телефона). Пока бинарь сохраняет их в /data/local/tmp/libgui_syms.txt.
// ---------------------------------------------------------------------------
#include <cstring>
#include <set>
#include <string>

static void dump_gui_symbols() {
    static const char* paths[] = { "/system/lib64/libgui.so", "/system_ext/lib64/libgui.so", nullptr };
    static const char* prefixes[] = { "_ZN7android21SurfaceComposerClient", "_ZN7android14SurfaceControl",
                                      "_ZN7android7Surface", nullptr };
    std::set<std::string> out;
    for (const char* const* p = paths; *p; ++p) {
        FILE* f = fopen(*p, "rb");
        if (!f) continue;
        std::string buf;
        char tmp[65536]; size_t n;
        while ((n = fread(tmp, 1, sizeof tmp, f)) > 0) buf.append(tmp, n);
        fclose(f);
        for (const char* const* pre = prefixes; *pre; ++pre) {
            size_t pos = 0, plen = strlen(*pre);
            while ((pos = buf.find(*pre, pos)) != std::string::npos) {
                if (pos == 0 || buf[pos - 1] == '\0') out.insert(std::string(buf.c_str() + pos));
                pos += plen;
            }
        }
        if (!out.empty()) break;
    }
    FILE* o = fopen("/data/local/tmp/libgui_syms.txt", "w");
    if (!o) return;
    for (const auto& s : out) fprintf(o, "%s\n", s.c_str());
    fclose(o);
    fprintf(stderr, "libgui symbols: %zu -> /data/local/tmp/libgui_syms.txt\n", out.size());
}

ANativeWindow* create_overlay_window(int* w, int* h) {
    (void)w; (void)h;
    dump_gui_symbols();
    fprintf(stderr, "overlay window backend not implemented yet, send libgui_syms.txt\n");
    return nullptr;
}
