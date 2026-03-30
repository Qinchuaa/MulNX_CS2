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

class ControlAdvancedView {
public:
    std::atomic<bool> Enable = false;
    
    std::atomic<int> boneIndex1 = 8;
    std::atomic<int> boneIndex2 = 9;
    std::atomic<int> boneIndex3 = 10;

    std::atomic<float> distance = 50.0f;

    // 平滑系数 (0.0 ~ 1.0)，值越小越平滑
    std::atomic<float> SMOOTH_FACTOR = 0.35f;

    // 静态平滑状态（仅在第一次成功时初始化，异常时重置）
    DirectX::XMFLOAT3 smoothCameraPos{};
    DirectX::XMFLOAT3 smoothCameraAngle{};
    bool initialized = false;  // 是否已初始化过平滑值

    // 新增：摄像机相对于武器坐标系的位置偏移（右、上、前）
    DirectX::XMFLOAT3 localPositionOffset{ -50.0f, 20.0f, -30.0f };  // 默认：左后方

    // 新增：滚转角（度），绕前向轴旋转
    float rollDegrees{ 0.0f };

    // 新增：注视点相对于武器坐标系的偏移（右、上、前）
    // 如果为零向量，则摄像机指向武器自身（枪口位置）
    DirectX::XMFLOAT3 localTargetOffset{ 0.0f, 0.0f, 0.0f };

    // 启用/禁用高级模式（现在旧模式已删除，此开关可省略，但保留以便UI控制）
    bool useAdvancedMode{ true };

    // 可选：是否使用第三个骨骼辅助定义局部坐标系的垂直轴
    // 若为 true，则使用骨骼1→骨骼3 作为临时上向参考；否则使用世界向上
    std::atomic<bool> useThirdBoneForUp{ false };
};

class CSController final :public MulNX::IAbstractLayer3D {
private:
    C_Modules Modules{};

    ControlSmoke controlSmoke{};
    ControlView controlView{};
    ControlAdvancedView controlAdvancedView{};

    // 自由摄像机控制
    std::atomic<bool> EnableFreeCameraControl = false;

    // 逆向层关键接口
    void* Source2EngineToClient001 = nullptr;
    VExecutor<void(int, const char*, int)> executor{};
    // 逆向层数据备份
    C_ConVarSystem CvarSystem{};
    C_GlobalVars* CSGlobalVars{};
    
    void ESP();
    void QuaternionToEuler(const DirectX::XMVECTOR& quat, float& pitch, float& yaw, float& roll);
public:
    std::atomic<bool> ESPDraw = false;
    
    std::unique_ptr<MulNX::Memory::HookEx> MyHook = nullptr;
    void HandleOverrideView(CS2::CViewSetup* viewSetup);
    int HandleSelfViewUpdate();
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