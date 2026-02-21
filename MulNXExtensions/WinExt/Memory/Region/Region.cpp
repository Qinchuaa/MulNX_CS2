#include "Region.hpp"

MulNX::Memory::Region::ProtectionGuard::ProtectionGuard(Region* pRegion) {
    if (!pRegion || !pRegion->IsValid()) {
        this->Valid = false;
        return;
    }
    this->Valid = true;
    this->pRegion = pRegion;
}

MulNX::Memory::Region::ProtectionGuard::~ProtectionGuard() {
    if (!this->IsValid()) return;
    VirtualProtect(reinterpret_cast<LPVOID>(this->pRegion->Base), this->pRegion->Size, this->pRegion->OldProtection, &this->pRegion->Protection);
}
bool MulNX::Memory::Region::ProtectionGuard::IsValid() const {
    return this->Valid;
}
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