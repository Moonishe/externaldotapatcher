//
// main.cpp — Dota 2 External Patcher
// Raw WinAPI, no BlackBone dependency.
//
// Features:
//   [1] Camera distance
//   [2] Patch ZFar (fog controller)
//   [3] Fog toggle
//   [4] Particles in FOW
//   [5] Weather change
//   [6] Dota Plus unlock
//   [0] Exit
//

#include "color.hpp"
#include "dota_sdk.hpp"
#include "autoaccept.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <csignal>
#include <atomic>
#include <cstdint>
#include <windows.h>
#include <tlhelp32.h>

// ── Process finder ─────────────────────────────────────────────
static DWORD find_pid(const wchar_t* name) {
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (wcscmp(pe.szExeFile, name) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

// ── Dota Plus toggle ──────────────────────────────────────────
static void toggle_dota_plus(uintptr_t addr) {
    if (!addr) return;
    uint8_t first = Memory::read<uint8_t>(addr).value_or(0);
    if (first == 0x48) {
        // sub rsp, 38h -> mov al, 1; ret
        Memory::patch(addr, "B0 01 C3");
        std::cout << "[+] Dota Plus ON\n";
    } else if (first == 0xB0) {
        // restore prologue: 48 83 EC (4th byte 38 still intact)
        Memory::patch(addr, "48 83 EC");
        std::cout << "[+] Dota Plus OFF\n";
    } else {
        std::cout << "[unknown dota_plus bytes at " << std::hex << addr << "]\n";
    }
}

// ── Cached scan results ────────────────────────────────────────
#include "context.hpp"
Context g;

static bool is_particles_on() {
    if (!g.particles_addr) return false;
    return Memory::read<uint8_t>(g.particles_addr + 1).value_or(0) == 0x85;
}
static bool is_dota_plus_on() {
    if (!g.dota_plus_addr) return false;
    return Memory::read<uint8_t>(g.dota_plus_addr).value_or(0) == 0xB0;
}

// ── Initialize: scan all patterns once ─────────────────────────
static bool init_all() {
    if (g.init) return true;

    g.camera          = Scanner::find_camera();
    g.entity_system   = Scanner::find_entity_system();
    g.particles_addr  = Scanner::find_particles_addr();
    g.particles_fix   = Scanner::find_particles_fix_addr();
    g.weather_addr    = Scanner::find_weather_addr();
    g.dota_plus_addr  = Scanner::find_proto_account_plus();

    if (g.weather_addr) {
        auto first = Memory::read<uint8_t>(g.weather_addr).value_or(0);
        if (first == 0xB8)
            g.current_weather_id = Memory::read<uint8_t>(g.weather_addr + 1).value_or(0);
        else
            g.current_weather_id = 0;
    }

    g.init = true;

    // Report what was found
    if (g.camera.valid())
        std::cout << dye::green("  [+] Camera found\n");
    else
        std::cout << "  [-] Camera not found\n";

    if (g.entity_system)
        std::cout << dye::green("  [+] Entity system found\n");
    else
        std::cout << "  [-] Entity system not found\n";

    {
        auto pit = Memory::modules.find("particles.dll");
        if (pit != Memory::modules.end()) {
            std::cout << "  [i] particles.dll: 0x" << std::hex << pit->second.start
                      << " - 0x" << pit->second.end << " (size " << std::dec << pit->second.size << ")\n";
            std::wcout << "      path: " << pit->second.path << "\n";
        }
        if (g.particles_addr)
            std::cout << dye::green("  [+] Particles pattern found at 0x") << std::hex << g.particles_addr << "\n";
        else
            std::cout << "  [-] Particles pattern not found\n";
    }

    if (g.weather_addr)
        std::cout << dye::green("  [+] Weather pattern found\n");
    else
        std::cout << "  [-] Weather pattern not found\n";

    if (g.dota_plus_addr)
        std::cout << dye::green("  [+] Dota Plus pattern found\n");
    else
        std::cout << "  [-] Dota Plus pattern not found\n";

    return true;
}

static std::string colored_autoaccept_mode() {
    auto s = autoaccept_mode_str();
    auto m = autoaccept_get_mode();
    std::ostringstream oss;
    if (m == AutoAcceptMode::Normal)
        oss << dye::light_green(s);
    else if (m == AutoAcceptMode::DotaPlus)
        oss << dye::light_yellow(s);
    else
        oss << dye::light_red(s);
    return oss.str();
}

// ── Menu ───────────────────────────────────────────────────────
static void show_menu() {
    std::cout << dye::aqua_on_blue("  Dota 2 External Patcher  ") << "\n"
              << "[" << dye::light_aqua("https://github.com/Moonishe/externaldotapatcher") << "]\n\n"
              << dye::black_on_yellow("  [!] = may cause instability  ") << "\n\n"
              << " [1] Camera distance\n"
              << " [2] Patch ZFar (fog controller)\n"
              << " [3] Fog              " << (g.fog_enabled ? dye::light_green("On") : dye::light_red("Off")) << "\n"
              << " [4] " << dye::black_on_yellow("[!]") << " Particles in FOW   " << (is_particles_on() ? dye::light_green("On") : dye::light_red("Off")) << "\n"
              << " [5] " << dye::black_on_yellow("[!]") << " Weather            " << "(id: " << g.current_weather_id << ", 0=default, 1..9=other)\n"
              << " [6] " << dye::black_on_yellow("[!]") << " Dota Plus          " << (is_dota_plus_on() ? dye::light_green("On") : dye::light_red("Off")) << "\n"
              << " [7] " << dye::black_on_yellow("[!]") << " Auto-Accept        " << colored_autoaccept_mode() << "\n"
              << " [0] Exit\n\n"
              << "=> ";
}

void redraw_menu() {
    system("cls");
    std::cout << dye::green("[+] Auto-Accept: game accepted! Mode reset to Off.\n\n");
    show_menu();
}

// ── Main ───────────────────────────────────────────────────────
int main() {
    std::cout << "Looking for dota2.exe...\n";
    DWORD pid = find_pid(L"dota2.exe");
    if (!pid) {
        std::cout << dye::black_on_red("dota2.exe not found. Is the game running?") << "\n";
        system("pause");
        return 1;
    }

    if (!Memory::open(pid)) {
        std::cout << dye::black_on_red("OpenProcess failed. Run as administrator.") << "\n";
        system("pause");
        return 1;
    }

    std::cout << "Loading modules...\n";
    if (!Memory::load_modules(pid)) {
        std::cout << dye::black_on_red("Failed to load modules.") << "\n";
        system("pause");
        return 1;
    }

    auto client = Memory::pattern_scan("client.dll", "48 8B 0D");  // quick existence check
    if (!client) {
        std::cout << dye::black_on_red("client.dll not accessible.") << "\n";
        system("pause");
        return 1;
    }

    std::cout << dye::green("Attached to dota2.exe (PID: ") << pid << ")\n";
    std::cout << "Scanning patterns...\n\n";
    init_all();
    autoaccept_init();
    autoaccept_set_redraw_callback(redraw_menu);

    std::cout << "\nPress any key to continue...";
    system("pause >nul");
    system("cls");

    while (true) {
        show_menu();

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        system("cls");

        switch (choice) {
        case 0:
            goto exit;

        case 1: {
            if (!g.camera.valid()) {
                std::cout << dye::black_on_red("Camera not available\n");
                system("pause");
                break;
            }
            std::cout << "Current distance: " << g.camera.get_distance() << "\n";
            std::cout << "Enter new distance (default ~1200, max ~5000)\n=> ";
            float dist;
            std::cin >> dist;
            g.camera.set_distance(dist);
            std::cout << dye::green("[+] Camera distance set to ") << dist << "\n";
            Sleep(800);
            break;
        }

        case 2:
            if (!g.entity_system) {
                std::cout << dye::black_on_red("Entity system not available\n");
                system("pause");
                break;
            }
            toggle_fog_farz(g.entity_system, true);
            system("pause");
            break;

        case 3:
            if (!g.entity_system) {
                std::cout << dye::black_on_red("Entity system not available\n");
                system("pause");
                break;
            }
            g.fog_enabled = toggle_fog_farz(g.entity_system, false);
            system("pause");
            break;

        case 4:
            if (!g.particles_addr) {
                std::cout << dye::black_on_red("Particles pattern not found\n");
                system("pause");
                break;
            }
            toggle_particles(g.particles_addr);
            if (g.particles_fix)
                toggle_particles_fix(g.particles_fix);
            system("pause");
            break;

        case 5: {
            if (!g.weather_addr) {
                std::cout << dye::black_on_red("Weather pattern not found\n");
                system("pause");
                break;
            }
            std::cout << "Current weather id: " << g.current_weather_id << "\n";
            std::cout << "Available: 0 = default, 1 = rain, 2 = snow, 3 = ash, 4 = moon, 5 = rainstorm, 6 = custom1, 7 = custom2, 8 = custom3, 9 = custom4\n";
            std::cout << "Enter weather number (0-9)\n=> ";
            int w;
            std::cin >> w;
            change_weather(g.weather_addr, w);
            g.current_weather_id = w;
            std::cout << dye::green("[+] Weather set to id ") << w << "\n";
            system("pause");
            break;
        }

        case 6:
            if (!g.dota_plus_addr) {
                std::cout << dye::black_on_red("Dota Plus pattern not found\n");
                system("pause");
                break;
            }
            toggle_dota_plus(g.dota_plus_addr);
            system("pause");
            break;

        case 7: {
            auto mode = autoaccept_get_mode();
            if (mode == AutoAcceptMode::Off)
                autoaccept_set_mode(AutoAcceptMode::Normal);
            else if (mode == AutoAcceptMode::Normal)
                autoaccept_set_mode(AutoAcceptMode::DotaPlus);
            else
                autoaccept_set_mode(AutoAcceptMode::Off);
            std::cout << dye::green("[+] Auto-Accept: ") << colored_autoaccept_mode() << "\n";
            std::cout << "    Normal:  (960, 540)  white text\n";
            std::cout << "    Dota+:   (979, 381)  RGB(134,219,138)\n";
            Sleep(800);
            break;
        }

        default:
            break;
        }

        system("cls");
    }

exit:
    autoaccept_shutdown();
    Memory::close();
    return 0;
}
