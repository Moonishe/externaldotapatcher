#pragma once

#include "memory.hpp"
#include "patterns.hpp"
#include <cstdint>
#include <string>

class CDOTA_Camera {
public:
    uintptr_t base = 0;

    CDOTA_Camera() = default;
    explicit CDOTA_Camera(uintptr_t b) : base(b) {}

    bool valid() const { return base != 0; }

    // Offsets from original external_patcher + dota2dumped
    void set_distance(float d) { Memory::write<float>(base + 0x2E4, d); }
    float get_distance() const { return Memory::read<float>(base + 0x270).value_or(0.0f); }

    void set_farz(float f) { Memory::write<float>(base + 0x2A4, f); }
    float get_farz() const { return Memory::read<float>(base + 0x2A4).value_or(0.0f); }
};

class Scanner {
public:
    static CDOTA_Camera find_camera() {
        auto addr = Memory::pattern_scan("client.dll", sig::cdota_camera);
        if (!addr)
            return {};
        // +10 points to the lea instruction inside the pattern, then resolve rip-relative
        auto manager = Memory::absolute_address(addr.value() + 10, 3, 7);
        if (!manager)
            return {};
        auto camera = Memory::read<uintptr_t>(manager.value() + 0x20);
        if (!camera)
            return {};
        return CDOTA_Camera(camera.value());
    }

    static uintptr_t find_entity_system() {
        auto addr = Memory::pattern_scan("client.dll", sig::game_entity_system);
        if (!addr)
            return 0;
        return Memory::absolute_address(addr.value(), 3, 7).value_or(0);
    }

    static uintptr_t find_local_player() {
        auto addr = Memory::pattern_scan("client.dll", sig::local_player_array);
        if (!addr)
            return 0;
        return Memory::absolute_address(addr.value(), 3, 7).value_or(0);
    }

    static uintptr_t find_create_interface() {
        return Memory::pattern_scan("client.dll", sig::create_interface).value_or(0);
    }

    static uintptr_t find_entity_list() {
        auto addr = Memory::pattern_scan("client.dll", sig::entity_list);
        if (!addr)
            return 0;
        return Memory::absolute_address(addr.value(), 3, 7).value_or(0);
    }

    static uintptr_t find_prepare_unit_orders() {
        return Memory::pattern_scan("client.dll", sig::prepare_unit_orders).value_or(0);
    }

    static uintptr_t find_execute_ui_command() {
        return Memory::pattern_scan("client.dll", sig::execute_ui_command).value_or(0);
    }

    static uintptr_t find_play_panel_ptr() {
        auto addr = Memory::pattern_scan("client.dll", sig::play_panel_ptr);
        if (!addr)
            return 0;
        return Memory::absolute_address(addr.value(), 3, 7).value_or(0);
    }

    static uintptr_t find_accept_panel_ptr() {
        auto addr = Memory::pattern_scan("client.dll", sig::accept_panel_ptr);
        if (!addr)
            return 0;
        return Memory::absolute_address(addr.value(), 3, 7).value_or(0);
    }

    static uintptr_t find_proto_account_plus() {
        return Memory::pattern_scan("client.dll", sig::get_proto_account_plus).value_or(0);
    }

    static uintptr_t find_cgc_client_system() {
        auto addr = Memory::pattern_scan("client.dll", sig::cgc_client_system);
        if (!addr)
            return 0;
        return Memory::absolute_address(addr.value(), 3, 7).value_or(0);
    }

    static uintptr_t find_send_message() {
        return Memory::pattern_scan("client.dll", sig::send_message).value_or(0);
    }

    static uintptr_t find_particles_addr() {
        return Memory::pattern_scan("particles.dll", sig::particles_rendering).value_or(0);
    }

    static uintptr_t find_particles_fix_addr() {
        return Memory::pattern_scan("client.dll", sig::particles_rendering_fix).value_or(0);
    }

    static uintptr_t find_weather_addr() {
        auto addr = Memory::pattern_scan("client.dll", sig::weather);
        if (!addr)
            return 0;
        return addr.value() - 0x6;
    }
};

static uintptr_t GetBaseEntity(uintptr_t entity_system, int idx) {
    if (idx <= 0x7FFE && (idx >> 9) <= 0x3F) {
        auto chunk = Memory::read<uintptr_t>(entity_system + 8 * (idx >> 9) + 16);
        if (!chunk || !chunk.value())
            return 0;
        auto ent_ptr = chunk.value() + 120 * (idx & 0x1FF);
        return Memory::read<uintptr_t>(ent_ptr).value_or(0);
    }
    return 0;
}

