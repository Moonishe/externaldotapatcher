#include "autoaccept.hpp"

#include <windows.h>
#include <thread>
#include <atomic>
#include <cmath>

namespace {
    struct Config {
        // Button plate (trigger + shape check)
        int plate_x = 0;
        int plate_y = 0;
        COLORREF plate_color = 0;
        int plate_tolerance = 35;
        // Button text (additional check)
        int text_x = 0;
        int text_y = 0;
        COLORREF text_color = 0;
        int text_tolerance = 35;
    };

    std::atomic<bool> g_running{ false };
    std::atomic<bool> g_enabled{ false };
    std::thread g_thread;
    AutoAcceptMode g_mode = AutoAcceptMode::Off;

    Config cfg_normal;
    Config cfg_dotaplus;

    Config load_normal() {
        // User-provided: normal queue
        // Screenshot shows Russian "ПРИНЯТЬ" text is white on a dark green plate.
        return {
            960, 540, RGB(10, 80, 40), 35,
            960, 540, RGB(255, 255, 255), 35
        };
    }

    Config load_dotaplus() {
        // User-provided: Dota+ queue
        return {
            979, 381, RGB(134, 219, 138), 35,
            979, 381, RGB(134, 219, 138), 35
        };
    }

    bool is_dota2_active() {
        HWND fg = GetForegroundWindow();
        if (!fg) return false;
        wchar_t title[256]{};
        GetWindowTextW(fg, title, 256);
        // Dota 2 window title usually contains "Dota 2"
        return wcsstr(title, L"Dota 2") != nullptr;
    }

    bool color_matches(COLORREF c1, COLORREF c2, int tolerance) {
        int r1 = GetRValue(c1);
        int g1 = GetGValue(c1);
        int b1 = GetBValue(c1);
        int r2 = GetRValue(c2);
        int g2 = GetGValue(c2);
        int b2 = GetBValue(c2);
        return std::abs(r1 - r2) + std::abs(g1 - g2) + std::abs(b1 - b2) <= tolerance;
    }

    bool check_button_plate(HDC hdc, int text_x, int text_y, COLORREF color, int tolerance) {
        // Check the 8 pixels surrounding the text pixel.
        // If at least 5 of them match the plate color, we treat it as the button plate.
        int matches = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                COLORREF pixel = GetPixel(hdc, text_x + dx, text_y + dy);
                if (pixel != CLR_INVALID && color_matches(pixel, color, tolerance))
                    ++matches;
            }
        }
        return matches >= 5;
    }

    bool check_text_pixel(HDC hdc, int x, int y, COLORREF color, int tolerance) {
        COLORREF pixel = GetPixel(hdc, x, y);
        return pixel != CLR_INVALID && color_matches(pixel, color, tolerance);
    }

    void click_at(int x, int y) {
        int screen_w = GetSystemMetrics(SM_CXSCREEN);
        int screen_h = GetSystemMetrics(SM_CYSCREEN);

        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = static_cast<LONG>(x * 65535LL / screen_w);
        input.mi.dy = static_cast<LONG>(y * 65535LL / screen_h);
        input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }

    void autoaccept_loop() {
        HWND desktop = GetDesktopWindow();
        HDC hdc = GetDC(desktop);
        if (!hdc) return;

        while (g_running) {
            if (g_enabled) {
                const Config& cfg = (g_mode == AutoAcceptMode::DotaPlus) ? cfg_dotaplus : cfg_normal;

                if (is_dota2_active()) {
                    bool plate_ok = check_button_plate(hdc, cfg.plate_x, cfg.plate_y, cfg.plate_color, cfg.plate_tolerance);
                    bool text_ok  = check_text_pixel(hdc, cfg.text_x, cfg.text_y, cfg.text_color, cfg.text_tolerance);
                    if (plate_ok && text_ok) {
                        click_at(cfg.plate_x, cfg.plate_y);
                        // Stop after accepting one match
                        g_enabled = false;
                        g_mode = AutoAcceptMode::Off;
                    }
                }
            }
            Sleep(350);
        }

        ReleaseDC(desktop, hdc);
    }
} // namespace

bool autoaccept_init() {
    if (g_running) return true;
    cfg_normal = load_normal();
    cfg_dotaplus = load_dotaplus();
    g_running = true;
    g_thread = std::thread(autoaccept_loop);
    return true;
}

void autoaccept_shutdown() {
    g_running = false;
    g_enabled = false;
    if (g_thread.joinable())
        g_thread.join();
}

void autoaccept_set_mode(AutoAcceptMode mode) {
    g_mode = mode;
    g_enabled = (mode != AutoAcceptMode::Off);
}

AutoAcceptMode autoaccept_get_mode() {
    return g_mode;
}

std::string autoaccept_mode_str() {
    switch (g_mode) {
        case AutoAcceptMode::Normal:   return "Normal";
        case AutoAcceptMode::DotaPlus: return "Dota+";
        default:                       return "Off";
    }
}
