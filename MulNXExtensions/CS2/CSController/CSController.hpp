#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

#include "ConVarSystem/ConVarSystem.hpp"
#include "GlobalVars/GlobalVars.hpp"
#include "List/C_BaseEntity.hpp"

#include "C_CSGameRules/C_CSGameRules.hpp"
#include "Client/Client.hpp"

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

class AxisInfo {
public:
    DirectX::XMFLOAT3 PosOrigin;
    DirectX::XMFLOAT3 PosForward;
    DirectX::XMFLOAT3 PosUp;

    // 轴向单位向量（局部坐标系的三轴）
    DirectX::XMFLOAT3 AxisForward;
    DirectX::XMFLOAT3 AxisLeft;
    DirectX::XMFLOAT3 AxisUp;
};

struct ControlAdvancedView {
    std::atomic<bool> Enable = false;
    std::atomic<bool> OverrideSelfView = false;
    std::atomic<bool> AlwaysCaulate = false;

    std::atomic<bool> useLocalPawn = false;
    // 强制头模式
    std::atomic<bool> forceHeadMode = false;
    // 是否使用人体骨骼而非武器骨骼
    std::atomic<bool> UseBodyBones = false;

    // 骨骼索引
    std::atomic<int> boneIndex1{ 0 };
    std::atomic<int> boneIndex2{ 1 };
    std::atomic<int> boneIndex3{ 2 };

    // 位置和旋转偏移
    DirectX::XMFLOAT3 localPositionOffset = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 localRotationOffset = { 0.0f, 0.0f, 0.0f };

    // 调试信息
    std::atomic<std::shared_ptr<AxisInfo>> CurrentBoneInfo;
    // 是否反转上向（用于修正骨骼上向方向）
    std::atomic<bool> InvertUp{ false };
    // 绘制选项
    std::atomic<bool> ShowOriginalBones{ true };
    std::atomic<bool> ShowCoordinateAxes{ true };
    // 绘制坐标轴的长度（世界单位）
    std::atomic<float> AxisLength{ 30.0f };
};

class CSController final :public MulNX::IAbstractLayer3D {
private:
    C_Modules Modules{};

    ControlSmoke controlSmoke{};
    ControlView controlView{};

    ControlAdvancedView controlAdvancedView{};
    MulNX::Math::ViewBuffer viewBuffer{};

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
    std::atomic<bool> ESPDraw = false;
    
    std::unique_ptr<MulNX::Memory::HookEx> MyHook = nullptr;
    void HandleOverrideView(CS2::CViewSetup* viewSetup);

    CS2::C_CSPlayerPawn* GetSelfViewTargetPawn();
    std::expected<MulNX::Math::Point3, int> GetPoint3(CS2::CViewSetup* viewSetup);
    void HandleCameraSystemPlay(CS2::CViewSetup* viewSetup);
    std::expected<MulNX::Math::View, int> HandleSelfViewUpdate(CS2::CViewSetup* viewSetup);

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