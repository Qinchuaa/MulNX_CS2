#include "CSController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXThirdParty/All_MinHook.hpp>
#include <MulNX/Systems/InputSystem/InputSystem.hpp>
#include <MulNX/Base/Math/Translate/Translate.hpp>

void CSController::HandleCameraSystemPlay(CS2::CViewSetup* viewSetup) {
    // 加载来自摄像机系统的View
    auto view = this->controlView.ViewToGame.load(std::memory_order_acquire);
    if (view) {
        viewSetup->pViewOrigin()->x = view->OriginX;
        viewSetup->pViewOrigin()->y = view->OriginY;
        viewSetup->pViewOrigin()->z = view->OriginZ;

        viewSetup->pViewAngles()->x = view->AnglesX;
        viewSetup->pViewAngles()->y = view->AnglesY;
        viewSetup->pViewAngles()->z = view->AnglesZ;

        if (view->FOV > 0.01f) {
            *viewSetup->pFov() = view->FOV;
        }
    }
}

void CSController::HandleOverrideView(CS2::CViewSetup* viewSetup) {
    // 同步窗口尺寸到ControlView
    this->controlView.currentView.WindowWidth.store(*viewSetup->pWidth(), std::memory_order_relaxed);
    this->controlView.currentView.WindowHeight.store(*viewSetup->pHeight(), std::memory_order_relaxed);

    // 执行roll覆盖
    viewSetup->pViewAngles()->z = this->controlView.InputRoll.load(std::memory_order_acquire);

    // 如果启用了高级视角控制，尝试更新视角数据，注意这里只是更新，不涉及具体的视角控制逻辑，视角控制逻辑在后面根据状态分流执行
    if (this->controlAdvancedView.Enable.load(std::memory_order_acquire)) {
        // 通过时间桥判断是否需要更新视角，防止抖动
        static auto lastTime = this->AL3D->Time()->GetReal();
        auto currentTime = this->AL3D->Time()->GetReal();
        if (currentTime > lastTime || lastTime - currentTime > 0.015f || this->controlAdvancedView.AlwaysCaulate.load(std::memory_order_acquire)) {
            auto result = this->HandleSelfViewUpdate();
            if (result.has_value()) {
                auto newView = result.value();
                this->viewBuffer.Push(newView);
            }
            else {
                this->ISys().LogWarning(std::format("HandleSelfViewUpdate failed with code: 0x{:X}", result.error()));
            }
            lastTime = currentTime;
        }
    }

    // 根据状态调用不同的视角控制逻辑
    // 自由摄像机优先级最高，其次是高级视角控制，最后是普通摄像机系统控制
    if (this->EnableFreeCameraControl.load(std::memory_order_acquire)) {
        // 如果启用自由摄像机控制，同步角度到InputSystem
        auto* inputSys = this->pInputSystem;
        auto& freeCam = inputSys->GetFreeCamera();
        freeCam.Rotation = *viewSetup->pViewAngles();
        freeCam.Update(inputSys);
        viewSetup->pViewOrigin()->x = freeCam.Position.x;
        viewSetup->pViewOrigin()->y = freeCam.Position.y;
        viewSetup->pViewOrigin()->z = freeCam.Position.z;
    }
    else if (this->controlAdvancedView.OverrideSelfView.load(std::memory_order_acquire)) {
        // 输出平滑后的值
        *viewSetup->pViewOrigin() = this->viewBuffer.Get().position;
        *viewSetup->pViewAngles() = this->viewBuffer.Get().rotation;
    }
    else {
        this->HandleCameraSystemPlay(viewSetup);
    }

    // 记录视角数据
    this->controlView.currentView.OriginX.store(viewSetup->pViewOrigin()->x, std::memory_order_release);
    this->controlView.currentView.OriginY.store(viewSetup->pViewOrigin()->y, std::memory_order_release);
    this->controlView.currentView.OriginZ.store(viewSetup->pViewOrigin()->z, std::memory_order_release);

    this->controlView.currentView.AnglesX.store(viewSetup->pViewAngles()->x, std::memory_order_release);
    this->controlView.currentView.AnglesY.store(viewSetup->pViewAngles()->y, std::memory_order_release);
    this->controlView.currentView.AnglesZ.store(viewSetup->pViewAngles()->z, std::memory_order_release);

    this->controlView.currentView.FOV.store(*viewSetup->pFov(), std::memory_order_release);
}

