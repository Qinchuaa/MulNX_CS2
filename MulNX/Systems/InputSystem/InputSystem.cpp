#include "InputSystem.hpp"

#include <unordered_map>
#include <Windows.h>
#include <DirectXMath.h>
#include <MulNX/Base/Math/Translate/Translate.hpp>

std::string MulNX::KeyCheckPack::GetMsg()const {
    std::ostringstream oss;

    if (this->Ctrl) oss << "Ctrl + ";
    if (this->Shift) oss << "Shift + ";
    if (this->Alt) oss << "Alt + ";

    //虚拟键码到中文的映射
    static const std::unordered_map<unsigned char, std::string> keyMap = {
        {0x70, "F1"}, {0x71, "F2"}, {0x72, "F3"}, {0x73, "F4"},
        {0x74, "F5"}, {0x75, "F6"}, {0x76, "F7"}, {0x77, "F8"},
        {0x78, "F9"}, {0x79, "F10"}, {0x7A, "F11"}, {0x7B, "F12"}
    };

    //处理字母键 (A-Z)
    if (this->vkCode >= 0x41 && this->vkCode <= 0x5A) {
        oss << static_cast<char>(this->vkCode);
    }
    //处理功能键 (F1-F12)
    else if (keyMap.find(this->vkCode) != keyMap.end()) {
        oss << keyMap.at(this->vkCode);
    }
    // 其他键 - 格式化为两位十六进制
    else {
        unsigned char uc = static_cast<unsigned char>(this->vkCode);
        oss << "键码[0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(uc) << "]";
    }

    // 始终显示连击数，包括x1
    oss << "x" << static_cast<int>(this->ComboClick);

    return oss.str();
}
void MulNX::KeyCheckPack::Refresh() {
    this->Usable = this->vkCode && this->ComboClick;
    return;
}

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