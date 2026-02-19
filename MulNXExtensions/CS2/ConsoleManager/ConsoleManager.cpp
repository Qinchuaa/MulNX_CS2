#include "ConsoleManager.hpp"

#include "../GameSettingsManager/GameSettingsManager.hpp"
#include "../GameCfgManager/GameCfgManager.hpp"

#include <MulNX/MulNX.hpp>

#include <MulNXExtensions/MiniMap/MiniMap.hpp>

#include <MulNXThirdParty/All_ImGui.hpp>

bool ConsoleManager::Init() {
    this->NeedUINode = true;
    return true;
}

bool ConsoleManager::UINodeFunc(MulNXUINode* ThisNode) {
	MulNX::AutoChild Child(this,"ConsoleManager", 0.5f);// 占据半个窗口高度，另一半给VirtualUser

	if (ImGui::CollapsingHeader("调试器控制")) {
		if (ImGui::Button("解限所有CS2控制台变量")) {
			int Count = 0;
			this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().UnlockHiddenCVars(Count);
			this->ISys().LogSucc("成功解限" + std::to_string(Count) + "个控制台命令！");
		}
		if (ImGui::Button("限住所有CS2控制台变量")) {
			int Count = 0;
			this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().LockAllCvars(Count);
			this->ISys().LogSucc("成功限住" + std::to_string(Count) + "个控制台命令！");
		}
		if(ImGui::Button("列出所有CS2控制台变量")) {
			this->ISys().LogInfo("---------------------------------------------------------------------------------");
			uint64_t idx = 0;
			this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().GetFirstCvarIterator(idx);
			while (idx!=0xFFFFFFFF) {
				C_ConVar* var = this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().GetCVarByIndex(idx);
				if (var) {
					std::string Name = var->szName ? var->szName : "未知";
					this->ISys().LogInfo("控制台命令：" + Name);
				}
				this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().GetNextCvarIterator(idx);
			}
			this->ISys().LogInfo("---------------------------------------------------------------------------------");
		}
	}

	if (ImGui::CollapsingHeader("游戏配置管理器控制")) {
		if (ImGui::Button("打开游戏配置管理器")) {
			this->Core->ModuleManager()->FindModule("GameCfgManager")->OpenWindow();
		}
		ImGui::SameLine();
		if (ImGui::Button("关闭游戏配置管理器")) {
			this->Core->ModuleManager()->FindModule("GameCfgManager")->CloseWindow();
		}
	}

	if (ImGui::CollapsingHeader("小地图控制")) {
		if (ImGui::Button("打开小地图")) {
			this->Core->ModuleManager()->FindModule("MiniMap")->OpenWindow();
		}
		ImGui::SameLine();
		if (ImGui::Button("关闭小地图")) {
			this->Core->ModuleManager()->FindModule("MiniMap")->CloseWindow();
		}
	}

	return true;
}

void ConsoleManager::VirtualMain() {

}