CS2::C_CSPlayerPawn* CSController::GetObserverTargetPawn() {
    try {
        auto* localController = this->Modules.client.dwLocalPlayerController();
        if (!localController) return nullptr;
        auto hLocalPawn = MulNX::MRead(localController->hPawn());
        auto* localPawn = this->Modules.client.GetBaseEntityFromHandle(hLocalPawn)->As<CS2::C_CSPlayerPawn>();
        if (!localPawn) return nullptr;

        auto hObserverTarget = localPawn->GetHandleObserverTarget();
        auto* target = this->Modules.client.GetBaseEntityFromHandle(hObserverTarget)->As<CS2::C_CSPlayerPawn>();
        return target;
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("GetObserverTargetPawn exception: {}", e.what()));
        return nullptr;
    }
}

std::expected<MulNX::Math::Point3, int> CSController::GetPoint3() {
    try {
        auto* target = this->GetObserverTargetPawn();
        if (!target) return std::unexpected(3);

        // 根据模式选择骨骼来源（武器或人体），二选一（不回退）
        bool useBody = this->controlAdvancedView.UseBodyBones.load(std::memory_order_acquire);
        MulNX::Math::Point3 point3{};

        if (useBody) {
            // 从人体读取骨骼
            point3.origin = target->GetBonePos(this->controlAdvancedView.boneIndex1.load());
            point3.forward = target->GetBonePos(this->controlAdvancedView.boneIndex2.load());
            point3.up = target->GetBonePos(this->controlAdvancedView.boneIndex3.load());
        }
        else {
            // 从武器读取骨骼
            auto hActiveWeapon = target->GetHandleActiveWeapon();
            auto* pWeapon = this->Modules.client.GetBaseEntityFromHandle(hActiveWeapon)->As<CS2::C_BasePlayerWeapon>();
            if (!pWeapon) return std::unexpected(4);
            point3.origin = pWeapon->GetBonePos(this->controlAdvancedView.boneIndex1.load());
            point3.forward = pWeapon->GetBonePos(this->controlAdvancedView.boneIndex2.load());
            point3.up = pWeapon->GetBonePos(this->controlAdvancedView.boneIndex3.load());
        }
        return std::expected<MulNX::Math::Point3, int>(point3);
    }
    catch (const std::exception& e) {
        this->ISys().LogWarning(std::format("GetObserverTargetBonePos exception: {}", e.what()));
        return std::unexpected(5);
    }
}

