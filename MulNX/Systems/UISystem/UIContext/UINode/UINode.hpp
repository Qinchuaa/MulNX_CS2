#pragma once

#include <MulNX/Systems/HandleSystem/IHandleSystem.hpp>
#include <MulNX/Systems/MessageManager/IMessageManager.hpp>
#include <functional>

class MulNXUIContext;

class MulNXUINode {
public:
	std::string name{};
    std::function<void(MulNXUINode*)>MyFunc = nullptr;
    MulNX::IMessageManager* pMsgManager = nullptr;

	// 按照线程管理进行成员分类

    // 初始化即可
    MulNXHandle hSelf{};
    MulNXHandle HModule{};

	// 跨线程数据

	bool Active = true;
    std::atomic<bool>* buzy = nullptr;

	MulNXUIContext* MainContext = nullptr;

    MulNXUINode() = default;
    MulNXUINode(const MulNXUINode&) = default;
    MulNXUINode(MulNXUINode&&) = default;
    MulNXUINode& operator=(const MulNXUINode&) = default;
    MulNXUINode& operator=(MulNXUINode&&) = default;

	void Draw();

    bool CallUINode(std::string&& Name);
    bool SetNextUINode(std::string&& Name);
    bool PublishAsync(MulNX::Message&& Msg);

    static MulNXUINode Create(MulNX::ModuleBase* MB);
};