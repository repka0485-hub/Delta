#pragma once
#include "UmbrellaMenu.h"   // первым: нужен IMGUI_DEFINE_MATH_OPERATORS
#include "esp.h"

namespace feat {

inline float g_espCol[4] = { 1.f, 0.24f, 0.24f, 1.f };

inline void Register(umb::Menu& m) {
    using umb::Tab; using umb::Icon;
    m.Toggle(Tab::Visual, "ESP", "Enabled",    &g_esp.on,         Icon::Eye);
    m.Toggle(Tab::Visual, "ESP", "Box",        &g_esp.box,        Icon::Cube);
    m.Toggle(Tab::Visual, "ESP", "Health bar", &g_esp.hp,         Icon::Bolt);
    m.Toggle(Tab::Visual, "ESP", "Distance",   &g_esp.dist,       Icon::Crosshair);
    m.Toggle(Tab::Visual, "ESP", "Team check", &g_esp.team_check, Icon::Sparkle);
    m.SliderFloat(Tab::Visual, "Filter", "Max distance", &g_esp.max_dist, 50.f, 1000.f, "%.0f m", Icon::Resize);
    m.Color(Tab::Visual, "Filter", "Box color", g_espCol, Icon::Palette);
}

// каждый кадр, даже когда меню скрыто
inline void Update(float) {
    g_esp.col = IM_COL32((int)(g_espCol[0] * 255.f), (int)(g_espCol[1] * 255.f), (int)(g_espCol[2] * 255.f), 255);
}

} // namespace feat
