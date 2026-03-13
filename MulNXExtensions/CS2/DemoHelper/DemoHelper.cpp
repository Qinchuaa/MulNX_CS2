#include "DemoHelper.hpp"

#include <MulNX/MulNX.hpp>

#include <MulNXThirdParty/All_ImGui.hpp>



static std::atomic<int> ClickCount = 0;

bool DemoHelper::UINodeFunc(MulNXUINode* node) {
    auto ReadData = this->Data.load(std::memory_order_acquire);
	ImGui::Text("第一个异步模块");
	if (ImGui::Button("标记当前时间")) {
        node->SendToOwner(node->CreateMsg(0x101));
	}
	ImGui::Text("目前，时间列表容器是");
	ImGui::SameLine();
    if (ReadData->TimeMarks.empty()) {
		ImGui::Text("空的");
	}
	else{
        ImGui::Text("有 %d 个时间点的", (int)ReadData->TimeMarks.size());
        for (auto time : ReadData->TimeMarks) {
			ImGui::Text("时间点： %.3f 秒", time);
			ImGui::SameLine();
			std::string str = "跳转##" + std::to_string(time);
			if (ImGui::Button(str.c_str())) {
                MulNX::Message Msg = node->CreateMsg(0x102);
				Msg.p1.f = time;
                node->SendToOwner(std::move(Msg));
			}
		}
	}
	ImGui::Text("按钮已被点击 %d 次", ClickCount.load());

	return true;
}

bool DemoHelper::Init() {
	this->MainMsgChannel = this->ICreateAndGetMessageChannel();
	this->ISys()
		.SubscribeAsync("UISystem/UICommand");

    this->NeedUINode = true;
    return true;
}

void DemoHelper::ProcessMsg(MulNX::Message* Msg) {
	switch (Msg->type) {
        case "UISystem/UICommand"_hash: {
		this->ISys().LogSucc("测试成功");
		this->HandleUICommand(Msg);
		Msg->pMsgChannel->PushMessage(MulNX::Message("UISystem/ModuleResponse"_hash));
		break;
	}
	}
}

void DemoHelper::HandleUICommand(MulNX::Message* Msg) {
	switch (Msg->p2.i) {
	case 0x101: {
		this->MarkTime();
		ClickCount++;
		break;
	}
	case 0x102: {
		float data = Msg->p1.f;
		std::string str = "跳转到" + std::to_string(data);
		this->ISys().LogInfo(str);
	}
		
	}
}

void DemoHelper::VirtualMain() {
	this->EntryProcessMsg();

    auto data = std::make_shared<DemoHelperPrivateData>();
    data->TimeMarks = this->Marks;
    this->Data.store(std::move(data), std::memory_order_release);
}

bool DemoHelper::MarkTime() {
	this->Marks.push_back(this->AL3D->GetTime());


	return true;
}