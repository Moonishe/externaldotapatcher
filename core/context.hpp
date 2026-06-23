#pragma once

#include "dota_sdk.hpp"
#include <cstdint>

struct Context {
    bool init = false;

    CDOTA_Camera camera;
    uintptr_t entity_system = 0;
    uintptr_t particles_addr = 0;
    uintptr_t particles_fix  = 0;
    uintptr_t weather_addr   = 0;
    uintptr_t dota_plus_addr = 0;

    bool fog_enabled = false;
    int  current_weather_id = 0;
};
