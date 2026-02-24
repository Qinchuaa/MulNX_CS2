#pragma once

#include "../Pattern/Pattern.hpp"
#include "../Assembler/Assembler.hpp"

#include <Windows.h>

namespace MulNX {
    namespace Memory {
        // 内存区域类，表示一个连续的内存块，提供迭代器支持
        // 左闭右开区间 [Base, End)，Size = End - Base
        class Region {
        private:
            uintptr_t Base = 0;
            size_t Size = 0;
            DWORD Protection = 0;// 当前保护属性
            DWORD OldProtection = 0;// 交换保护时保存旧属性
        public:
            class ProtectionGuard {
            private:
                Region* pRegion = nullptr;
                bool Valid = false;
                ProtectionGuard() = default;
            public:
                ProtectionGuard(Region* Region);
                ~ProtectionGuard();

                bool IsValid()const;
                static ProtectionGuard Invalid() { return MulNX::Memory::Region::ProtectionGuard(); }
            };
            friend class ProtectionGuard;
            std::optional<uint8_t*> FindHead(const uint8_t* Begin, const uint8_t Byte)const;
            bool MatchPattern(const uint8_t* Address, const Pattern& Pattern)const;
        public:
            Region() = delete;
            Region(uintptr_t Base, size_t Size);
            static Region InValid() { return Region(0, 0); }
            // 迭代器支持
            const uint8_t* Begin()const { return reinterpret_cast<const uint8_t*>(Base); }
            const uint8_t* End()const { return reinterpret_cast<const uint8_t*>(Base + Size); }
            size_t GetSize() const { return this->Size; }
            uint8_t* Data() { return reinterpret_cast<uint8_t*>(Base); }
            bool IsValid() const { return Base != 0 && Size != 0; }
            DWORD protection() const { return Protection; }
            ProtectionGuard ExchangeProtection(DWORD NewProtect);

            Region FindRegion(const Pattern& pattern)const;
            bool DataOverride(const Asm::Code& AsmCode);
        };
    }
}