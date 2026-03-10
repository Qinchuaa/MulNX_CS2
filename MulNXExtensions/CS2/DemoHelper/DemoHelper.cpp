#include "DemoHelper.hpp"

#include <MulNX/MulNX.hpp>

#include <MulNXThirdParty/All_ImGui.hpp>

// 具体数据类型都隐藏在CPP文件
struct DemoHelperPrivateData {
	std::vector<float> TimeMarks{};
};

static std::atomic<int> ClickCount = 0;

static void MyDraw(MulNXUINode* This) {
	auto ReadData = This->GetRead<DemoHelperPrivateData>();
	auto ThisData = ReadData.get();
	ImGui::Text("第一个异步模块");
	if (ImGui::Button("标记当前时间")) {
		This->SendToOwner(This->CreateMsg(0x101));
	}
	ImGui::Text("目前，时间列表容器是");
	ImGui::SameLine();
	if(ThisData->TimeMarks.empty()){
		ImGui::Text("空的");
	}
	else{
		ImGui::Text("有 %d 个时间点的", (int)ThisData->TimeMarks.size());
		for(auto time : ThisData->TimeMarks){
			ImGui::Text("时间点： %.3f 秒", time);
			ImGui::SameLine();
			std::string str = "跳转##" + std::to_string(time);
			if (ImGui::Button(str.c_str())) {
				MulNX::Message Msg = This->CreateMsg(0x102);
				Msg.ParamFloat = time;
				This->SendToOwner(std::move(Msg));
			}
		}
	}
	ImGui::Text("按钮已被点击 %d 次", ClickCount.load());

	return;
}

bool DemoHelper::Init() {
	this->MainMsgChannel = this->ICreateAndGetMessageChannel();
	this->ISys()
		.SubscribeAsync("UISystem_UICommand");

	auto SingleContext = MulNXUINode::Create(this);
	auto* SContextPtr = SingleContext.get<MulNXUINode>();	
    SContextPtr->name = "DemoHelper";
    auto [Buffer, pBuffer] = MulNX::Base::make_any_unique<MulNX::Base::TripleBuffer<DemoHelperPrivateData>>();
    SContextPtr->pBuffer = std::move(Buffer);
	SContextPtr->MyFunc = MyDraw;

	this->hUINode = this->Core->IHandleSystem().RegisteUnique(std::move(SingleContext));

	MulNX::Message Msg("UISystem_ModulePush"_hash);
	Msg.Handle = this->hUINode;
	this->ISys().PublishAsync(std::move(Msg));

	return true;
}

void DemoHelper::ProcessMsg(MulNX::Message* Msg) {
	switch (Msg->Type) {
        case "UISystem_UICommand"_hash: {
		this->ISys().LogSucc("测试成功");
		this->HandleUICommand(Msg);
		Msg->pMsgChannel->PushMessage(MulNX::Message("UISystem_ModuleResponse"_hash));
		break;
	}
	}
}

void DemoHelper::HandleUICommand(MulNX::Message* Msg) {
	switch (Msg->SubType) {
	case 0x101: {
		this->MarkTime();
		ClickCount++;
		break;
	}
	case 0x102: {
		float data = Msg->ParamFloat;
		std::string str = "跳转到" + std::to_string(data);
		this->ISys().LogInfo(str);
	}
		
	}
}

void DemoHelper::VirtualMain() {
	this->EntryProcessMsg();
	auto UINode = this->Core->IUISystem().GetUIContext()->GetUINode(this->hUINode);
	if (!UINode) return; // 或跳过本帧处理
	auto data = UINode->GetWrite<DemoHelperPrivateData>();
	data->TimeMarks = this->Marks;
}

bool DemoHelper::MarkTime() {
	this->Marks.push_back(this->AL3D->GetTime());


	return true;
}