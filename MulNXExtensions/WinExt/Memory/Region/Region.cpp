#include "Region.hpp"

MulNX::Memory::Region::Region(uintptr_t Base, size_t Size) : Base(Base), Size(Size) {
    this->Protection = 0;
    if (!this->IsValid()) return;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(reinterpret_cast<LPCVOID>(Base), &mbi, sizeof(mbi)) == 0) {
        this->Base = 0;
        this->Size = 0;
        return;
    }
    this->Protection = mbi.Protect;
}
MulNX::Memory::Region::ProtectionGuard MulNX::Memory::Region::ExchangeProtection(DWORD NewProtect) {
    if (!this->IsValid()) return ProtectionGuard::Invalid();
    if (!VirtualProtect(reinterpret_cast<LPVOID>(this->Base), this->Size, NewProtect, &this->OldProtection))
        return ProtectionGuard::Invalid();

    this->Protection = NewProtect;
    return ProtectionGuard(this);
}

std::optional<uint8_t*> MulNX::Memory::Region::FindHead(const uint8_t* Begin, const uint8_t Byte)const {
    for (const uint8_t* Current = Begin; Current < this->End(); ++Current) {
        if (*Current == Byte) {
            return const_cast<uint8_t*>(Current);
        }
    }
    return std::nullopt;
}

bool MulNX::Memory::Region::MatchPattern(const uint8_t* Address, const Pattern& Pattern)const {
    for (size_t i = 0; i < Pattern.size(); ++i) {
        if (Pattern[i].has_value() && Address[i] != Pattern[i].value()) {
            return false;//当前字节不匹配
        }
    }
    return true;//完全匹配
}

MulNX::Memory::Region MulNX::Memory::Region::FindRegion(const Pattern& pattern) const {
    for (const uint8_t* Current = this->Begin(); Current < this->End();) {
        auto FoundHead = this->FindHead(Current, *pattern.First());
        if (!FoundHead.has_value()) {
            break;//未找到更多匹配
        }
        if (this->MatchPattern(FoundHead.value(), pattern)) {
            //找到匹配，返回对应内存区域
            return MulNX::Memory::Region(reinterpret_cast<uintptr_t>(FoundHead.value()), pattern.size());
        }
        //继续寻找下一个匹配头
        Current = FoundHead.value() + 1;
    }
    return Region::InValid();//未找到匹配
}

bool MulNX::Memory::Region::SameSizeSwap(const Asm::Code& Source) {
    if (this->GetSize() != Source.Size()) {
        throw "Size Error";
    }
    memcpy(static_cast<void*>(this->Data()), Source.Data(), Source.Size());
    return true;
}
bool MulNX::Memory::Region::TryResize(size_t NewSize) {
    if (this->Size < NewSize) {
        // 不能扩大
        return false;
    }
    this->Size = NewSize;
    return true;
}