static std::string GetEntityName(uintptr_t ident) {
    char buf[64] = {};

    auto m_name = Memory::read<uintptr_t>(ident + 0x18);
    if (m_name && m_name.value()) {
        Memory::read_buffer(m_name.value(), buf, sizeof(buf) - 1);
        if (buf[0])
            return std::string(buf);
    }

    auto m_designerName = Memory::read<uintptr_t>(ident + 0x20);
    if (m_designerName && m_designerName.value()) {
        Memory::read_buffer(m_designerName.value(), buf, sizeof(buf) - 1);
        if (buf[0])
            return std::string(buf);
    }

    return "";
}

static bool toggle_fog_farz(uintptr_t ppGameEntitySystem, bool farz = false) {
    auto pGameEntitySystem = Memory::read<uintptr_t>(ppGameEntitySystem).value_or(0);
    if (!pGameEntitySystem) {
        std::cout << "[fog] entity system pointer is null\n";
        return false;
    }

    std::cout << "[fog] entity system = 0x" << std::hex << pGameEntitySystem << "\n";

    auto first_identity = Memory::read<uintptr_t>(pGameEntitySystem + 0x210).value_or(0);
    std::cout << "[fog] first_identity = 0x" << std::hex << first_identity << "\n";
    if (!first_identity) {
        std::cout << "[fog] first identity is null (wrong offset?)\n";
        return false;
    }

    bool found = false;
    int count = 0;
    bool new_enabled = false;
    for (auto ident = first_identity; ident && count < 20000; ident = Memory::read<uintptr_t>(ident + 0x58).value_or(0), ++count) {
        auto name = GetEntityName(ident);
        if (name == "env_fog_controller" || name == "C_FogController" || name == "fog_controller") {
            auto fog = Memory::read<uintptr_t>(ident).value_or(0);
            if (!fog)
                continue;

            found = true;
            uintptr_t fogparams = fog + 0x5F0;
            if (farz) {
                Memory::write<float>(fogparams + 0x2c, 18000.0f);
                std::cout << "[fog] set farz=18000 on " << name << " (fog=0x" << std::hex << fog << ")\n";
            } else {
                bool enabled = Memory::read<uint8_t>(fogparams + 0x64).value_or(0);
                new_enabled = !enabled;
                Memory::write<uint8_t>(fogparams + 0x64, new_enabled);
                std::cout << "[+] Fog " << (new_enabled ? "ON" : "OFF") << "\n";
            }
        }
    }

    if (!found)
        std::cout << "[fog] no fog controller entity found (scanned " << count << " identities)\n";

    return farz ? found : new_enabled;
}

static void toggle_particles(uintptr_t particles_addr) {
    if (!particles_addr)
        return;
    uint8_t b = Memory::read<uint8_t>(particles_addr + 1).value_or(0);
    if (b == 0x84) {
        Memory::patch(particles_addr + 1, { 0x85 });
        std::cout << "[+] Particles in FOW ON\n";
    } else if (b == 0x85) {
        Memory::patch(particles_addr + 1, { 0x84 });
        std::cout << "[+] Particles in FOW OFF\n";
    } else {
        std::cout << "[unknown particle bytes at " << std::hex << particles_addr << "+1]\n";
    }
}

static void toggle_particles_fix(uintptr_t fix_addr) {
    if (!fix_addr)
        return;
    uint8_t b = Memory::read<uint8_t>(fix_addr + 1).value_or(0);
    if (b == 0x84)
        Memory::patch(fix_addr + 1, { 0x85 });
    else if (b == 0x85)
        Memory::patch(fix_addr + 1, { 0x84 });
}

static void change_weather(uintptr_t weather_addr, int weather_id) {
    if (!weather_addr)
        return;

    uint8_t first = Memory::read<uint8_t>(weather_addr).value_or(0);
    uint8_t patch[6] = { 0xB8, static_cast<uint8_t>(weather_id), 0x00, 0x00, 0x00, 0xC3 };

    if (weather_id != 0) {
        Memory::patch(weather_addr, std::vector<uint8_t>(patch, patch + 6));
    } else if (first == 0xB8) {
        Memory::patch(weather_addr, "40 53 48 83 EC 20");
    }
}

static void toggle_camera_farz(CDOTA_Camera& cam) {
    if (!cam.valid())
        return;
    float farz = cam.get_farz();
    if (farz < 10000.0f)
        cam.set_farz(18000.0f);
    else
        cam.set_farz(1000.0f);
}

