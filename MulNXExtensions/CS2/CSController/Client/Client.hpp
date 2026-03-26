#pragma once

#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/CSController/List/C_BaseEntity.hpp>
#include <MulNXExtensions/CS2/CSController/C_CSGameRules/C_CSGameRules.hpp>

namespace CS2 {
    namespace Module {
        class Client :public MulNX::Memory::DllModule {
        public:
            using MulNX::Memory::DllModule::DllModule;
            uintptr_t dwEntityList() { return MulNX::MRead<uintptr_t>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwEntityList); }
            uintptr_t dwGameEntitySystem() { return MulNX::MRead<uintptr_t>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGameEntitySystem); }
            int dwGameEntitySystem_highestEntityIndex() { return MulNX::MRead<int>(this->dwGameEntitySystem() + cs2_dumper::offsets::client_dll::dwGameEntitySystem_highestEntityIndex); }
            CS2::C_CSGameRules* dwGameRules() { return MulNX::MRead<CS2::C_CSGameRules*>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGameRules); }
            float* dwViewMatrix() { return reinterpret_cast<float*>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwViewMatrix); }
            CS2::CCSPlayerController* dwLocalPlayerController() { return MulNX::MRead(reinterpret_cast<CS2::CCSPlayerController**>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwLocalPlayerController)); }
            CS2::C_CSPlayerPawn* dwLocalPlayerPawn() { return MulNX::MRead(reinterpret_cast<CS2::C_CSPlayerPawn**>(this->GetBaseAddress() + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn)); }

            // 常常用于获取控制器
            CS2::C_BaseEntity* GetBaseEntity(int index);
            // 常常用于获取Pawn对象
            CS2::C_BaseEntity* GetBaseEntityFromHandle(uint32_t uHandle);
        };
    }
}