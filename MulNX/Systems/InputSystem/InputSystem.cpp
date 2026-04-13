#include "InputSystem.hpp"

#include <unordered_map>
#include <Windows.h>
#include <DirectXMath.h>
#include <MulNX/Base/Math/Translate/Translate.hpp>

bool MulNX::InputSystem::Init() {
    this->LastUpdateTime = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->ClockEpoch).count()) / 1000.0f;
    this->SendTask("输入检测线程", [this]()->bool {
        this->UpdateKeysState();
        std::this_thread::sleep_for(std::chrono::milliseconds(this->MyThreadDelta));
        return true;
        });
    return true;
}
bool MulNX::InputSystem::UpdateKeysState() {
    unsigned int Threshold = this->Threshold;
    unsigned int DThreshold = Threshold * 2;
    this->CurrentTimeMs = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - this->ClockEpoch).count());
    for (int vkCode = 0; vkCode < 256; ++vkCode) {
        SHORT keyState = GetAsyncKeyState(vkCode);
        this->KeysState[vkCode].Current = (keyState & 0x8000) != 0;
        //this->KeysState[vkCode].Current = (keyboardState[vkCode] & 0x80) != 0;
        if (this->KeysState[vkCode].BufferComboClick) {
            if (this->CurrentTimeMs - this->KeysState[vkCode].LastPressTimeMs > DThreshold) {
                this->KeysState[vkCode].BufferComboClick = 0;
            }
        }
        if (this->KeysState[vkCode].ComboClick) {
            if (this->CurrentTimeMs - this->KeysState[vkCode].LastPressTimeMs > Threshold) {
                this->KeysState[vkCode].BufferComboClick.store(this->KeysState[vkCode].ComboClick.load());
                this->KeysState[vkCode].ComboClick = 0;
            }
        }
        if (this->KeysState[vkCode].Current) {
            if (!this->KeysState[vkCode].Previous) {
                this->KeysState[vkCode].ComboClick++;
            }
            this->KeysState[vkCode].Previous = true;
            this->KeysState[vkCode].LastPressTimeMs = this->CurrentTimeMs;
        }
        else {
            this->KeysState[vkCode].Previous = false;
        }
    }
    return true;
}

bool MulNX::InputSystem::IsKeyPressed(const unsigned char vkCode)const {
    return KeysState[vkCode].Current;//返回指定键的当前状态
}

bool MulNX::InputSystem::CheckComboClick(const unsigned char vkCode, unsigned char TargetCombo) {
    if (TargetCombo < 0)return false;
    return this->KeysState[vkCode].BufferComboClick.compare_exchange_strong(TargetCombo, 0);
}

//要求按键检测包与当前状态完全匹配
bool MulNX::InputSystem::CheckWithPack(const KeyCheckPack& Pack) {
    if (!Pack.Usable)return false;
    // 检查Ctrl状态必须完全匹配
    bool ctrlPressed = this->IsKeyPressed(VK_CONTROL) ||
        this->IsKeyPressed(VK_LCONTROL) ||
        this->IsKeyPressed(VK_RCONTROL);
    if (Pack.Ctrl != ctrlPressed) return false;

    // 检查Shift状态必须完全匹配
    bool shiftPressed = this->IsKeyPressed(VK_SHIFT) ||
        this->IsKeyPressed(VK_LSHIFT) ||
        this->IsKeyPressed(VK_RSHIFT);
    if (Pack.Shift != shiftPressed) return false;

    // 检查Alt状态必须完全匹配
    bool altPressed = this->IsKeyPressed(VK_MENU) ||
        this->IsKeyPressed(VK_LMENU) ||
        this->IsKeyPressed(VK_RMENU);
    if (Pack.Alt != altPressed) return false;

    return this->CheckComboClick(Pack.vkCode, Pack.ComboClick);
}

unsigned char MulNX::InputSystem::CheckComboClickUnremove(const unsigned char vkCode)const {
    return this->KeysState[vkCode].ComboClick;
}

void MulNX::InputSystem::ResetThreshold(const unsigned int Threshold) {
    if (Threshold < 1)return;
    this->Threshold = Threshold;
}

void MulNX::FreeCameraController::Update(InputSystem* inputSystem) {
    float currentTime = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - inputSystem->ClockEpoch).count()) / 1000.0f;
    float deltaTime = currentTime - inputSystem->LastUpdateTime;
    inputSystem->LastUpdateTime = currentTime;

    // CS2 坐标轴：X前，Y左，Z上
    DirectX::XMFLOAT3 moveDir = { 0.0f, 0.0f, 0.0f };

    if (inputSystem->IsKeyPressed('W')) moveDir.x += 1.0f;  // 前进
    if (inputSystem->IsKeyPressed('S')) moveDir.x -= 1.0f;  // 后退
    if (inputSystem->IsKeyPressed('A')) moveDir.y += 1.0f;  // 左移
    if (inputSystem->IsKeyPressed('D')) moveDir.y -= 1.0f;  // 右移
    if (inputSystem->IsKeyPressed('R')) moveDir.z += 1.0f;  // 上移
    if (inputSystem->IsKeyPressed('V')) moveDir.z -= 1.0f;  // 下移

    // 归一化移动方向
    float moveLength = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y + moveDir.z * moveDir.z);
    if (moveLength > 0.0f) {
        moveDir.x /= moveLength;
        moveDir.y /= moveLength;
        moveDir.z /= moveLength;
    }

    // 使用四元数表示旋转，顺序：先 yaw（Z），再 pitch（Y），再 roll（X）
    // 注：DirectX 的四元数乘法顺序与矩阵一致（右乘表示先应用右侧）
    DirectX::XMVECTOR quatYaw = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), DirectX::XMConvertToRadians(this->Rotation.y));
    DirectX::XMVECTOR quatPitch = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), DirectX::XMConvertToRadians(this->Rotation.x));
    DirectX::XMVECTOR quatRoll = DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), DirectX::XMConvertToRadians(this->Rotation.z));

    // 组合顺序：先 yaw，再 pitch，再 roll（与矩阵顺序一致）
    DirectX::XMVECTOR quatTemp = DirectX::XMQuaternionMultiply(quatPitch, quatYaw);
    DirectX::XMVECTOR quatRot = DirectX::XMQuaternionMultiply(quatRoll, quatTemp);

    // 从四元数提取三个方向向量
    DirectX::XMFLOAT3 forward, left, up;
    DirectX::XMStoreFloat3(&forward, DirectX::XMVector3Rotate(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), quatRot));
    // 基向量 (0,1,0) 在本坐标系中表示向左，因此旋转后得到的是 left 向量
    DirectX::XMStoreFloat3(&left, DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), quatRot));
    DirectX::XMStoreFloat3(&up, DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), quatRot));

    auto speed = this->MoveSpeed.load(std::memory_order_acquire);
    if(inputSystem->IsKeyPressed(VK_SHIFT)) speed *= 0.2f; // 按住 Shift 减速

    // 应用移动：前/后（moveDir.x）、左/右（moveDir.y，正值为左）、上/下（moveDir.z）
    Position.x += (forward.x * moveDir.x + left.x * moveDir.y + up.x * moveDir.z) * speed * deltaTime;
    Position.y += (forward.y * moveDir.x + left.y * moveDir.y + up.y * moveDir.z) * speed * deltaTime;
    Position.z += (forward.z * moveDir.x + left.z * moveDir.y + up.z * moveDir.z) * speed * deltaTime;
}