std::expected<MulNX::Math::View, int> CSController::HandleSelfViewUpdate() {
    try {
        auto point3Result = this->GetPoint3();
        if (!point3Result.has_value()) {
            return std::unexpected(point3Result.error());
        }
        auto point3 = point3Result.value();

        // ========== 构建局部坐标系 ==========
        DirectX::XMFLOAT3 forward, left, up;
        if (!MulNX::Math::BuildLocalCoordinateSystem(point3.origin, point3.forward, point3.up,
            forward, left, up,
            this->controlAdvancedView.InvertUp.load(std::memory_order_acquire))) {
            return std::unexpected(5);
        }

        // 存储调试信息
        auto boneInfo = std::make_shared<AxisInfo>();
        boneInfo->PosOrigin = point3.origin;
        boneInfo->PosForward = point3.forward;
        boneInfo->PosUp = point3.up;
        boneInfo->AxisForward = forward;
        boneInfo->AxisLeft = left;
        boneInfo->AxisUp = up;
        float axisLen = this->controlAdvancedView.AxisLength.load(std::memory_order_acquire);

        this->controlAdvancedView.CurrentBoneInfo.store(boneInfo, std::memory_order_release);

        // ========== 构建局部坐标系到世界坐标系的旋转矩阵 ==========
        // 注意：DirectX 使用行主序矩阵，矩阵的行（而非列）应表示局部轴在世界坐标系中的分量
        // 所以每一行分别是 X 轴(forward)、Y 轴(left)、Z 轴(up)
        DirectX::XMMATRIX rotLocalToWorld = DirectX::XMMatrixSet(
            forward.x, forward.y, forward.z, 0.0f,    // 行0: X轴(forward)
            left.x,    left.y,    left.z,    0.0f,    // 行1: Y轴(left)
            up.x,      up.y,      up.z,      0.0f,    // 行2: Z轴(up)
            0.0f,      0.0f,      0.0f,      1.0f
        );

        // ========== 计算摄像机世界位置 ==========
        // 将局部偏移变换到世界坐标系
        DirectX::XMVECTOR localOffset = DirectX::XMLoadFloat3(&this->controlAdvancedView.localPositionOffset);

        // 位置去耦：直接使用局部坐标系到世界坐标系的变换，
        // 使 localRotationOffset 不影响最终位置（旋转仅用于方向）
        DirectX::XMVECTOR worldOffset = DirectX::XMVector3TransformCoord(localOffset, rotLocalToWorld);

        DirectX::XMFLOAT3 worldOffsetVec;
        DirectX::XMStoreFloat3(&worldOffsetVec, worldOffset);

        // 计算最终摄像机位置
        DirectX::XMFLOAT3 targetCameraPos = point3.origin + worldOffsetVec;

        // ========== 计算摄像机世界旋转 ==========
        // 构建局部旋转偏移矩阵
        // 注意：旋转顺序应该是Yaw→Pitch→Roll
        float pitchRad = this->controlAdvancedView.localRotationOffset.x * (DirectX::XM_PI / 180.0f);
        float yawRad = this->controlAdvancedView.localRotationOffset.y * (DirectX::XM_PI / 180.0f);
        float rollRad = this->controlAdvancedView.localRotationOffset.z * (DirectX::XM_PI / 180.0f);

        // 创建旋转四元数
        DirectX::XMVECTOR quatYaw = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), yawRad);
        DirectX::XMVECTOR quatPitch = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), pitchRad);
        DirectX::XMVECTOR quatRoll = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rollRad);

        // 组合顺序
        DirectX::XMVECTOR quatTemp = DirectX::XMQuaternionMultiply(quatPitch, quatYaw);
        DirectX::XMVECTOR quatLocalRot = DirectX::XMQuaternionMultiply(quatRoll, quatTemp);

        // 将四元数转换为矩阵
        DirectX::XMMATRIX rotLocalOffset = DirectX::XMMatrixRotationQuaternion(quatLocalRot);

        // 计算最终旋转矩阵
        // 先应用局部旋转偏移，再变换到世界坐标系
        DirectX::XMMATRIX rotFinal = DirectX::XMMatrixMultiply(rotLocalOffset, rotLocalToWorld);

        // 从最终旋转矩阵中提取四元数
        DirectX::XMVECTOR quatFinal = DirectX::XMQuaternionRotationMatrix(rotFinal);
        DirectX::XMFLOAT4 quatF;
        DirectX::XMStoreFloat4(&quatF, quatFinal);

        // 将四元数转换为欧拉角
        DirectX::XMFLOAT3 targetCameraAngle;
        MulNX::Math::CSQuatToEuler(quatF, targetCameraAngle);

        // 规范化角度
        while (targetCameraAngle.y > 180.0f) targetCameraAngle.y -= 360.0f;
        while (targetCameraAngle.y < -180.0f) targetCameraAngle.y += 360.0f;

        return std::expected<MulNX::Math::View, int>(MulNX::Math::View(targetCameraPos, targetCameraAngle, 90.0f));
    }
    catch (...) {
        return std::unexpected(0xFFFF);
    }
}

