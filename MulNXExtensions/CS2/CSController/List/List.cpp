#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>


CS2::C_BaseEntity* CSController::GetBaseEntity(int index) {
    uintptr_t entListBase = *reinterpret_cast<uintptr_t*>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwEntityList);
    if (entListBase == 0) {
        return 0;
    }
    uintptr_t entityListBase = *reinterpret_cast<uintptr_t*>(entListBase + 0x8 * (index >> 9) + 0x10);
    if (entityListBase == 0) {
        return 0;
    }
    return *reinterpret_cast<CS2::C_BaseEntity**>(entityListBase + (0x70 * (index & 0x1FF)));
}

CS2::C_BaseEntity* CSController::GetBaseEntityFromHandle(uint32_t uHandle) {
    const int nIndex = uHandle & 0x7FFF;
    return this->GetBaseEntity(nIndex);
}