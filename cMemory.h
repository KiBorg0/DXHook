#pragma once
#include <Windows.h>
#include <cstdint>
#include <psapi.h>
#include <QDebug>
// Original function by learn_more -- https://github.com/learn-more/findpattern-bench
#define INRANGE(x, a, b) (x >= a && x <= b)
#define getBits(x) (INRANGE((x & (~0x20)), 'A', 'F') ? ((x & (~0x20)) - 'A' + 0xA): (INRANGE(x, '0', '9') ? x - '0': 0))
#define getByte(x) (getBits(x[0]) << 4 | getBits(x[1]))

#define new_My_Thread(Function) CreateThread(0,0,(LPTHREAD_START_ROUTINE)Function,0,0,0);

BOOL  bCompare(const BYTE* pData, const BYTE* bMask, const char *szMask);
DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, const char *szMask);
void *Create_Hook(BYTE *src, const BYTE *dst, const int len);

static uintptr_t FindPattern(const uintptr_t& start_address, const uintptr_t& end_address, const char* target_pattern) {
    const char* pattern = target_pattern;

    uintptr_t first_match = 0;

    for (uintptr_t position = start_address; position < end_address; position++) {
        if (!*pattern)
            return first_match;

        const uint8_t pattern_current = *reinterpret_cast<const uint8_t*>(pattern);
        const uint8_t memory_current = *reinterpret_cast<const uint8_t*>(position);

        if (pattern_current == '\?' || memory_current == getByte(pattern)) {
            if (!first_match)
                first_match = position;

            if (!pattern[2])
                return first_match;

            pattern += pattern_current != '\?' ? 3 : 2;
        }
        else {
            pattern = target_pattern;
            first_match = 0;
        }
    }

    return NULL;
}

static uintptr_t FindPattern(const char* module, const char* target_pattern) {
    MODULEINFO module_info = { 0, 0, 0 };
    HMODULE hmodule = 0;
    while (!hmodule) hmodule = GetModuleHandleA(module);
//    qDebug() << hmodule;
    if (!GetModuleInformation(GetCurrentProcess(), hmodule, &module_info, sizeof(MODULEINFO)))
        return NULL;

    const uintptr_t start_address = uintptr_t(module_info.lpBaseOfDll);
    const uintptr_t end_address = start_address + module_info.SizeOfImage;

    return FindPattern(start_address, end_address, target_pattern);
}

