#include "AdvancedViewController.hpp"

#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool AdvancedViewController::Init() {
    this->CS = this->Core->ModuleManager()->FindModule<CSController>("CSController");
    this->SendUINode(this->GetName(), [this](MulNXUINode* node) {return this->Menu(node); });

    return true;
}

bool AdvancedViewController::Menu(MulNXUINode* node) {
    if (ImGui::CollapsingHeader("高级视角控制")) {
        MulNX::UI::Checkbox("启用高级视角控制", this->Enable);
        MulNX::UI::Checkbox("覆盖自视角", this->OverrideSelfView);
        MulNX::UI::Checkbox("始终计算视角", this->AlwaysCaulate);

        MulNX::UI::Checkbox("使用本地Pawn", this->useLocalPawn);
        MulNX::UI::Checkbox("强制头模式", this->forceHeadMode);
        // 骨骼来源模式：武器 / 人体
        MulNX::UI::Checkbox("人体骨骼模式", this->UseBodyBones);

        // 骨骼选择（三点定义坐标系）
        MulNX::UI::SliderInt("骨骼起点 (原点)", this->boneIndex1, 0, 127);
        MulNX::UI::SliderInt("骨骼终点 (前向)", this->boneIndex2, 0, 127);
        MulNX::UI::SliderInt("骨骼辅助 (上向参考)", this->boneIndex3, 0, 127);

        // 位置偏移（前, 左, 上） — 与输入系统约定对应：localPositionOffset (x=前, y=左, z=上)
        ImGui::Text("摄像机局部偏移 (前,左,上)");
        ImGui::SliderFloat("前移 (X)", &this->localPositionOffset.x, -200.0f, 200.0f);
        ImGui::SliderFloat("左移 (Y)", &this->localPositionOffset.y, -200.0f, 200.0f);
        ImGui::SliderFloat("上移 (Z)", &this->localPositionOffset.z, -200.0f, 200.0f);

        // 旋转偏移（统一为：俯仰, 偏航, 滚转）
        ImGui::Text("摄像机局部旋转 (俯仰,偏航,滚转)");
        ImGui::SliderFloat("俯仰 (Pitch)", &this->localRotationOffset.x, -89.0f, 89.0f);
        ImGui::SliderFloat("偏航 (Yaw)", &this->localRotationOffset.y, -180.0f, 180.0f);
        ImGui::SliderFloat("滚转 (Roll)", &this->localRotationOffset.z, -180.0f, 180.0f);

        // 平滑系数
        MulNX::UI::SliderFloat("平滑系数", this->CS->viewBuffer.SMOOTH_FACTOR, 0.0f, 1.0f);

        // 上向反转选项
        MulNX::UI::Checkbox("反转上向", this->InvertUp);
        // 绘制选项：原始骨骼点与坐标轴
        MulNX::UI::Checkbox("显示原始骨骼点", this->ShowOriginalBones);
        MulNX::UI::Checkbox("显示坐标轴", this->ShowCoordinateAxes);
        MulNX::UI::SliderFloat("坐标轴长度", this->AxisLength, 1.0f, 200.0f);

        // 绘制当前骨骼位置（调试用）
        auto boneInfo = this->CurrentBoneInfo.load(std::memory_order_acquire);
        if (boneInfo) {
            MulNX::TransInfo info;
            info.pMatrix = this->CS->GetViewMatrix();
            info.windowHeight = this->CS->GetWinHeight();
            info.windowWidth = this->CS->GetWinWidth();

            // 分两部分绘制：原始骨骼点 与 坐标轴
            if (this->ShowOriginalBones.load(std::memory_order_acquire)) {
                MulNX::UI::DrawWorldPoint(boneInfo->PosOrigin, info, "Origin");
                MulNX::UI::DrawWorldPoint(boneInfo->PosForward, info, "Forward");
                MulNX::UI::DrawWorldPoint(boneInfo->PosUp, info, "Up");
            }

            if (this->ShowCoordinateAxes.load(std::memory_order_acquire)) {
                float axisLen = this->AxisLength.load(std::memory_order_acquire);
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

    return true;
}