#pragma once

#include "MulNXUINode/MulNXUINode.hpp"

#include <queue>

class MulNXUIContext {
private:
	friend class MulNXUINode;
	bool CallUINode(const std::string& Name);
public:
	bool Active = true;
	MulNX::Core::Core* Core = nullptr;
	bool EnableErrorHandle = false;

	// 入口点字符串
	std::string EntryDraw{};
	// 由字符串映射到句柄
	std::unordered_map<std::string, MulNXHandle>CallMap{};
	// 然后从句柄得到具体的UI节点
	std::unordered_map<MulNXHandle, MulNXUINode>UINodeMap{};
	// 下一个要调用的UI节点名称
	std::string Next;

	void Draw();
    void AddUINode(MulNXHandle hUINode, MulNXUINode&& UINode);
    MulNXUINode* GetUINode(const MulNXHandle& hUINode);
};