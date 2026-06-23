#include "memory.hpp"

#include <fstream>
#include <cstring>

bool Memory::open(DWORD pid, DWORD desired) {
    if (h_process)
        CloseHandle(h_process);
    h_process = OpenProcess(desired, FALSE, pid);
    return h_process != nullptr;
}

void Memory::close() {
    if (h_process) {
        CloseHandle(h_process);
        h_process = nullptr;
    }
}

bool Memory::load_modules(DWORD pid) {
    modules.clear();

    const char* needed[] = {
        "client.dll",
        "server.dll",
        "engine2.dll",
        "schemasystem.dll",
        "particles.dll",
        "tier0.dll"
    };

    std::vector<std::string> missing(std::begin(needed), std::end(needed));

    while (!missing.empty()) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
        if (snap == INVALID_HANDLE_VALUE)
            return false;

        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);

        if (Module32FirstW(snap, &me)) {
            do {
                std::wstring wmod(me.szModule);
                std::string mod(wmod.begin(), wmod.end());

                auto it = std::find(missing.begin(), missing.end(), mod);
                if (it != missing.end()) {
                    uintptr_t base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                    modules[mod] = {
                        base,
                        base + static_cast<uintptr_t>(me.modBaseSize),
                        static_cast<size_t>(me.modBaseSize),
                        std::wstring(me.szExePath)
                    };
                    missing.erase(it);
                }
            } while (Module32NextW(snap, &me) && !missing.empty());
        }

        CloseHandle(snap);

        if (!missing.empty())
            Sleep(1000);
    }

    return true;
}

std::vector<std::pair<uint8_t, bool>> Memory::parse_pattern(const std::string& pattern) {
    std::vector<std::pair<uint8_t, bool>> out;
    std::stringstream ss(pattern);
    std::string tok;

    while (ss >> tok) {
        if (tok == "?" || tok == "??" || tok == "x" || tok == "X") {
            out.emplace_back(0, false);
        } else {
            try {
                unsigned int b = std::stoul(tok, nullptr, 16);
                out.emplace_back(static_cast<uint8_t>(b), true);
            } catch (...) {
                out.emplace_back(0, false);
            }
        }
    }

    return out;
}

