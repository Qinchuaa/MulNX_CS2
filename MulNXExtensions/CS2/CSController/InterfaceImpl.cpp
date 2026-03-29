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
float CSController::GetTime() {
    float time = 0;
    try {
        time = MulNX::MRead<float>(this->CSGlobalVars->fCurrentTime());
    }
    catch (const bad_memory_read& e) {
        this->ISys().LogError("读取游戏时间失败");
        return 0;
    }
    return time;
}
bool CSController::JumpTime(const float time) {
    // 这个函数的实现思路是通过demo_gototick命令跳转到指定时间的tick上，CS2每秒钟有64个tick，所以需要将时间转换为tick
    float startTime = MulNX::MRead(this->Modules.client.dwGameRules()->fWarmupPeriodEnd());
    float targetGameTime = time - startTime;
    if (targetGameTime < 0) {
        // 时间不能为负
        return false;
    }
    int tick = static_cast<int>(targetGameTime * 64.0f);
    std::string command = "demo_gototick " + std::to_string(tick);
    this->ExecuteCommand(command);
    return true;
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
void CSController::SetDOF(const MulNX::Math::DOFParam& dof) {
    *this->controlView.dofs.pNearBlurry = dof.NearBlurry;
    *this->controlView.dofs.pNearCrisp = dof.NearCrisp;
    *this->controlView.dofs.pFarCrisp = dof.FarCrisp;
    *this->controlView.dofs.pFarBlurry = dof.FarBlurry;
}