#pragma once

#include "DllModule/DllModule.hpp"
#include "Pattern/Pattern.hpp"
#include "Region/Region.hpp"
#include "Assembler/Assembler.hpp"
#include "Hook/Hook.hpp"

#include <atomic>
#include <string>
#include <vector>
#include <optional>
#include <Windows.h>

namespace MulNX {
    namespace Memory {
        namespace ReadWrite {
            class AccessError :public std::runtime_error {
            public:
                explicit AccessError(const std::string& msg)
                    : std::runtime_error(msg) {}
            };

            class bad_memory_read :public AccessError {
            public:
                explicit bad_memory_read(const std::string& msg)
                    : AccessError(msg) {}
            };

            class bad_memory_write :public AccessError {
            public:
                explicit bad_memory_write(const std::string& msg)
                    : AccessError(msg) {}
            };

            template<typename T>
            static bool ReadImpl(uintptr_t address, T& target) {
                __try {
                    target = *reinterpret_cast<T*>(address);
                    return true;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return false;
                }
            }

            template<typename T>
            T MRead(uintptr_t address) {
                T target;
                if (!ReadImpl(address, target)) {
                    throw bad_memory_read("access error");
                }
                else {
                    return target;
                }
            }
            template<typename T>
            T MRead(T* address) {
                T target;
                if (!ReadImpl(reinterpret_cast<uintptr_t>(address), target)) {
                    throw bad_memory_read("access error");
                }
                else {
                    return target;
                }
            }

            template<typename T>
            static bool WriteImpl(uintptr_t address, const T& value) {
                __try {
                    *reinterpret_cast<T*>(address) = value;
                    return true;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    return false;
                }
            }

            template<typename T>
            void MWrite(uintptr_t address, const T& value) {
                if (!WriteImpl(address, value)) {
                    throw bad_memory_write("access error");
                }
            }

            template<typename T>
            void MWrite(T* address, const T& value) {
                if (!WriteImpl(reinterpret_cast<uintptr_t>(address), value)) {
                    throw bad_memory_write("access error");
                }
            }
        }
        // 安全读取字符串（ANSI/UTF-8），逐字节读取直到遇到空字符或达到缓冲大小
        bool ReadString(const uintptr_t Address, char* Buffer, size_t BufferSize);
        // 安全读取宽字符串（UTF-16），逐字符读取直到遇到空字符或达到缓冲字符数
        bool ReadWString(const uintptr_t Address, wchar_t* Buffer, size_t BufferCount);
    }
    using namespace Memory::ReadWrite;
}