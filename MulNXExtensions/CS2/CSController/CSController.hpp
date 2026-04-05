#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>

#include "ConVarSystem/ConVarSystem.hpp"
#include "GlobalVars/GlobalVars.hpp"
#include "List/C_BaseEntity.hpp"

#include "C_CSGameRules/C_CSGameRules.hpp"
#include "Client/Client.hpp"

#include <MulNXExtensions/CS2/AdvancedViewController/AdvancedViewController.hpp>

#include <expected>

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

class Dofs{
public:
    float* pNearBlurry = nullptr;
    float* pNearCrisp = nullptr;
    float* pFarCrisp = nullptr;
    float* pFarBlurry = nullptr;
};

class ControlView {
public:
    std::atomic<std::shared_ptr<Views>> ViewToGame = nullptr;
    std::atomic<float> InputRoll = 0;
    std::atomic<bool> CameraMode = false;
    Views currentView{};

    Dofs dofs{};
};

class CSController final :public MulNX::IAbstractLayer3D {
private:
    ControlSmoke controlSmoke{};
    ControlView controlView{};

    AdvancedViewController* pAdvancedViewController = nullptr;
    
    // 自由摄像机控制
    std::atomic<bool> EnableFreeCameraControl = false;

    // 逆向层关键接口
    void* Source2EngineToClient001 = nullptr;
    VExecutor<void(int, const char*, int)> executor{};
    // 逆向层数据备份
    C_ConVarSystem CvarSystem{};
    C_GlobalVars* CSGlobalVars{};
    
    void ESP();
public:
    C_Modules Modules{};
    std::atomic<bool> ESPDraw = false;
    
    std::unique_ptr<MulNX::Memory::HookEx> MyHook = nullptr;
    void HandleOverrideView(CS2::CViewSetup* viewSetup);

    CS2::C_CSPlayerPawn* GetSelfViewTargetPawn();
    std::expected<MulNX::Math::Point3, int> GetPoint3(CS2::CViewSetup* viewSetup);
    void HandleCameraSystemPlay(CS2::CViewSetup* viewSetup);

    bool Init()override;
    bool UINodeFunc(MulNXUINode* node);
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
    float GetTime()override;
    bool JumpTime(const float time)override;
    float GetWinWidth()const override;
    float GetWinHeight()const override;
    bool SpecPlayer(int IndexInMap)override;
    D_Player& GetPlayerMsg(int Index)override;
    void spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot)override;
    void ClearViewOverride()override;
    void SetDOF(const MulNX::Math::DOFParam& dof)override;

    // CameraSystemIO的处理

    void HandleFreeCameraPath(const CameraSystemIO* const IO);
    void HandleFirstPersonCameraPath(const CameraSystemIO* const IO);
    bool CameraSystemIOOverride(const CameraSystemIO* const IO)override;

    // 获取控制台变量系统
    C_ConVarSystem& GetCvarSystem() { return this->CvarSystem; }
};