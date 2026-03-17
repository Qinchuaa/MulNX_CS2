#pragma once

#include <MulNX/Systems/HandleSystem/IHandleSystem.hpp>
#include <functional>

class MulNXUIContext;

class MulNXUINode {
public:
	std::string name{};
    std::function<void(MulNXUINode*)>MyFunc = nullptr;

	// 按照线程管理进行成员分类

    // 初始化即可
    MulNXHandle hSelf{};
    MulNXHandle HModule{};
	MulNX::IMessageChannel* OwnerMsgChannel = nullptr;
	MulNX::IMessageChannel* MyMsgChannel = nullptr;

	// 跨线程数据

	bool Active = true;
	bool WaitingResponse = false;
	//std::atomic<MulNX::Message*>pUpdateData = nullptr;


	MulNXUIContext* MainContext = nullptr;

    MulNXUINode() = default;
    MulNXUINode(const MulNXUINode&) = default;
    MulNXUINode(MulNXUINode&&) = default;
    MulNXUINode& operator=(const MulNXUINode&) = default;
    MulNXUINode& operator=(MulNXUINode&&) = default;

	void Draw();
	bool SendToOwner(MulNX::Message&& Msg);
	MulNX::Message CreateMsg(int SubType);
	MulNXHandle CreateStringHandle(std::string&& Str);

    bool CallUINode(std::string&& Name);
    bool SetNextUINode(std::string&& Name);

    static MulNXUINode Create(const MulNX::ModuleBase* const MB);
    static bool CreateAndRegiste(MulNX::ModuleBase* const MB, std::string&& Name, std::function<void(MulNXUINode*)>MyFunc);
};