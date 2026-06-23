#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <initializer_list>

class Memory {
public:
    struct ModuleInfo {
        uintptr_t start;
        uintptr_t end;
        size_t    size;
        std::wstring path;
    };

    static inline HANDLE h_process = nullptr;
    static inline std::unordered_map<std::string, ModuleInfo> modules;

    static bool open(DWORD pid, DWORD desired = PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION);
    static void close();
    static bool load_modules(DWORD pid);

    static std::optional<uintptr_t> pattern_scan(const std::string& module, const std::string& pattern);
    static std::optional<uintptr_t> absolute_address(uintptr_t instr, uint8_t offset = 3, uint8_t size = 7);

    template<typename T>
    static std::optional<T> read(uintptr_t addr) {
        T v{};
        SIZE_T r = 0;
        if (!addr || !ReadProcessMemory(h_process, reinterpret_cast<LPCVOID>(addr), &v, sizeof(T), &r) || r != sizeof(T))
            return std::nullopt;
        return v;
    }

    static bool read_buffer(uintptr_t addr, void* out, size_t size);

    template<typename T>
    static bool write(uintptr_t addr, const T& v) {
        SIZE_T w = 0;
        return addr && WriteProcessMemory(h_process, reinterpret_cast<LPVOID>(addr), &v, sizeof(T), &w) && w == sizeof(T);
    }

    static bool write(uintptr_t addr, const void* data, size_t size);
    static bool protect(uintptr_t addr, size_t size, DWORD prot, DWORD* old = nullptr);
    static bool patch(uintptr_t addr, const std::vector<uint8_t>& bytes);
    static bool patch(uintptr_t addr, const std::string& hex_pattern);
    static bool patch(uintptr_t addr, std::initializer_list<uint8_t> bytes) {
        return patch(addr, std::vector<uint8_t>(bytes));
    }

private:
    static std::vector<std::pair<uint8_t, bool>> parse_pattern(const std::string& pattern);
};
