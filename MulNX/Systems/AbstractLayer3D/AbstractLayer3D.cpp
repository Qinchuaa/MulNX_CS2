#include "AbstractLayer3D.hpp"

#include "../../Core/Cores.hpp"

#include "../MulNXGlobalVars/MulNXGlobalVars.hpp"
#include "../MessageManager/IMessageManager.hpp"
#include "../Debugger/IDebugger.hpp"
#include "../MulNXUISystem/IMulNXUISystem.hpp"
#include "../MulNXExtensions/WinExt/WinExt.hpp"

//接口实现

float MulNX::AbstractLayer3D::GetWinWidth()const {
    return this->AL3DCurrentWindowWidth;
}
float MulNX::AbstractLayer3D::GetWinHeight()const {
    return this->AL3DCurrentWindowHeight;
}
bool MulNX::AbstractLayer3D::SpecPlayer(int IndexInMap) {
	this->ExecuteCommand("spec_mode 2;spec_player " + std::to_string(this->AL3DGameData.Players[IndexInMap].IndexInMap));
	return true;
}
D_Player& MulNX::AbstractLayer3D::GetPlayerMsg(int Index) {
	//std::shared_lock lock(this->GetMtx());
	return this->AL3DGameData.Players[Index];
}