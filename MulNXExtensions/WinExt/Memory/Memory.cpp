#include "Memory.hpp"

#include <stdexcept>

bool MulNX::Memory::ReadString(const uintptr_t Address, char* Buffer, size_t BufferSize) {
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
bool MulNX::Memory::ReadWString(const uintptr_t Address, wchar_t* Buffer, size_t BufferCount) {
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

bool MulNX::Memory::GetModuleInfo(const wchar_t* moduleName, uintptr_t& baseAddr, size_t& moduleSize) {
    HMODULE hModule = GetModuleHandleW(moduleName);
    if (!hModule) return false;

    MODULEINFO modInfo = { 0 };
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
        return false;

    baseAddr = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    moduleSize = modInfo.SizeOfImage;
    return true;
}
bool MulNX::Memory::GetTextSectionRange(uintptr_t moduleBase, uintptr_t& textStart, size_t& textSize) {
    // DOS头
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)moduleBase;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

    // NT头
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(moduleBase + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return false;

    // 段表起始位置
    IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        // 检查段是否包含可执行代码（IMAGE_SCN_MEM_EXECUTE 标志）
        if (sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            textStart = moduleBase + sections[i].VirtualAddress;
            textSize = sections[i].Misc.VirtualSize;
            return true;
        }
    }
    return false;
}



std::optional<uint8_t*> MulNX::Memory::Accessor::FindHead(const uint8_t* Begin, const uint8_t* End, const uint8_t Byte) {
    for (const uint8_t* Current = Begin; Current < End; ++Current) {
        if (*Current == Byte) {
            return const_cast<uint8_t*>(Current);
        }
    }
    return std::nullopt;
}

bool MulNX::Memory::Accessor::MatchPattern(const uint8_t* Address, const Pattern& Pattern) {
    for (size_t i = 0; i < Pattern.size(); ++i) {
        if (Pattern[i].has_value() && Address[i] != Pattern[i].value()) {
            return false;//当前字节不匹配
        }
    }
    return true;//完全匹配
}

MulNX::Memory::Region MulNX::Memory::Accessor::FindRegion(const Region& Region, const Pattern& Pattern) {
    for (const uint8_t* Current = Region.begin(); Current < Region.end();) {
        auto FoundHead = MulNX::Memory::Accessor::FindHead(Current, Region.end(), *Pattern.begin());
        if (!FoundHead.has_value()) {
            break;//未找到更多匹配
        }
        if (MulNX::Memory::Accessor::MatchPattern(FoundHead.value(), Pattern)) {
            //找到匹配，返回对应内存区域
            return MulNX::Memory::Region(reinterpret_cast<uintptr_t>(FoundHead.value()), Pattern.size());
        }
        //继续寻找下一个匹配头
        Current = FoundHead.value() + 1;
    }
    return Region::InValid();//未找到匹配
}