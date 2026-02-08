#pragma once

#include"MulNXSingleUIContext/MulNXSingleUIContext.hpp"

#include<queue>

class MulNXUIContext {
private:
	friend class MulNXSingleUIContext;
	bool CallSingleContext(const std::string& Name);
public:
	bool Active = true;
	MulNX::Core::Core* Core = nullptr;
	bool EnableErrorHandle = false;

	//入口点字符串
	std::string EntryDraw{};
	//由字符串映射到句柄
	std::unordered_map<std::string, MulNXHandle>CallMap{};
	//这里存储所有句柄
	std::vector<MulNXHandle>ContextOrder{};
	//然后从句柄得到具体的单上下文
	std::unordered_map<MulNXHandle, MulNX::Base::any_unique_ptr>ContextMap{};
	
	std::string next;

	void Draw();
	void AddSingleContext(MulNXHandle hContext, MulNX::Base::any_unique_ptr SContext);
	MulNXSingleUIContext* GetSingleContext(const MulNXHandle& hContext);
};