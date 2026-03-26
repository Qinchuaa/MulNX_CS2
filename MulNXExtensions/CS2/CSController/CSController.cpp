#include "CSController.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXThirdParty/All_MinHook.hpp>

void CSController::HandleOverrideView(void* ThisCViewSetup) {
    // 定位关键数据
    int* pWidth = (int*)((char*)ThisCViewSetup + 0x434);
    int* pHeight = (int*)((char*)ThisCViewSetup + 0x43C);
    this->controlView.currentView.WindowWidth.store(*pWidth, std::memory_order_relaxed);
    this->controlView.currentView.WindowHeight.store(*pHeight, std::memory_order_relaxed);

    float* pFov = (float*)((char*)ThisCViewSetup + 0x498);
    float* pViewOrigin = (float*)((char*)ThisCViewSetup + 0x4a0);
    float* pViewAngles = (float*)((char*)ThisCViewSetup + 0x4b8);

    // 加载来自摄像机系统的View
    auto view = this->controlView.ViewToGame.load(std::memory_order_acquire);
    if (view != nullptr) {
        pViewOrigin[0] = view->OriginX;
        pViewOrigin[1] = view->OriginY;
        pViewOrigin[2] = view->OriginZ;

        pViewAngles[0] = view->AnglesX;
        pViewAngles[1] = view->AnglesY;
        pViewAngles[2] = view->AnglesZ;

        if (view->FOV > 0.01f) {
            *pFov = view->FOV;
        }
    }
    // 执行roll覆盖
    pViewAngles[2] = this->controlView.InputRoll.load(std::memory_order_acquire);
    // 记录视角数据
    this->controlView.currentView.AnglesX.store(pViewAngles[0], std::memory_order_release);
    this->controlView.currentView.AnglesY.store(pViewAngles[1], std::memory_order_release);
    this->controlView.currentView.AnglesZ.store(pViewAngles[2], std::memory_order_release);
    this->controlView.currentView.OriginX.store(pViewOrigin[0], std::memory_order_release);
    this->controlView.currentView.OriginY.store(pViewOrigin[1], std::memory_order_release);
    this->controlView.currentView.OriginZ.store(pViewOrigin[2], std::memory_order_release);
    if (*pFov < 0.01f) {
        this->controlView.currentView.FOV.store(90.0f, std::memory_order_release);
    }
    else {
        this->controlView.currentView.FOV.store(*pFov, std::memory_order_release);
    }

    // // 平滑系数 (0.0 ~ 1.0)，值越小越平滑
    // constexpr float SMOOTH_FACTOR = 0.2f;

    // // 使用静态变量保存上一帧的平滑值
    // static DirectX::XMFLOAT3 s_smoothCameraPos{};
    // static DirectX::XMFLOAT3 s_smoothCameraAngle{};
    // static bool s_firstFrame = true;

    // try {
    //     auto* localController = this->Modules.client.dwLocalPlayerController();
    //     if (!localController) return;
    //     auto hLocalPawn = MulNX::MRead(localController->hPawn());
    //     if (!hLocalPawn.Valid()) return;
    //     auto* localPawn = this->Modules.client.GetBaseEntityFromHandle(hLocalPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
    //     if (!localPawn) return;
    //     auto* pOB = MulNX::MRead(localPawn->pObserverServices());
    //     if (!pOB) return;
    //     auto handle = MulNX::MRead(pOB->hObserverTarget());
    //     if (!handle.Valid()) return;
    //     auto* target = this->Modules.client.GetBaseEntityFromHandle(handle.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
    //     if (!target) return;

    //     auto EyePos = MulNX::MRead(target->vOldOrigin()) + MulNX::MRead(target->vecViewOffset());
    //     auto ViewAngle = MulNX::MRead(target->angEyeAngles());

    //     // 计算目标摄像机位置（目标前方 50 单位）
    //     auto dir = MulNX::Math::CSEulerToDir(ViewAngle.x, ViewAngle.y);
    //     dir *= 50;
    //     DirectX::XMFLOAT3 targetCameraPos = dir + EyePos;

    //     // 计算目标摄像机角度（从摄像机指向目标）
    //     DirectX::XMFLOAT3 targetCameraAngle;
    //     MulNX::Math::CSDirToEuler(EyePos - targetCameraPos, targetCameraAngle);
    //     // 滚转角强制为 0（保持水平）
    //     targetCameraAngle.z = 0;

    //     // 第一帧直接赋值，避免突变
    //     if (s_firstFrame) {
    //         s_smoothCameraPos = targetCameraPos;
    //         s_smoothCameraAngle = targetCameraAngle;
    //         s_firstFrame = false;
    //     }

    //     // 指数平滑位置
    //     s_smoothCameraPos.x += (targetCameraPos.x - s_smoothCameraPos.x) * SMOOTH_FACTOR;
    //     s_smoothCameraPos.y += (targetCameraPos.y - s_smoothCameraPos.y) * SMOOTH_FACTOR;
    //     s_smoothCameraPos.z += (targetCameraPos.z - s_smoothCameraPos.z) * SMOOTH_FACTOR;

    //     // 指数平滑角度（注意处理角度环绕）
    //     // 先计算差值，并规范化到 [-180, 180]
    //     auto angleDiff = [](float target, float current) -> float {
    //         float diff = target - current;
    //         // 将差值限制在 [-180, 180] 范围内（处理环绕）
    //         if (diff > 180.0f) diff -= 360.0f;
    //         if (diff < -180.0f) diff += 360.0f;
    //         return diff;
    //         };
    //     s_smoothCameraAngle.x += angleDiff(targetCameraAngle.x, s_smoothCameraAngle.x) * SMOOTH_FACTOR;
    //     s_smoothCameraAngle.y += angleDiff(targetCameraAngle.y, s_smoothCameraAngle.y) * SMOOTH_FACTOR;
    //     // 滚转角始终为 0，不做平滑（或直接赋值）
    //     s_smoothCameraAngle.z = 0;

    //     // 输出平滑后的值
    //     pViewOrigin[0] = s_smoothCameraPos.x;
    //     pViewOrigin[1] = s_smoothCameraPos.y;
    //     pViewOrigin[2] = s_smoothCameraPos.z;

    //     pViewAngles[0] = s_smoothCameraAngle.x;
    //     pViewAngles[1] = s_smoothCameraAngle.y;
    //     pViewAngles[2] = 0;
    // }
    // catch (...) {
    //     // 捕获异常后可重置第一帧标志，避免残留错误状态
    //     s_firstFrame = true;
    // }


    // 平滑系数 (0.0 ~ 1.0)，值越小越平滑
    constexpr float SMOOTH_FACTOR = 0.2f;
    // 摄像机与目标的偏移距离（单位）
    constexpr float CAMERA_OFFSET = 50.0f;

    // 使用静态变量保存上一帧的平滑值
    static DirectX::XMFLOAT3 s_smoothCameraPos{};
    static DirectX::XMFLOAT3 s_smoothCameraAngle{};
    static bool s_firstFrame = true;

    try {
        auto* localController = this->Modules.client.dwLocalPlayerController();
        if (!localController) return;
        auto hLocalPawn = MulNX::MRead(localController->hPawn());
        if (!hLocalPawn.Valid()) return;
        auto* localPawn = this->Modules.client.GetBaseEntityFromHandle(hLocalPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
        if (!localPawn) return;
        auto* pOB = MulNX::MRead(localPawn->pObserverServices());
        if (!pOB) return;
        auto handle = MulNX::MRead(pOB->hObserverTarget());
        if (!handle.Valid()) return;
        auto* target = this->Modules.client.GetBaseEntityFromHandle(handle.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
        if (!target) return;

        // ---------- 骨骼数据获取 ----------
        auto* pGameSceneNode = MulNX::MRead(target->pGameSceneNode());
        if (!pGameSceneNode) return;
        auto* bones = MulNX::MRead(static_cast<CS2::CSkeletonInstance*>(pGameSceneNode)->unkBoneArray());
        if (!bones) return;

        // 索引 15 = 头骨，索引 16 = 脖子（或头部参考点）
        DirectX::XMFLOAT3 headPos = MulNX::MRead(bones->at(15));     // 目标位置（头骨）
        DirectX::XMFLOAT3 neckPos = MulNX::MRead(bones->at(16));     // 辅助方向点

        // 计算前向向量（从脖子指向头）
        DirectX::XMFLOAT3 forwardDir = headPos - neckPos;
        float len = sqrtf(forwardDir.x * forwardDir.x + forwardDir.y * forwardDir.y + forwardDir.z * forwardDir.z);
        if (len > 0.0001f) {
            forwardDir.x /= len;
            forwardDir.y /= len;
            forwardDir.z /= len;
        }

        // 目标摄像机位置 = 头骨位置 + 前向向量 * 偏移距离
        DirectX::XMFLOAT3 targetCameraPos = headPos + forwardDir * CAMERA_OFFSET;

        // 目标摄像机角度：从摄像机指向头骨位置
        DirectX::XMFLOAT3 lookDir = headPos - targetCameraPos;
        DirectX::XMFLOAT3 targetCameraAngle;
        MulNX::Math::CSDirToEuler(lookDir, targetCameraAngle);
        // 滚转角强制为 0
        targetCameraAngle.z = 0;

        // ---------- 平滑处理 ----------
        if (s_firstFrame) {
            s_smoothCameraPos = targetCameraPos;
            s_smoothCameraAngle = targetCameraAngle;
            s_firstFrame = false;
        }

        // 指数平滑位置
        s_smoothCameraPos.x += (targetCameraPos.x - s_smoothCameraPos.x) * SMOOTH_FACTOR;
        s_smoothCameraPos.y += (targetCameraPos.y - s_smoothCameraPos.y) * SMOOTH_FACTOR;
        s_smoothCameraPos.z += (targetCameraPos.z - s_smoothCameraPos.z) * SMOOTH_FACTOR;

        // 指数平滑角度（处理环绕）
        auto angleDiff = [](float target, float current) -> float {
            float diff = target - current;
            if (diff > 180.0f) diff -= 360.0f;
            if (diff < -180.0f) diff += 360.0f;
            return diff;
            };
        s_smoothCameraAngle.x += angleDiff(targetCameraAngle.x, s_smoothCameraAngle.x) * SMOOTH_FACTOR;
        s_smoothCameraAngle.y += angleDiff(targetCameraAngle.y, s_smoothCameraAngle.y) * SMOOTH_FACTOR;
        s_smoothCameraAngle.z = 0;

        // 输出平滑后的值
        pViewOrigin[0] = s_smoothCameraPos.x;
        pViewOrigin[1] = s_smoothCameraPos.y;
        pViewOrigin[2] = s_smoothCameraPos.z;

        pViewAngles[0] = s_smoothCameraAngle.x;
        pViewAngles[1] = s_smoothCameraAngle.y;
        pViewAngles[2] = 0;
    }
    catch (...) {
        s_firstFrame = true;
    }


    // try {
    //     for (int i = 0;i <= this->Modules.client.dwGameEntitySystem_highestEntityIndex();i++) {
    //         auto entity = this->Modules.client.GetBaseEntity(i);
    //         if (entity == 0) {
    //             continue;
    //         }
    //         auto hPawn = MulNX::MRead(entity->As<CS2::CBasePlayerController>()->hPawn());
    //         if (!hPawn.Valid()) {
    //             continue;
    //         }
    //         auto* pawn = this->Modules.client.GetBaseEntity(hPawn.GetIndexInEntityList());
    //         if (!pawn) {
    //             continue;
    //         }
    //         auto* pGameSceneNode = MulNX::MRead(pawn->pGameSceneNode());
    //         if (!pGameSceneNode)continue;
    //         auto* bones = MulNX::MRead(static_cast<CS2::CSkeletonInstance*>(pGameSceneNode)->unkBoneArray());
    //         if (!bones)continue;

    //         MulNX::TransInfo info;
    //         info.pMatrix = this->GetViewMatrix();
    //         info.windowHeight = 1080;
    //         info.windowWidth = 1920;

    //         DirectX::XMFLOAT3 pos15 = MulNX::MRead(bones->at(15));
    //         DirectX::XMFLOAT3 pos16 = MulNX::MRead(bones->at(16));

    //         DirectX::XMFLOAT3 euler;
    //         MulNX::Math::CSDirToEuler(pos16 - pos15, euler);

    //         pViewOrigin[0] = pos15.x;
    //         pViewOrigin[1] = pos15.y;
    //         pViewOrigin[2] = pos15.z;

    //         pViewAngles[0] = euler.x;
    //         pViewAngles[1] = euler.y;
    //         break;
    //     }
    // }
    // catch (const std::runtime_error& e) {
    //     this->ISys().LogWarning("捕获到在应用自拍杆时发生的异常");
    // }

    return;
}

bool CSController::UINodeFunc(MulNXUINode* node) {
    if (this->ESPDraw.load(std::memory_order_acquire)) {
        this->ESP();
    }
    auto w = MulNX::UI::RAIIWindow("快捷操作", this->ShowWindow);
    if (!w)return true;

    MulNX::UI::SliderFloat("roll调整", this->controlView.InputRoll, -179, 179);

    auto* pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();
    if (ImGui::SliderFloat("fov调整", pGlobalFOV, 0, 179));
    MulNX::UI::Checkbox("摄像机模式", this->controlView.CameraMode);
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
#ifdef _DEBUG
    try {
        for (int i = 0;i <= this->Modules.client.dwGameEntitySystem_highestEntityIndex();i++) {
            auto* pEntity = this->Modules.client.GetBaseEntity(i);
            if (!pEntity)continue;
            auto hPawn = MulNX::MRead(pEntity->As<CS2::CBasePlayerController>()->hPawn());
            if (!hPawn.Valid())continue;
            auto* pPawn = this->Modules.client.GetBaseEntity(hPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
            if (!pPawn)continue;
            auto* pGameSceneNode = MulNX::MRead(pPawn->pGameSceneNode());
            if (!pGameSceneNode)continue;
            auto* bones = MulNX::MRead(static_cast<CS2::CSkeletonInstance*>(pGameSceneNode)->unkBoneArray());
            if (!bones)continue;

            auto* pWeaponServices = MulNX::MRead(pPawn->pWeaponServices());
            auto hAc = MulNX::MRead(pWeaponServices->hActiveWeapon());
            auto* pWeapon = this->Modules.client.GetBaseEntityFromHandle(hAc.GetIndexInEntityList())->As<CS2::C_BasePlayerWeapon>();
            auto* pWeaponsGameSceneNode = MulNX::MRead(pWeapon->pGameSceneNode());
            auto* bones2 = MulNX::MRead(static_cast<CS2::CSkeletonInstance*>(pWeaponsGameSceneNode)->unkBoneArray());



            MulNX::TransInfo info;
            info.pMatrix = this->GetViewMatrix();
            info.windowHeight = this->GetWinHeight();
            info.windowWidth = this->GetWinWidth();

            for (int i = 0;i < 34;++i) {
                DirectX::XMFLOAT3 pos = MulNX::MRead(bones->at(i));
                DirectX::XMFLOAT3 pos2 = MulNX::MRead(bones2->at(i));
                MulNX::UI::DrawWorldPoint(pos, info, std::to_string(i).c_str());
                MulNX::UI::DrawWorldPoint(pos2, info, std::to_string(i).c_str());
            }
        }
    }
    catch (const std::runtime_error& e) {
        this->ISys().LogWarning("捕获到在绘制时发生的异常");
    }


#endif
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
    this->NeedUINode = true;

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

    //this->LocalPlayer.pGlobalFOV = this->CvarSystem.GetCvar("fov_cs_debug")->GetPtr<float>();

    if (this->Modules.client.Valid) {
        // 搜索 .text 段
        auto textRegion = this->Modules.client.GetTextRegion();
        if (textRegion.IsValid()) {
            // 搜索特征码
            const auto& pattern = MulNX::CS2::Signatures::CallIsPlayingDemo;
            auto target = textRegion.FindRegion(pattern);

            if (target.IsValid()) {
                this->MyHook = MulNX::Memory::HookEx::Create(target.Data(), 16, false, [this](RegContext* ctx)->void {
                    return this->HandleOverrideView((void*)ctx->rsi);
                    });
                this->MyHook->Attach();
            }
        }
    }

    return true;
}

int CSController::BasicUpdate() {
    // 获取CS2全局变量
    this->CSGlobalVars.Address = MulNX::MRead<uintptr_t>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwGlobalVars);
    
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

            //auto pOrigin = (*entity->pGameSceneNode())->vecAbsOrigin();

            // auto view = std::make_shared<Views>();
            // view->OriginX = pOrigin->x;
            // view->OriginY = pOrigin->y;
            // view->OriginZ = pOrigin->z;
            // view->FOV = 90;
            // view->AnglesX = 0;
            // view->AnglesY = 0;
            // view->AnglesZ = 0;
            // this->ViewToGame.store(view);
        }
    }
#ifdef _DEBUG

    // int highestEntityIndex = *this->Modules.client.dwGameEntitySystem_highestEntityIndex();
    // for (int i = 0;i <= highestEntityIndex;i++) {
    //     auto entity = this->Modules.client.GetBaseEntity(i);
    //     if (entity == 0) {
    //         continue;
    //     }
    //     auto hPawn = *entity->As<CS2::CBasePlayerController>()->hPawn();
    //     if (!hPawn.Valid()) {
    //         continue;
    //     }
    //     auto* pawn = this->Modules.client.GetBaseEntity(hPawn.GetIndexInEntityList());
    //     if (!pawn) {
    //         continue;
    //     }
    //     auto* pRot = pawn->As<CS2::C_CSPlayerPawn>()->angEyeAngles();
    //     auto t = std::string(*(*pawn->pClassInfo())->pName());
    //     if (t.find("layer") == std::string::npos) {
    //         continue;
    //     }
    //     auto* pPlayerRot = (*pawn->pGameSceneNode())->angAbsRotation();
    //     auto* pPlayerRot2 = (*pawn->pGameSceneNode())->angRotation();
    //     auto* pPlayerRot3 = (*pawn->pGameSceneNode())->angWrappedLocalRotation();

    //     auto** ppWeaponServices = pawn->As<CS2::C_BasePlayerPawn>()->pWeaponServices();
    //     if (!ppWeaponServices) {
    //         continue;
    //     }
    //     auto pWeaponServices = *ppWeaponServices;
    //     if (!pWeaponServices) {
    //         continue;
    //     }
    //     auto phWeapon = pWeaponServices->hActiveWeapon();
    //     if (!phWeapon || reinterpret_cast<uintptr_t>(phWeapon) == 0xFFFFFFFFFFFFFFFF) {
    //         continue;
    //     }
    //     CS2::CHandle<CS2::C_BasePlayerWeapon> hWeapon = *phWeapon;
    //     if (!hWeapon.Valid()) {
    //         continue;
    //     }
    //     auto weapon = this->Modules.client.GetBaseEntity(hWeapon.GetIndexInEntityList());
    //     if (!weapon) {
    //         continue;
    //     }

    //     auto pGameSceneNode = *weapon->pGameSceneNode();
    //     if (!pGameSceneNode) {
    //         continue;
    //     }
    //     char* pName = *(*weapon->pClassInfo())->pName();
    //     auto* pWeaponPos = pGameSceneNode->vecAbsOrigin();

    //     if (!pWeaponPos) {
    //         continue;
    //     }
    //     DirectX::XMFLOAT3 pos = *pWeaponPos;
    //     DirectX::XMFLOAT3 rot = *pRot;

    //     auto zipaiView = std::make_shared<Views>();
    //     zipaiView->AnglesX = 89.0f;
    //     zipaiView->AnglesY = rot.y;
    //     zipaiView->AnglesZ = rot.z;

    //     zipaiView->OriginX = pos.x;
    //     zipaiView->OriginY = pos.y;
    //     zipaiView->OriginZ = pos.z + 120.0f;

    //     zipaiView->FOV = 90.0f;

    //     //this->ViewToGame.store(zipaiView);
    //     //this->GlobalVars->CampathPlaying = true;
    // }
#endif 
    return 0;
}
int CSController::EntityListUpdate() {
    std::unique_lock lock(this->GetMutex());
    // 玩家控制器，地图上从1到10，位于索引1到10处
    for (int i = 1; i <= 10; ++i) {
        auto* controller = this->Modules.client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
        if (!controller)continue;
        auto hPawn = MulNX::MRead(controller->hPawn());
        if (!hPawn.Valid())continue;
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
    Result = this->EntityListUpdate();
    if (Result)return Result;
    

    uintptr_t ppPlantedC4 = ppPlantedC4 = MulNX::MRead<uintptr_t>(this->Modules.client.GetBaseAddress() + cs2_dumper::offsets::client_dll::dwPlantedC4);
    if (ppPlantedC4) {
        this->PlantedC4.Address = MulNX::MRead<uintptr_t>(ppPlantedC4);
        this->PlantedC4.Update();
    }


    this->CSGlobalVars.Update();
    this->GlobalVars->InGamePlaying = true;

    return 0;
}


void CSController::HandleFreeCameraPath(const CameraSystemIO* const IO) {
    DirectX::XMFLOAT4 PosAndFOV = IO->Frame.GetPositionAndFOV();
    DirectX::XMFLOAT3 RotEuler = IO->Frame.GetRotationEuler();
#ifdef _DEBUG
    static MulNX::Math::Frame thisFrame;
    if (thisFrame != IO->Frame) {
        thisFrame = IO->Frame;
        this->ISys().LogInfo(thisFrame.GetMsg());
    }
#endif // _DEBUG
    auto view = std::make_shared<Views>();
    view->OriginX = PosAndFOV.x;
    view->OriginY = PosAndFOV.y;
    view->OriginZ = PosAndFOV.z;
    view->FOV = PosAndFOV.w;
    view->AnglesX = RotEuler.x;
    view->AnglesY = RotEuler.y;
    view->AnglesZ = RotEuler.z;
    this->controlView.InputRoll.store(view->AnglesZ, std::memory_order_release);
    this->controlView.ViewToGame.store(view);
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

// void CSController::HandleAimAtEntity(int AimTargetIndexInMap) {
//     DirectX::XMFLOAT3 LocalEyePos = this->LocalPlayer.Entity.Pawn.GetEyePos();
//     DirectX::XMFLOAT3 TargetEyePos;
//     int TargetIndexInEntityList = this->GetIndexInEntityListFromIndexInMap(AimTargetIndexInMap);
//     if (TargetIndexInEntityList == -1)return;
//     TargetEyePos = this->EntityList.GetEntity(TargetIndexInEntityList).Pawn.GetEyePos();

//     DirectX::XMFLOAT3 dir = TargetEyePos - LocalEyePos;
//     DirectX::XMFLOAT3 TargetViewAngles{};
//     MulNX::Math::CSDirToEuler(dir, TargetViewAngles);

//     this->LocalPlayer.SetViewAngle(TargetViewAngles);
// }

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