#pragma once

#include "../Region/Region.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <Windows.h>
#include <Psapi.h>

namespace MulNX {
    namespace Memory {
        class DllModule {
            bool Valid = false;
            HMODULE hModule = nullptr;
            MODULEINFO modInfo = { 0 };
            std::wstring Name;
        public:
            DllModule() = delete;
            DllModule(std::wstring&& Name);

            uintptr_t GetBaseAddress()const;
            size_t GetModuleSize()const;
            bool IsValid()const { return this->Valid; }

            Region GetTextRegion()const;
        };
        bool GetModuleInfo(const wchar_t* moduleName, uintptr_t& baseAddr, size_t& moduleSize);
        bool GetTextSectionRange(uintptr_t moduleBase, uintptr_t& textStart, size_t& textSize);
    }
}