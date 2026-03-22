#include "DemoHelper.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>

static std::atomic<int> ClickCount = 0;

bool DemoHelper::UINodeFunc(MulNXUINode* node) {
    auto w = MulNX::UI::RAIIWindow("Demo辅助", this->ShowWindow);
    if (!w)return true;
    auto ReadData = this->Data.load(std::memory_order_acquire);
	ImGui::Text("第一个异步模块");
    if (ImGui::Button("标记当前时间")) {
        MulNX::Message msg("DemoHelper/MarkTime"_hash);
        node->PublishAsync(std::move(msg));
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
                MulNX::Message Msg("DemoHelper/JumpTIme"_hash);
				Msg.p1.f = time;
                node->PublishAsync(std::move(Msg));
			}
		}
	}
	ImGui::Text("按钮已被点击 %d 次", ClickCount.load());

	return true;
}

bool DemoHelper::Init() {
    this->ISys()
        .SubscribeAsync("DemoHelper/MarkTime")
        .SubscribeAsync("DemoHelper/JumpTIme")
        ;

    this->NeedUINode = true;
    return true;
}

void DemoHelper::ProcessMsg(MulNX::Message& Msg) {
	switch (Msg.type) {
    case "DemoHelper/MarkTime"_hash: {
        this->MarkTime();
        ClickCount++;
        break;
    }
    case "DemoHelper/JumpTIme"_hash: {
        float data = Msg.p1.f;
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