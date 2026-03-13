#include "CSController.hpp"

bool CSController::ExecuteCommand(const std::string& cmd) {
    this->executor(0, cmd.c_str(), 1);
    return true;
}
float* CSController::GetViewMatrix()const {
    return this->LocalPlayer.ViewMatrix;
}
MulNX::Math::View CSController::GetView()const {
    auto view = this->LocalPlayer.GetView();
    view.FOV = this->outFOV.load(std::memory_order_acquire);
    view.rotation.z = this->atoRoll.load(std::memory_order_acquire);
    return view;
}
float CSController::GetTime()const {
    float time = 0;
    uintptr_t GlobalVarsPointer = this->CSGlobalVars.GetCurrentTimePointer();
    MulNX::Memory::Read<float>(GlobalVarsPointer, time);
    return time;
}

float CSController::GetWinWidth()const {
    return this->AL3DCurrentWindowWidth;
}
float CSController::GetWinHeight()const {
    return this->AL3DCurrentWindowHeight;
}
bool CSController::SpecPlayer(int IndexInMap) {
    this->ExecuteCommand("spec_mode 2;spec_player " + std::to_string(this->AL3DGameData.Players[IndexInMap].IndexInMap));
    return true;
}
D_Player& CSController::GetPlayerMsg(int Index) {
    //std::shared_lock lock(this->GetMtx());
    return this->AL3DGameData.Players[Index];
}