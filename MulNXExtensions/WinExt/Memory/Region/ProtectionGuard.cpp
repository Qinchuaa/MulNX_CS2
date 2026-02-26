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
    VirtualProtect(reinterpret_cast<LPVOID>(this->pRegion->Base), this->pRegion->RawSize, this->pRegion->OldProtection, &this->pRegion->Protection);
}
bool MulNX::Memory::Region::ProtectionGuard::IsValid() const {
    return this->Valid;
}