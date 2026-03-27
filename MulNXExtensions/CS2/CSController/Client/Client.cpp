#include "Client.hpp"

#pragma once

#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/CSController/List/C_BaseEntity.hpp>
#include <MulNXExtensions/CS2/CSController/C_CSGameRules/C_CSGameRules.hpp>

CS2::C_BaseEntity* CS2::Module::Client::GetBaseEntity(int index) {
    uintptr_t entListBase = MulNX::MRead<uintptr_t>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwEntityList);
    if (entListBase == 0) {
        return 0;
    }
    uintptr_t entityListBase = MulNX::MRead<uintptr_t>(entListBase + 0x8 * (index >> 9) + 0x10);
    if (entityListBase == 0) {
        return 0;
    }
    return MulNX::MRead<CS2::C_BaseEntity*>(entityListBase + (0x70 * (index & 0x1FF)));
}

CS2::C_BaseEntity* CS2::Module::Client::GetBaseEntityFromHandle(uint32_t uHandle) {
    const int nIndex = uHandle & 0x7FFF;
    return this->GetBaseEntity(nIndex);
}

CS2::C_BaseEntity* CS2::Module::Client::GetBaseEntityFromHandle(CS2::CHandleBase handle) {
    if (!handle.Valid())return nullptr;
    return this->GetBaseEntity(handle.GetIndexInEntityList());
}