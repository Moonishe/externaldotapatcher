# external_patcher v2

Refactored Dota 2 external patcher that no longer depends on BlackBone. It uses plain Win32 API (`ReadProcessMemory` / `WriteProcessMemory` / `VirtualProtectEx`) and a small IDA-style pattern scanner.

## Build

### Visual Studio 2022
Open `camera_patch.vcxproj`, pick `Release|x64`, build.

### Visual Studio Code
Requires MSVC (Visual Studio / Build Tools) and CMake Tools extension.

1. Open the patcher folder in VS Code.
2. CMake Tools will detect `CMakeLists.txt`.
3. Select kit: `Visual Studio ... Release|x64`.
4. Build.

Or from terminal (Developer Command Prompt):

```batch
build.bat
```

No extra dependencies required.

## Features

- Camera distance patch
- ZFar patch (via fog controller entity)
- Fog toggle
- Particles in FOW toggle (with crash fix)
- Emoticon unlock
- Weather change
- Visible-by-enemy toggle (hero visible through fog)
- Dota Plus unlock (client-side visual)
- Auto-accept match

## Current signatures (as of the latest scraped forum data)

| Name | Module | Signature |
|------|--------|-----------|
| `create_interface` | client.dll | `4C 8B ? ? ? ? ? 4C 8B ? 4C 8B ? 4D 85 ? 74 ? 49 8B ? ? 4D 8B` |
| `CDOTACamera` | client.dll | `48 8D ? ? ? ? ? 48 8D ? ? ? ? ? 48 89 ? ? ? ? ? E9 ? ? ? ? CC CC CC CC CC CC CC CC 48 8D` |
| `ViewMatrix` | client.dll | `48 8D 0D ? ? ? ? 48 C1 E0 06` |
| `EntityList` | client.dll | `4C 8B 0D ? ? ? ? 4D 85 C9 74 ? 41 83 F8 ? 74 ? 41 8B C8 81 E1 ? ? ? ? 8B C1 C1 E8 ? 4D 8B 14 C1 4D 85 D2 74 ? 81 E1 ? ? ? ? 48 6B C1 ? 49 03 C2 74 ? 44 39 40 ? 48 0F 45 C2 EB ? 48 8B C2 48 85 C0 74 ? 48 8B 10 48 8B C2 C3 CC` |
| `LocalPlayerController` | client.dll | `4C 8D 05 ? ? ? ? 83 F9 ? 8B C2` |
| `PrepareUnitOrders` | client.dll | `4C 89 4C 24 ? 44 89 44 24 ? 89 54 24 ? 48 89 4C 24 ? 55 53 56 57 41 56` |
| `ExecuteUICommand` | client.dll | `40 53 48 83 EC 30 48 8B D9 48 8B 0D ? ? ? ? 48 85 C9 75 0B` |
| `AcceptMatchHandler` | client.dll | `48 83 EC 48 48 8D 05 ? ? ? ? 45 33 C9 48 89 44 24 38 48 8D 15 ? ? ? ? 48 89 54 24 30` |
| `ResumeFindingMatch` | client.dll | `49 8B CE E8 ? ? ? ? E8 ? ? ? ? 83 F8 01` |
| `PlayPanelPtr` | client.dll | `48 8B 1D ? ? ? ? 48 85 DB 0F 84 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 0F B7 0D ? ? ? ?` |
| `AcceptPanelPtr` | client.dll | `48 8B 1D ? ? ? ? 48 8D 15 ? ? ? ? 48 8D 8D ? ? ? ? E8 ? ? ? ? 48 8B 4B 08` |
| `GetProtoCDOTAGameAccountPlus` | client.dll | `48 83 EC 38 48 8B 89 38 05 00 00 48 85 C9 74 45 BA DC 07 00 00 E8` |
| `CGCClientSystem` | client.dll | `48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 01 FF 90 28 01 00 00` |
| `SendMessage` | client.dll | `48 89 5C 24 08 57 48 83 EC 30 48 8B F9 E8 ? ? ? ? 48 8B 97 00 06 00 00 48 8B 9F 70 16 00 00` |
| `GameEntitySystem` | client.dll | `48 8B 0D ? ? ? ? 33 D2 E8 ? ? ? ?` |
| `LocalPlayerArray` | client.dll | `48 8B 05 ? ? ? ? 89 BE` |
| `set_rendering_enabled` | particles.dll | `0F 84 ? ? ? ? 4D 89 73` |
| `set_rendering_enabled_fix` | client.dll | `0F 84 ? ? ? ? 48 8B ? 48 85 ? 74 ? 48 8B ? 48 8B` |
| `visible_by_enemy` (VBE) | client.dll | `75 ? 41 8B CE E8 ? ? ? ? 48 85 C0 74 ? 80 B8` |
| `emoticon` | client.dll | `56 57 48 83 EC ? 33 FF 8B F2` |
| `weather` | client.dll | `48 8B D9 BA ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 85 C0 75 ? 48 8B 05 ? ? ? ? 48 8B 40 ? 83 38 ? 74 ? BA ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 85 C0 75 ? 48 8B 05 ? ? ? ? 48 8B 40 ? 8B 00` |

All of these are centralized in `core/patterns.hpp`.

## Notes

- Signatures from TheXSVV (forum #28), Cr33py (forum #31-39), and Wolf49406/Dota2Patcher.
- All patterns centralized in `core/patterns.hpp` using IDA-style `?` wildcards.
- If Dota 2 updates, patterns may need updating — the patcher reports which ones it finds on startup.
- Run as administrator for `OpenProcess` to succeed.
- Use at your own risk.
