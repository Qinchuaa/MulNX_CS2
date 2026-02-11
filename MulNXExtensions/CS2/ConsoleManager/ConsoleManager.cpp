#include "ConsoleManager.hpp"

#include "../GameSettingsManager/GameSettingsManager.hpp"
#include "../GameCfgManager/GameCfgManager.hpp"

#include <MulNX/MulNX.hpp>

#include <MulNXExtensions/MiniMap/MiniMap.hpp>

#include <MulNX/ThirdParty/All_ImGui.hpp>

bool ConsoleManager::Init() {
    auto SingleContext = MulNXUINode::Create(this);
    auto* SContextPtr = SingleContext.get<MulNXUINode>();
    SContextPtr->name = "ConsoleManager";
    SContextPtr->MyFunc = [this](MulNXUINode* This)->void {
        this->Menu();
    };

    MulNX::Message Msg(MulNX::MsgType::UISystem_ModulePush);
    Msg.Handle = this->Core->IHandleSystem().RegisteUnique(std::move(SingleContext));
    this->IPublish(std::move(Msg));
    
    return true;
}

void ConsoleManager::Menu() {
	MulNX::AutoChild Child(this,"ConsoleManager", 0.5f);// 占据半个窗口高度，另一半给VirtualUser

	if (ImGui::CollapsingHeader("调试器控制")) {
		// 调试功能设置
		// 调试模式下提供更多功能，但可能影响性能和稳定性
		static bool debugMode = this->GlobalVars->DebugMode;
		if (ImGui::Checkbox("调试模式（Debug Mode），提供更多功能，但可能影响性能和稳定性", &debugMode)) {
			this->GlobalVars->DebugMode = debugMode;
		}
		if (ImGui::Button("解限所有CS2控制台变量")) {
			int Count = 0;
			this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().UnlockHiddenCVars(Count);
			this->IDebugger->AddSucc("成功解限" + std::to_string(Count) + "个控制台命令！");
		}
		if (ImGui::Button("限住所有CS2控制台变量")) {
			int Count = 0;
			this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().LockAllCvars(Count);
			this->IDebugger->AddSucc("成功限住" + std::to_string(Count) + "个控制台命令！");
		}
		if(ImGui::Button("列出所有CS2控制台变量")) {
			this->IDebugger->AddInfo("---------------------------------------------------------------------------------");
			uint64_t idx = 0;
			this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().GetFirstCvarIterator(idx);
			while (idx!=0xFFFFFFFF) {
				C_ConVar* var = this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().GetCVarByIndex(idx);
				if (var) {
					std::string Name = var->szName ? var->szName : "未知";
					this->IDebugger->AddInfo("控制台命令：" + Name);
				}
				this->Core->ModuleManager()->FindModule<CSController>("CSController")->GetCvarSystem().GetNextCvarIterator(idx);
			}
			this->IDebugger->AddInfo("---------------------------------------------------------------------------------");
		}
	}

	if (ImGui::CollapsingHeader("初始化控制")) {
		if (ImGui::Button("初始化IPCer")) {
			this->IDebugger->AddInfo("正在尝试初始化IPCer");
			this->Core->IPCer().Init();
		}
		ImGui::SameLine();
		if (ImGui::Button("查看IPCer结果")) {
			this->ShowFilePath();
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

	return;
}

void ConsoleManager::VirtualMain() {

}

void ConsoleManager::ShowFilePath() {
	if (this->Core->IPCer().Inited) {
		this->IDebugger->AddInfo("---------------------------------------------------------------------------------");
		this->IDebugger->AddInfo(this->Core->IPCer().GetAllPathMsg());
		this->IDebugger->AddInfo("---------------------------------------------------------------------------------");
	}
	else {
		this->IDebugger->AddError("IPCer尚未初始化成功！");
	}
}