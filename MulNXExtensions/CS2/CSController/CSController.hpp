#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>

#include <MulNXExtensions/CS2/CSClasses/tree/tree.hpp>
#include <MulNXExtensions/CS2/CSClasses/GlobalVars/GlobalVars.hpp>
#include <MulNXExtensions/CS2/CSClasses/CSDll/CSDll.hpp>
#include <MulNXExtensions/CS2/CSClasses/C_CSGameRules/C_CSGameRules.hpp>

#include "ConVarSystem/ConVarSystem.hpp"
#include "FreeCameraController/FreeCameraController.hpp"
#include "AdvancedViewController/AdvancedViewController.hpp"

#include <expected>

class C_Modules {
public:
    CS2::Module::Client client{};
    CS2::Module::engine2 engine2{};
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
    ControlView controlView{};

    FreeCameraController* pFreeCameraController{};
    AdvancedViewController* pAdvancedViewController = nullptr;

    // 逆向层关键接口
    void* Source2EngineToClient001 = nullptr;
    VExecutor<void(int, const char*, int)> executor{};
    // 逆向层数据备份
    C_ConVarSystem CvarSystem{};
    C_GlobalVars* CSGlobalVars{};

    std::atomic<bool> autoTick = true;
    std::atomic<int> deltaTick = 0;

    void ESP();
public:
    std::vector<std::function<bool(CS2::CCSPlayerController*, CS2::C_CSPlayerPawn*)>>handlesControlPlayer{};
    C_Modules Modules{};
    std::atomic<bool> ESPDraw = false;
    
    std::unique_ptr<MulNX::Hook> hkPosCallIsPlayingDemo = nullptr;
    
    void HandleOverrideView(CS2::CViewSetup* viewSetup);
    void HandleCameraSystemPlay(CS2::CViewSetup* viewSetup);

    bool Init()override;
    bool UINodeFunc(MulNXUINode* node);
    void ProcessMsg(MulNX::Message& Msg)override;
    void Update();

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