std::optional<uintptr_t> Memory::pattern_scan(const std::string& module, const std::string& pattern) {
    auto it = modules.find(module);
    if (it == modules.end())
        return std::nullopt;

    const auto& m = it->second;
    auto pat = parse_pattern(pattern);
    if (pat.empty())
        return std::nullopt;

    const size_t pat_len = pat.size();
    const size_t chunk_size = 4 * 1024 * 1024;
    std::vector<uint8_t> buffer(chunk_size + pat_len);

    for (uintptr_t addr = m.start; addr < m.end; addr += chunk_size) {
        size_t remaining = static_cast<size_t>(m.end - addr);
        size_t to_read = std::min(chunk_size + pat_len, remaining);
        SIZE_T r = 0;
        if (!ReadProcessMemory(h_process, reinterpret_cast<LPCVOID>(addr), buffer.data(), to_read, &r) || r == 0)
            continue;

        size_t scan_len = std::min(chunk_size, static_cast<size_t>(r));
        for (size_t i = 0; i + pat_len <= scan_len; ++i) {
            bool ok = true;
            for (size_t j = 0; j < pat_len; ++j) {
                if (pat[j].second && buffer[i + j] != pat[j].first) {
                    ok = false;
                    break;
                }
            }
            if (ok)
                return addr + i;
        }
    }

    // Process scan failed; try the on-disk module as fallback.
    // This is needed when Dota 2's copy of the module is readable on disk
    // but ReadProcessMemory fails (e.g., protected pages).
    std::ifstream file(m.path.c_str(), std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> file_data(file_size);
        if (file.read(reinterpret_cast<char*>(file_data.data()), file_size)) {
            // Parse PE
            if (file_size >= 0x40) {
                uint32_t pe_offset = *reinterpret_cast<uint32_t*>(file_data.data() + 0x3C);
                if (pe_offset + 24u <= static_cast<size_t>(file_size)) {
                    uint16_t num_sections = *reinterpret_cast<uint16_t*>(file_data.data() + pe_offset + 6);
                    uint16_t opt_header_size = *reinterpret_cast<uint16_t*>(file_data.data() + pe_offset + 20);
                    size_t section_table_offset = pe_offset + 24 + opt_header_size;
                    if (section_table_offset + num_sections * 40u <= static_cast<size_t>(file_size)) {
                        uint32_t image_size = *reinterpret_cast<uint32_t*>(file_data.data() + pe_offset + 24 + 56);
                        std::vector<uint8_t> image(image_size, 0);
                        for (uint16_t s = 0; s < num_sections; ++s) {
                            size_t off = section_table_offset + s * 40;
                            uint32_t vsize = *reinterpret_cast<uint32_t*>(file_data.data() + off + 8);
                            uint32_t vaddr = *reinterpret_cast<uint32_t*>(file_data.data() + off + 12);
                            uint32_t raw_size = *reinterpret_cast<uint32_t*>(file_data.data() + off + 16);
                            uint32_t raw_ptr = *reinterpret_cast<uint32_t*>(file_data.data() + off + 20);
                            size_t copy_size = std::min(vsize, raw_size);
                            if (raw_ptr + copy_size > static_cast<size_t>(file_size))
                                copy_size = file_size - raw_ptr;
                            if (copy_size > 0 && vaddr + copy_size <= image.size())
                                std::memcpy(image.data() + vaddr, file_data.data() + raw_ptr, copy_size);
                        }
                        for (size_t i = 0; i + pat_len <= image.size(); ++i) {
                            bool ok = true;
                            for (size_t j = 0; j < pat_len; ++j) {
                                if (pat[j].second && image[i + j] != pat[j].first) {
                                    ok = false;
                                    break;
                                }
                            }
                            if (ok)
                                return m.start + i;
                        }
                    }
                }
            }
        }
    }

    return std::nullopt;
}

std::optional<uintptr_t> Memory::absolute_address(uintptr_t instr, uint8_t offset, uint8_t size) {
    int32_t rel = 0;
    SIZE_T r = 0;
    if (!ReadProcessMemory(h_process, reinterpret_cast<LPCVOID>(instr + offset), &rel, sizeof(rel), &r) || r != sizeof(rel))
        return std::nullopt;
    return instr + rel + size;
}

bool Memory::read_buffer(uintptr_t addr, void* out, size_t size) {
    SIZE_T r = 0;
    return addr && ReadProcessMemory(h_process, reinterpret_cast<LPCVOID>(addr), out, size, &r) && r == size;
}

bool Memory::write(uintptr_t addr, const void* data, size_t size) {
    SIZE_T w = 0;
    return addr && WriteProcessMemory(h_process, reinterpret_cast<LPVOID>(addr), data, size, &w) && w == size;
}

bool Memory::protect(uintptr_t addr, size_t size, DWORD prot, DWORD* old) {
    DWORD tmp;
    return VirtualProtectEx(h_process, reinterpret_cast<LPVOID>(addr), size, prot, old ? old : &tmp);
}

bool Memory::patch(uintptr_t addr, const std::vector<uint8_t>& bytes) {
    DWORD old;
    if (!protect(addr, bytes.size(), PAGE_EXECUTE_READWRITE, &old))
        return false;

    bool ok = write(addr, bytes.data(), bytes.size());
    protect(addr, bytes.size(), old, &old);
    return ok;
}

bool Memory::patch(uintptr_t addr, const std::string& hex_pattern) {
    auto pat = parse_pattern(hex_pattern);
    std::vector<uint8_t> bytes;
    for (auto& p : pat)
        bytes.push_back(p.first);
    return patch(addr, bytes);
}
