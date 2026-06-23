#pragma once

#include <cstdint>

//
// patterns.hpp — All signature patterns for Dota 2 external patcher
//
// Sources:
//   TheXSVV (forum #28): PrepareUnitOrders, EntityList, LocalPlayerController, ViewMatrix
//   Cr33py (forum #31-39): CGCClientSystem, SendMessage, ExecuteUICommand,
//       ResumeFindingMatch, PlayPanelPtr, AcceptPanelPtr, GetProtoCDOTAGameAccountPlus,
//       AcceptMatchHandler, particles rendering, visible_by_enemy, fog patches
//   Wolf49406/Dota2Patcher: CreateInterface, CDOTACamera, VMatrix, set_rendering_enabled
//   tranqu1lizer (original): game_entity_system, emoticon, weather, local_player_array
//
// IDA-style pattern syntax: ? = wildcard byte
//

namespace sig {

// ── Core interfaces / globals ──

inline constexpr const char* create_interface =
    "4C 8B ? ? ? ? ? 4C 8B ? 4C 8B ? 4D 85 ? 74 ? 49 8B ? ? 4D 8B";

// CDOTACamera — original tranqu1lizer pattern from external_patcher
inline constexpr const char* cdota_camera =
    "48 89 74 24 40 48 89 7C 24 50 48 8D ? ? ? ? ? 48 8B 04 C8 8B 04 02 39";

// ViewMatrix — long form (TheXSVV / Dota2Patcher), more specific
inline constexpr const char* view_matrix =
    "48 8D 0D ? ? ? ? 48 C1 E0 ? 48 03 C1 C3 "
    "CC CC CC CC CC CC CC CC CC CC CC CC CC CC";

// ── Entity system (original tranqu1lizer + TheXSVV) ──

inline constexpr const char* game_entity_system =
    "48 8B 0D ? ? ? ? 33 D2 E8 ? ? ? ?";

inline constexpr const char* entity_list =
    "4C 8B 0D ? ? ? ? 4D 85 C9 74 ? 41 83 F8 ? 74 ? 41 8B C8 81 E1 ? ? ? ? "
    "8B C1 C1 E8 ? 4D 8B 14 C1 4D 85 D2 74 ? 81 E1 ? ? ? ? 48 6B C1 ? 49 03 C2 "
    "74 ? 44 39 40 ? 48 0F 45 C2 EB ? 48 8B C2 48 85 C0 74 ? 48 8B 10 48 8B C2 C3 CC";

inline constexpr const char* local_player_array =
    "48 8B 05 ? ? ? ? 89 BE";

// ── Player / orders (TheXSVV) ──

inline constexpr const char* local_player_controller =
    "4C 8D 05 ? ? ? ? 83 F9 ? 8B C2";

inline constexpr const char* prepare_unit_orders =
    "4C 89 4C 24 ? 44 89 44 24 ? 89 54 24 ? 48 89 4C 24 ? 55 53 56 57 41 56";

// ── GC Client system (Cr33py) ──

inline constexpr const char* cgc_client_system =
    "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 01 FF 90 28 01 00 00";

inline constexpr const char* send_message =
    "48 89 5C 24 08 57 48 83 EC 30 48 8B F9 E8 ? ? ? ? "
    "48 8B 97 00 06 00 00 48 8B 9F 70 16 00 00";

// ── UI / match system (Cr33py) ──

inline constexpr const char* execute_ui_command =
    "40 53 48 83 EC 30 48 8B D9 48 8B 0D ? ? ? ? 48 85 C9 75 0B";

inline constexpr const char* play_panel_ptr =
    "48 8B 1D ? ? ? ? 48 85 DB 0F 84 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 0F B7 0D ? ? ? ?";

inline constexpr const char* accept_panel_ptr =
    "48 8B 1D ? ? ? ? 48 8D 15 ? ? ? ? 48 8D 8D ? ? ? ? E8 ? ? ? ? 48 8B 4B 08";

inline constexpr const char* resume_finding_match =
    "49 8B CE E8 ? ? ? ? E8 ? ? ? ? 83 F8 01";

inline constexpr const char* accept_match_handler =
    "48 83 EC 48 48 8D 05 ? ? ? ? 45 33 C9 48 89 44 24 38 48 8D 15 ? ? ? ? 48 89 54 24 30";

// ── Dota Plus (Cr33py) ──

inline constexpr const char* get_proto_account_plus =
    "48 83 EC 38 48 8B 89 38 05 00 00 48 85 C9 74 45 BA DC 07 00 00 E8";

// ── Visual patches (Cr33py / Wolf49406) ──

// Particles always render — Dota2Patcher pattern (particles.dll)
inline constexpr const char* particles_rendering =
    "0F 84 ? ? ? ? 4D 89 73";

// Particles crash fix — in client.dll
inline constexpr const char* particles_rendering_fix =
    "0F 84 ? ? ? ? 48 8B ? 48 85 ? 74 ? 48 8B ? 48 8B";

// Visible By Enemy / hero visible through fog — same code path.
// First match is the function the original patcher uses; patch 75 -> EB (jmp).
inline constexpr const char* visible_by_enemy =
    "75 ? 41 8B CE E8 ? ? ? ? 48 85 C0 74 ? 80 B8";

// ── Original tranqu1lizer patterns (updated) ──

inline constexpr const char* weather =
    "48 8B D9 BA ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 85 C0 75 ? "
    "48 8B 05 ? ? ? ? 48 8B 40 ? 83 38 ? 74 ? BA ? ? ? ? 48 8D 0D ? ? ? ? "
    "E8 ? ? ? ? 48 85 C0 75 ? 48 8B 05 ? ? ? ? 48 8B 40 ? 8B 00";

} // namespace sig