bool CSController::UINodeFunc(MulNXUINode* node) {
    if (this->ESPDraw.load(std::memory_order_acquire)) {
        this->ESP();
    }
    auto w = MulNX::UI::RAIIWindow("快捷操作", this->ShowWindow);
    if (!w)return true;
    MulNX::TransInfo info;
    info.pMatrix = this->GetViewMatrix();
    info.windowHeight = this->GetWinHeight();
    info.windowWidth = this->GetWinWidth();

    static float gameTimeScale = 1.0f;
    static float virtualTimeScale = 1.0f;
    ImGui::SliderFloat("游戏时间流速", &gameTimeScale, 0.0f, 5.0f);
    ImGui::SliderFloat("虚拟时间流速", &virtualTimeScale, 0.0f, 5.0f);

    if (ImGui::Button("启用时间虚拟化")) {
        this->AL3D->ExecuteCommand(std::format("host_timescale {}", gameTimeScale));
        this->AL3D->Time()->RefreshVirtual(true, virtualTimeScale);
    }
    ImGui::SameLine();
    if (ImGui::Button("解除时间虚拟化")) {
        this->AL3D->ExecuteCommand("host_timescale 1");
        this->AL3D->Time()->RefreshVirtual(false, 1.0f);
    }

    MulNX::UI::SliderFloat("roll调整", this->controlView.InputRoll, -179.99f, 179.99f);

    auto* pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();
    ImGui::SliderFloat("fov调整", pGlobalFOV, 0, 179.99f);
    if (ImGui::Button("一键归正")) {
        this->controlView.InputRoll.store(0, std::memory_order_release);
        *pGlobalFOV = 0;
    }
    if (ImGui::CollapsingHeader("烟雾弹控制")) {
        MulNX::UI::Checkbox("启用烟雾弹控制", this->controlSmoke.Enbale);
        MulNX::UI::Checkbox("烟雾显示", this->controlSmoke.Show);
        MulNX::UI::SliderFloat("色彩R", this->controlSmoke.R, 0, 255);
        MulNX::UI::SliderFloat("色彩G", this->controlSmoke.G, 0, 255);
        MulNX::UI::SliderFloat("色彩B", this->controlSmoke.B, 0, 255);
    }
    if (ImGui::CollapsingHeader("高级视角控制")) {
        MulNX::UI::Checkbox("启用高级视角控制", this->controlAdvancedView.Enable);
        MulNX::UI::Checkbox("覆盖自视角", this->controlAdvancedView.OverrideSelfView);
        MulNX::UI::Checkbox("始终计算视角", this->controlAdvancedView.AlwaysCaulate);

        // 骨骼选择（三点定义坐标系）
        MulNX::UI::SliderInt("骨骼起点 (原点)", this->controlAdvancedView.boneIndex1, 0, 127);
        MulNX::UI::SliderInt("骨骼终点 (前向)", this->controlAdvancedView.boneIndex2, 0, 127);
        MulNX::UI::SliderInt("骨骼辅助 (上向参考)", this->controlAdvancedView.boneIndex3, 0, 127);

        // 位置偏移（前, 左, 上） — 与输入系统约定对应：localPositionOffset (x=前, y=左, z=上)
        ImGui::Text("摄像机局部偏移 (前,左,上)");
        ImGui::SliderFloat("前移 (X)", &this->controlAdvancedView.localPositionOffset.x, -200.0f, 200.0f);
        ImGui::SliderFloat("左移 (Y)", &this->controlAdvancedView.localPositionOffset.y, -200.0f, 200.0f);
        ImGui::SliderFloat("上移 (Z)", &this->controlAdvancedView.localPositionOffset.z, -200.0f, 200.0f);

        // 旋转偏移（统一为：俯仰, 偏航, 滚转）
        ImGui::Text("摄像机局部旋转 (俯仰,偏航,滚转)");
        ImGui::SliderFloat("俯仰 (Pitch)", &this->controlAdvancedView.localRotationOffset.x, -89.0f, 89.0f);
        ImGui::SliderFloat("偏航 (Yaw)", &this->controlAdvancedView.localRotationOffset.y, -180.0f, 180.0f);
        ImGui::SliderFloat("滚转 (Roll)", &this->controlAdvancedView.localRotationOffset.z, -180.0f, 180.0f);

        // 平滑系数
        MulNX::UI::SliderFloat("平滑系数", this->viewBuffer.SMOOTH_FACTOR, 0.0f, 1.0f);

        // 上向反转选项
        MulNX::UI::Checkbox("反转上向", this->controlAdvancedView.InvertUp);
        // 绘制选项：原始骨骼点与坐标轴
        MulNX::UI::Checkbox("显示原始骨骼点", this->controlAdvancedView.ShowOriginalBones);
        MulNX::UI::Checkbox("显示坐标轴", this->controlAdvancedView.ShowCoordinateAxes);
        MulNX::UI::SliderFloat("坐标轴长度", this->controlAdvancedView.AxisLength, 1.0f, 200.0f);
        // 骨骼来源模式：武器 / 人体
        MulNX::UI::Checkbox("人体骨骼模式", this->controlAdvancedView.UseBodyBones);

        // 绘制当前骨骼位置（调试用）
        auto boneInfo = this->controlAdvancedView.CurrentBoneInfo.load(std::memory_order_acquire);
        if (boneInfo) {
            // 分两部分绘制：原始骨骼点 与 坐标轴
            if (this->controlAdvancedView.ShowOriginalBones.load(std::memory_order_acquire)) {
                MulNX::UI::DrawWorldPoint(boneInfo->PosOrigin, info, "Origin");
                MulNX::UI::DrawWorldPoint(boneInfo->PosForward, info, "Forward");
                MulNX::UI::DrawWorldPoint(boneInfo->PosUp, info, "Up");
            }

            if (this->controlAdvancedView.ShowCoordinateAxes.load(std::memory_order_acquire)) {
                float axisLen = this->controlAdvancedView.AxisLength.load(std::memory_order_acquire);
                DirectX::XMFLOAT3 org = boneInfo->PosOrigin;
                DirectX::XMFLOAT3 f_end = (boneInfo->AxisForward * axisLen) + org;
                DirectX::XMFLOAT3 u_end = (boneInfo->AxisUp * axisLen) + org;
                DirectX::XMFLOAT3 r_end = (boneInfo->AxisLeft * axisLen) + org;

                MulNX::UI::DrawWorldLine(org, f_end, info, ImColor(255, 0, 0), 2.0f);
                MulNX::UI::DrawWorldLine(org, u_end, info, ImColor(0, 255, 0), 2.0f);
                MulNX::UI::DrawWorldLine(org, r_end, info, ImColor(0, 0, 255), 2.0f);

                // 在轴端点绘制点与标签
                MulNX::UI::DrawWorldPoint(f_end, info, "F");
                MulNX::UI::DrawWorldPoint(u_end, info, "U");
                MulNX::UI::DrawWorldPoint(r_end, info, "L");
            }
        }
    }

    // 自由摄像机控制
    if (ImGui::CollapsingHeader("自由摄像机控制")) {
        bool currentEnable = this->EnableFreeCameraControl.load(std::memory_order_acquire);
        if (ImGui::Checkbox("启用自由摄像机位置控制", &currentEnable)) {
            if (currentEnable && !this->EnableFreeCameraControl.load(std::memory_order_acquire)) {
                // 从未启用到启用：读取当前游戏位置和角度
                DirectX::XMFLOAT3 gamePos{
                    this->controlView.currentView.OriginX.load(std::memory_order_acquire),
                    this->controlView.currentView.OriginY.load(std::memory_order_acquire),
                    this->controlView.currentView.OriginZ.load(std::memory_order_acquire)
                };
                this->pInputSystem->GetFreeCamera().Position = gamePos;
            }
            this->EnableFreeCameraControl.store(currentEnable, std::memory_order_release);
        }

        if (currentEnable) {
            auto& freeCam = this->pInputSystem->GetFreeCamera();

            ImGui::Text("当前位置: X=%.2f, Y=%.2f, Z=%.2f",
                freeCam.Position.x, freeCam.Position.y, freeCam.Position.z);

            float speed = freeCam.MoveSpeed;
            if (ImGui::SliderFloat("移动速度", &speed, 10.0f, 1000.0f)) {
                freeCam.MoveSpeed = speed;
            }
        }
    }

    return true;
}

