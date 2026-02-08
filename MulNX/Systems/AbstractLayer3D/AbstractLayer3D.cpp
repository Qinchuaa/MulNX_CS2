#include "AbstractLayer3D.hpp"

#include "../../Core/Cores.hpp"

#include "../MulNXiGlobalVars/MulNXiGlobalVars.hpp"
#include "../MessageManager/IMessageManager.hpp"
#include "../Debugger/IDebugger.hpp"
#include "../MulNXUISystem/IMulNXUISystem.hpp"

using namespace MulNX;

bool AbstractLayer3D::Init() {
	this->ISubscribe(MulNX::MsgType::Core_ReHook);
	return true;
}
void AbstractLayer3D::VirtualMain() {
	this->EntryProcessMsg();
	if (!this->GlobalVars->CampathPlaying) {
		//*this->CS->GetLocalPlayer().pGlobalFOV = 0.0f;
	}
	this->UpdateTime();
	return;
}
void AbstractLayer3D::ProcessMsg(MulNX::Message* Msg) {
	switch (Msg->Type) {
	case MulNX::MsgType::Core_ReHook: {
		this->IDebugger->AddSucc("已完成Hook重载！");
		break;
	}

	}
}

//接口实现

void AbstractLayer3D::SetCurrentTimePointer(uintptr_t pCurrentTime) {
	this->pCurrentTime = pCurrentTime;
}
uintptr_t AbstractLayer3D::GetCurrentTimePointer() {
    return this->pCurrentTime.load();
}
void AbstractLayer3D::UpdateTime() {
    float rawTime;
    //MulNX::Base::Memory::Read(this->pCurrentTime, rawTime);
    /*if (rawTime < 0)return;
    if (rawTime > 1000000.0f)return;
    if (rawTime < this->CurrentTime && this->CurrentTime - rawTime < 0.5f)return;
	
    this->CurrentTime = rawTime;*/
    
    return;
}
float AbstractLayer3D::GetTime()const {
	return this->CurrentTime;
}

//获取阶段已用时间（正计时）
float AbstractLayer3D::GetPhaseElapsedTime()const {
	return this->CurrentTime - this->PhaseStartTime;
}
//获取阶段剩余时间（倒计时）
float AbstractLayer3D::GetPhaseRemainingTime()const {
	return this->PhaseDuration - (this->CurrentTime - this->PhaseStartTime);
}
//获取阶段总时长
float AbstractLayer3D::GetPhaseDuration()const {
	return this->PhaseDuration;
}
//设置阶段开始时间基准
void AbstractLayer3D::SetPhaseStartTime(float startTime) {
	this->PhaseStartTime = startTime;
	return;
}
//设置阶段总时长
void AbstractLayer3D::SetPhaseDuration(float duration) {
	this->PhaseDuration = duration;
	return;
}


MulNX::Base::Math::SpatialState AbstractLayer3D::GetSpatialState()const {
	return this->GetSpatialStateFunc();
}
float* AbstractLayer3D::GetViewMatrix()const {
	return this->GetViewMatrixFunc();
}
float AbstractLayer3D::GetWinWidth()const {
	return this->CurrentWindowWidth;
}
float AbstractLayer3D::GetWinHeight()const {
	return this->CurrentWindowHeight;
}
bool AbstractLayer3D::CameraSystemIOOverride(const CameraSystemIO* const IO) {
	return this->CameraSystemIOOverrideFunc(IO);
}
bool AbstractLayer3D::ExecuteCommand(const char* command) {
	this->CmdInterface(command);
	return true;
}
bool AbstractLayer3D::ExecuteCommand(const std::string& command) {
	this->ExecuteCommand(command.c_str());
	return true;
}

bool AbstractLayer3D::UpdatePlayerMsg(D_Player&& PlayMsg) {
	std::unique_lock<std::shared_mutex> lock(this->GetMtx());
	this->GameData.Players[PlayMsg.IndexInMap] = std::move(PlayMsg);
	return true;
}
bool AbstractLayer3D::SpecPlayer(int IndexInMap) {
	this->ExecuteCommand("spec_mode 2;spec_player " + std::to_string(this->GameData.Players[IndexInMap].IndexInMap));
	return true;
}
D_Player& AbstractLayer3D::GetPlayerMsg(int Index) {
	//std::shared_lock lock(this->GetMtx());
	return this->GameData.Players[Index];
}