#include "CSController.hpp"

using namespace MulNX::Memory::ReadWrite;

bool CSController::ExecuteCommand(const std::string& cmd) {
    this->executor(0, cmd.c_str(), 1);
    return true;
}
float* CSController::GetViewMatrix() {
    return this->Modules.client.dwViewMatrix();
}
MulNX::Math::View CSController::GetView()const {
    MulNX::Math::View view;
    view.position.x = this->controlView.currentView.OriginX.load(std::memory_order_acquire);
    view.position.y = this->controlView.currentView.OriginY.load(std::memory_order_acquire);
    view.position.z = this->controlView.currentView.OriginZ.load(std::memory_order_acquire);
    view.rotation.x = this->controlView.currentView.AnglesX.load(std::memory_order_acquire);
    view.rotation.y = this->controlView.currentView.AnglesY.load(std::memory_order_acquire);
    view.rotation.z = this->controlView.currentView.AnglesZ.load(std::memory_order_acquire);
    view.FOV = this->controlView.currentView.FOV.load(std::memory_order_acquire);

    view.dof.NearBlurry = *this->controlView.dofs.pNearBlurry;
    view.dof.NearCrisp = *this->controlView.dofs.pNearCrisp;
    view.dof.FarCrisp = *this->controlView.dofs.pFarCrisp;
    view.dof.FarBlurry = *this->controlView.dofs.pFarBlurry;

    return view;
}
float CSController::GetTime()const {
    float time = 0;
    uintptr_t GlobalVarsPointer = this->CSGlobalVars.GetCurrentTimePointer();
    time = MRead<float>(GlobalVarsPointer);
    static float timeBuffer = time;
    if (timeBuffer < time) {
        timeBuffer = time;
    }
    // 这个延迟是因为CS2的demo系统，它的时间阅读有时候会出现回跳，这里简单过滤一下
    else if (timeBuffer - time > 0.025f) {
        timeBuffer = time;
    }
    return timeBuffer;
}

float CSController::GetWinWidth()const {
    return this->controlView.currentView.WindowWidth.load(std::memory_order_relaxed);
}
float CSController::GetWinHeight()const {
    return this->controlView.currentView.WindowHeight.load(std::memory_order_relaxed);
}
bool CSController::SpecPlayer(int IndexInMap) {
    this->ExecuteCommand("spec_mode 2;spec_player " + std::to_string(this->AL3DGameData.Players[IndexInMap].IndexInMap));
    return true;
}
D_Player& CSController::GetPlayerMsg(int Index) {
    //std::shared_lock lock(this->GetMtx());
    return this->AL3DGameData.Players[Index];
}
void CSController::spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot) {
    this->ExecuteCommand(std::format("spec_goto {} {} {} {} {}", pos.x, pos.y, pos.z, rot.x, rot.y));
    this->controlView.InputRoll.store(rot.z, std::memory_order_release);
}
void CSController::ClearViewOverride() {
    this->controlView.ViewToGame.store(nullptr, std::memory_order_release);
}