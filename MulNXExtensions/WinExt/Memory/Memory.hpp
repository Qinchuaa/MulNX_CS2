#pragma once

#include <atomic>
#include <string>
#include <vector>
#include <optional>
#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

namespace MulNX {
    namespace Memory {
        // 安全访问，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
        template<typename T>
        inline bool Read(const uintptr_t Address, T& Value) {
            __try {
                Value = *reinterpret_cast<T*>(Address);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }
        // 安全访问，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
        template<typename T>
        inline bool Read(const T* Address, T& Value) {
            __try {
                Value = *Address;
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }
        // 安全访问，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
        template<typename T>
        inline bool Read(const uintptr_t Address, std::atomic<T>& Value) {
            __try {
                Value.store(*reinterpret_cast<T*>(Address));
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }
        // 安全访问，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
        template<typename T>
        inline T Read(const uintptr_t Address) {
            __try {
                return *reinterpret_cast<T*>(Address);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return T{};
            }
        }
        // 安全写入，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
        template<typename T>
        inline bool Write(T* Address, const T& Value) {
            __try {
                *Address = Value;
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }
        // 安全写入，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
        template<typename T>
        inline bool Write(const uintptr_t Address, const T& Value) {
            __try {
                *reinterpret_cast<T*>(Address) = Value;
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }
        // 安全读取字符串（ANSI/UTF-8），逐字节读取直到遇到空字符或达到缓冲大小
        bool ReadString(const uintptr_t Address, char* Buffer, size_t BufferSize);
        // 安全读取宽字符串（UTF-16），逐字符读取直到遇到空字符或达到缓冲字符数
        bool ReadWString(const uintptr_t Address, wchar_t* Buffer, size_t BufferCount);

        bool GetModuleInfo(const wchar_t* moduleName, uintptr_t& baseAddr, size_t& moduleSize);
        bool GetTextSectionRange(uintptr_t moduleBase, uintptr_t& textStart, size_t& textSize);

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
        public:
            Region(uintptr_t Base, size_t Size);
            static Region InValid() { return Region(0, 0); }
            // 迭代器支持
            const uint8_t* begin() const { return reinterpret_cast<const uint8_t*>(Base); }
            const uint8_t* end() const { return reinterpret_cast<const uint8_t*>(Base + Size); }
            size_t size() const { return Size; }
            bool IsValid() const { return Base != 0 && Size != 0; }
            DWORD protection() const { return Protection; }
            ProtectionGuard ExchangeProtection(DWORD NewProtect);
        };

        // 内存模式类，表示一个特定的字节模式，包含通配符为?，提供匹配功能
        class Pattern {
            std::string Raw;// 模式字符串，格式如 "48 8B ?? ?? ?? 48 85 C0"
            std::vector<std::optional<uint8_t>> Bytes;// 解析后的字节数组，其中std::nullopt表示通配符
        public:
            Pattern(std::string&& Raw);
            const uint8_t* begin() const { return reinterpret_cast<const uint8_t*>(Bytes.data()); }
            const uint8_t* end() const { return reinterpret_cast<const uint8_t*>(Bytes.data() + Bytes.size()); }
            size_t size() const { return Bytes.size(); }
            std::optional<uint8_t> operator[](size_t index) const { return Bytes[index]; }
        };
        class Accessor {
        private:
            static std::optional<uint8_t*> FindHead(const uint8_t* Begin, const uint8_t* End, const uint8_t Byte);
            static bool MatchPattern(const uint8_t* Address, const Pattern& Pattern);
        public:
            static Region FindRegion(const Region& Region, const Pattern& Pattern);
        };
    }
}