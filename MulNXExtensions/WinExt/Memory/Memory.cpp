#include "Memory.hpp"

bool MulNX::Base::Memory::ReadString(const uintptr_t Address, char* Buffer, size_t BufferSize) {
    if (Buffer == nullptr || BufferSize == 0) {
        return false;
    }
    __try {
        for (size_t i = 0; i < BufferSize; ++i) {
            char c = *reinterpret_cast<const char*>(Address + i);
            Buffer[i] = c;
            if (c == '\0') {
                return true;//成功读取到完整字符串
            }
        }
        //缓冲区不够，确保以空字符结尾
        Buffer[BufferSize - 1] = '\0';
        return false;//字符串被截断，但已安全处理
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Buffer[0] = '\0';//确保缓冲区为空字符串
        return false;
    }
}
bool MulNX::Base::Memory::ReadWString(const uintptr_t Address, wchar_t* Buffer, size_t BufferCount) {
    if (Buffer == nullptr || BufferCount == 0) {
        return false;
    }
    __try {
        for (size_t i = 0; i < BufferCount; ++i) {
            wchar_t c = *reinterpret_cast<const wchar_t*>(Address + i * sizeof(wchar_t));
            Buffer[i] = c;
            if (c == L'\0') {
                return true;//成功读取到完整字符串
            }
        }
        //缓冲区不够，确保以空字符结尾
        Buffer[BufferCount - 1] = L'\0';
        return false;//字符串被截断
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Buffer[0] = L'\0';//确保缓冲区为空字符串
        return false;
    }
}