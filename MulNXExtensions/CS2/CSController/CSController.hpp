#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

#include "ConVarSystem/ConVarSystem.hpp"
#include "GlobalVars/GlobalVars.hpp"
#include "List/C_BaseEntity.hpp"

#include "C_CSGameRules/C_CSGameRules.hpp"
#include "Client/Client.hpp"

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

    std::atomic<int> WindowWidth = 1920;
    std::atomic<int> WindowHeight = 1080;
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

class ControlAdvancedView{
public:
    std::atomic<bool> Enable = false;
    
    std::atomic<int> boneIndex1 = 8;
    std::atomic<int> boneIndex2 = 9;
    std::atomic<int> boneIndex3 = 10;

    std::atomic<float> distance = 50.0f;
};

class CSController final :public MulNX::IAbstractLayer3D {
private:
    C_Modules Modules{};

    ControlSmoke controlSmoke{};
    ControlView controlView{};
    ControlAdvancedView controlAdvancedView{};


    // 逆向层关键接口
    void* Source2EngineToClient001 = nullptr;
    VExecutor<void(int, const char*, int)> executor{};
    // 逆向层数据备份
    C_ConVarSystem CvarSystem{};
    C_GlobalVars CSGlobalVars{};
    
    void ESP();
public:
    std::atomic<bool> ESPDraw = false;
    
    std::unique_ptr<MulNX::Memory::HookEx> MyHook = nullptr;
    void HandleOverrideView(CS2::CViewSetup* viewSetup);
    //bool UINodeFunc(MulNXUINode* ThisNode)override;
    bool Init()override;
    bool UINodeFunc(MulNXUINode* node);
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
};