void CSController::VirtualMain() {
    this->EntryProcessMsg();
    return;
}
void CSController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "Core/ReHook"_hash: {
        this->ISys().LogSucc("已完成Hook重载！");
        break;
    }

    }
}

bool CSController::Init() {
    this->ShowWindow = true;
    this->ISys()
        .SubscribeAsync("Core/ReHook");
    this->NeedThread(3);
    this->SendUINode(this->GetName(), [this](MulNXUINode* node) {return this->UINodeFunc(node);});

    this->Modules.client = CS2::Module::Client(L"client.dll");
    this->Modules.engine2 = MulNX::Memory::DllModule(L"engine2.dll");
    this->Modules.tier0 = MulNX::Memory::DllModule(L"tier0.dll");

    this->Source2EngineToClient001 =
        this->Modules.engine2.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
        ("Source2EngineToClient001", nullptr);
    this->executor = IVClass::Assume(this->Source2EngineToClient001)->GetVFunc<void(int, const char*, int)>(49);

    this->CvarSystem.Address =
        (uintptr_t)this->Modules.tier0.GetProcAddressT<void* (const char*, int*)>("CreateInterface")
        ("VEngineCvar007", nullptr);

    if (this->Modules.client.Valid) {
        // 搜索 .text 段
        auto textRegion = this->Modules.client.GetTextRegion();
        if (textRegion.IsValid()) {
            // 搜索特征码
            const auto& pattern = MulNX::CS2::Signatures::CallIsPlayingDemo;
            auto target = textRegion.FindRegion(pattern);

            if (target.IsValid()) {
                this->MyHook = MulNX::Memory::HookEx::Create(target.Data(), 16, false, [this](RegContext* ctx)->void {
                    return this->HandleOverrideView((CS2::CViewSetup*)ctx->rsi);
                    });
                this->MyHook->Attach();
            }
        }
    }

    this->controlView.dofs.pNearBlurry = this->CvarSystem.GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
    this->controlView.dofs.pNearCrisp = this->CvarSystem.GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarCrisp = this->CvarSystem.GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
    this->controlView.dofs.pFarBlurry = this->CvarSystem.GetCvar("r_dof_override_far_blurry")->GetPtr<float>();

    return true;
}

