#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

#include "ConVarSystem/ConVarSystem.hpp"
#include "EntityList/EntityList.hpp"
#include "GlobalVars/GlobalVars.hpp"
#include "PlantedC4/PlantedC4.hpp"
#include "List/C_BaseEntity.hpp"

#include "C_CSGameRules/C_CSGameRules.hpp"

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
            // 常常用于获取控制器
            CS2::C_BaseEntity* GetBaseEntity(int index) {
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
            // 常常用于获取Pawn对象
            CS2::C_BaseEntity* GetBaseEntityFromHandle(uint32_t uHandle) {
                const int nIndex = uHandle & 0x7FFF;
                return this->GetBaseEntity(nIndex);
            }
        };
    }
}

class C_Modules {
public:
    CS2::Module::Client client{};
    MulNX::Memory::DllModule engine2{};
    MulNX::Memory::DllModule tier0{};
};

class Views {
public:
    std::atomic<float> OriginX = 0;
    std::atomic<float> OriginY = 0;
    std::atomic<float> OriginZ = 0;
    std::atomic<float> AnglesX = 0;
    std::atomic<float> AnglesY = 0;
    std::atomic<float> AnglesZ = 0;
    std::atomic<float> FOV = 90.0f;
};

class ControlSmoke {
public:
    std::atomic<bool> Enbale = false;
    std::atomic<bool> Show = true;
    std::atomic<float> R = 127;
    std::atomic<float> G = 127;
    std::atomic<float> B = 127;
};

class ControlView{
public:
    std::atomic<std::shared_ptr<Views>> ViewToGame = nullptr;
    std::atomic<float> InputRoll = 0;
    std::atomic<bool> CameraMode = false;
    Views currentView{};
};

class CSController final :public MulNX::IAbstractLayer3D {
private:
    ControlSmoke controlSomke{};
    ControlView controlView{};
    // 逆向层关键接口
    void* Source2EngineToClient001 = nullptr;
    VExecutor<void(int, const char*, int)> executor{};
    // 逆向层数据备份
    C_ConVarSystem CvarSystem{};
    C_GlobalVars CSGlobalVars{};
    C_PlantedC4 PlantedC4{};
    C_EntityList EntityList{};
    C_Modules Modules{};

    // 索引映射（小地图<->游戏实体列表）
    std::shared_mutex IndexMapMtx{};
    std::unordered_map<int, int>IndexInMap_To_IndexInEntityList_Map{};
    int GetIndexInEntityListFromIndexInMap(int IndexInMap);

    void ESP();
public:
    std::atomic<bool> ESPDraw = false;
    
    std::unique_ptr<MulNX::Memory::HookEx> MyHook = nullptr;
    void HandleOverrideView(void* ThisCViewSetup);
    //bool UINodeFunc(MulNXUINode* ThisNode)override;
    bool Init()override;
    bool UINodeFunc(MulNXUINode* node)override;
    void VirtualMain()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    void ThreadMain()override;
    // 核心任务
    std::atomic<int> GetMsgResult = 0;
    int TryGetMsg();
    // 子任务集合
    int BasicUpdate();
    int EntityListUpdate();


    // 核心接口
    bool ExecuteCommand(const std::string& cmd)override;
    float* GetViewMatrix()override;
    MulNX::Math::View GetView()const override;
    float GetTime()const override;
    float GetWinWidth()const override;
    float GetWinHeight()const override;
    bool SpecPlayer(int IndexInMap)override;
    D_Player& GetPlayerMsg(int Index)override;
    void spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot)override;
    void ClearViewOverride()override;

    // CameraSystemIO的处理

    void HandleFreeCameraPath(const CameraSystemIO* const IO);
    void HandleFirstPersonCameraPath(const CameraSystemIO* const IO);
    bool CameraSystemIOOverride(const CameraSystemIO* const IO)override;

    // 获取控制台变量系统
    C_ConVarSystem& GetCvarSystem() { return this->CvarSystem; }
    // 获取Entity引用，注意，内部读取还是拷贝
    C_EntityList& GetEntityList() { return this->EntityList; }

    void HandleAimAtEntity(int AimTargetIndexInMap);
};