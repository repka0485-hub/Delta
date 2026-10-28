#pragma once
#include <android/native_window.h>

// Implemented by your existing overlay code (surface.cpp in the repo root, picked up by CMake if present).
// create_overlay_window: transparent, always-on-top ANativeWindow, returns size in pixels.
// poll_touch: latest touch state in window pixels; return false if no touch device.
ANativeWindow* create_overlay_window(int* w, int* h);
bool poll_touch(float* x, float* y, bool* down);