int CSController::BasicUpdate() {
    // 获取CS2全局变量
    this->CSGlobalVars = MulNX::MRead<C_GlobalVars*>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGlobalVars);

    static int OldRoundStartCount = MulNX::MRead(this->Modules.client.dwGameRules()->nRoundStartCount());
    if (OldRoundStartCount != MulNX::MRead(this->Modules.client.dwGameRules()->nRoundStartCount())) {
        MulNX::Message Msg("Game/NewRound"_hash);
        this->ISys().PublishAsync(std::move(Msg));
        OldRoundStartCount = MulNX::MRead(this->Modules.client.dwGameRules()->nRoundStartCount());
    }
    for (int i = 0;i <= this->Modules.client.dwGameEntitySystem_highestEntityIndex();i++) {
        auto entity = this->Modules.client.GetBaseEntity(i);
        if (!entity)continue;

        auto pClassInfo = MulNX::MRead(entity->pClassInfo());
        if (!pClassInfo)continue;

        auto pName = MulNX::MRead(((pClassInfo))->pName());
        if (!pName)continue;

        auto zname = std::string(pName);

        if (zname.find("smokegrenade") != std::string::npos && zname.find("weapon") == std::string::npos) {
            if (this->controlSmoke.Enbale.load(std::memory_order_acquire)) {
                if (this->controlSmoke.Show.load(std::memory_order_acquire) == true) {
                    auto* color = entity->As<CS2::C_SmokeGrenadeProjectile>()->vSmokeColor();
                    DirectX::XMFLOAT3 pushIn{
                        this->controlSmoke.R.load(std::memory_order_acquire) ,
                        this->controlSmoke.G.load(std::memory_order_acquire) ,
                        this->controlSmoke.B.load(std::memory_order_acquire) };
                    MulNX::MWrite(color, pushIn);
                }
                else {
                    MulNX::MWrite(entity->As<CS2::C_SmokeGrenadeProjectile>()->bDidSmokeEffect(), false);
                    MulNX::MWrite(entity->As<CS2::C_SmokeGrenadeProjectile>()->bSmokeEffectSpawned(), false);
                    MulNX::MWrite(entity->As<CS2::C_SmokeGrenadeProjectile>()->nSmokeEffectTickBegin(), 0);
                }
            }
        }
    }
    return 0;
}
int CSController::EntityListUpdate() {
    std::unique_lock lock(this->GetMutex());
    // 玩家控制器，地图上从1到10，位于索引1到10处
    for (int i = 1; i <= 10; ++i) {
        auto* controller = this->Modules.client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
        if (!controller)continue;
        auto hPawn = MulNX::MRead(controller->hPawn());
        auto* pawn = this->Modules.client.GetBaseEntityFromHandle(hPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
        if (!pawn)continue;

        auto team = MulNX::MRead(pawn->iTeamNum());
        if (team != 2 && team != 3)continue;

        auto& AL3DEntity = this->AL3DGameData.Players[i];
        AL3DEntity.Position = MulNX::MRead(pawn->vOldOrigin());
        AL3DEntity.EyePosition = MulNX::MRead(pawn->vOldOrigin()) + MulNX::MRead(pawn->vecViewOffset());
        AL3DEntity.Rotation = MulNX::MRead(pawn->angEyeAngles());
        AL3DEntity.HP = MulNX::MRead(pawn->iHealth());
        AL3DEntity.Team = team;
        AL3DEntity.Alive = AL3DEntity.HP;
        AL3DEntity.IndexInMap = i;
    }

    return 0;
}

void CSController::ThreadMain() {
    while (this->MyThreadRunning) {
        try {
            this->GetMsgResult = this->TryGetMsg();
        }
        catch (const std::runtime_error& e) {
            this->ISys().LogWarning("在更新数据时捕获到异常：" + std::string(e.what()));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(this->MyThreadDelta));
    }
    return;
}
int CSController::TryGetMsg() {
    int Result = 0;
    Result = this->BasicUpdate();
    if (Result) {
        this->GlobalVars->InGamePlaying = false;
        return Result;
    }
    this->EntityListUpdate();

    this->GlobalVars->InGamePlaying = true;

    return 0;
}


void CSController::HandleFreeCameraPath(const CameraSystemIO* const IO) {
    const auto& pos = IO->Frame.view.position;
    const auto& fov = IO->Frame.view.FOV;
    const auto& rot = IO->Frame.view.rotation;
    const auto& dof = IO->Frame.view.dof;
#ifdef _DEBUG
    // static MulNX::Math::Frame thisFrame;
    // if (thisFrame != IO->Frame) {
    //     thisFrame = IO->Frame;
    //     this->ISys().LogInfo(thisFrame.GetMsg());
    // }
#endif // _DEBUG
    auto view = std::make_shared<Views>();
    view->OriginX = pos.x;
    view->OriginY = pos.y;
    view->OriginZ = pos.z;
    view->FOV = fov;
    view->AnglesX = rot.x;
    view->AnglesY = rot.y;
    view->AnglesZ = rot.z;
    this->controlView.InputRoll.store(view->AnglesZ, std::memory_order_release);
    this->controlView.ViewToGame.store(view);

    *this->controlView.dofs.pNearBlurry = dof.NearBlurry;
    *this->controlView.dofs.pNearCrisp = dof.NearCrisp;
    *this->controlView.dofs.pFarCrisp = dof.FarCrisp;
    *this->controlView.dofs.pFarBlurry = dof.FarBlurry;
}
void CSController::HandleFirstPersonCameraPath(const CameraSystemIO* const IO) {
    static uint8_t LastIndex = 0xFFFF;
    if (LastIndex != IO->Frame.TargetPlayerIndexInMap) {
        this->ExecuteCommand("spec_mode 2;spec_player " + std::to_string(IO->Frame.TargetPlayerIndexInMap));
        LastIndex = IO->Frame.TargetPlayerIndexInMap;
    }
}
bool CSController::CameraSystemIOOverride(const CameraSystemIO* const IO) {
    static float LastCallTime = IO->FrameGameTime;
    if (LastCallTime == IO->FrameGameTime) {
        return true;
    }
    LastCallTime = IO->FrameGameTime;

    switch (IO->Frame.TargetOBMode) {
    case 4:this->HandleFreeCameraPath(IO); break;
    case 2:this->HandleFirstPersonCameraPath(IO); break;
    }

    return true;
}

// C_ConVar* m_yawPtr = nullptr;
// m_yawPtr = this->CvarSystem.GetCvar("m_yaw");
// std::vector<int> schemas;
// for (int schema = 0;schema < 0x100;++schema) {
//     const float targetValue = 0.0165f;
//     bool valueIsRight = false;
//     auto checkValue = [&]()->bool {
//         float currentValue = *(float*)((uintptr_t)m_yawPtr + schema);
//         if (fabs(currentValue - targetValue) < 0.001f) {
//             valueIsRight = true;
//         }
//         return true;// 无崩溃
//         };
//     MulNX::Base::UnsafeFunc(checkValue);
//     if (valueIsRight) {
//         schemas.push_back(schema);
//     }
// }