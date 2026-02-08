#pragma once

#include"../../../../Base/Base.hpp"

#include"../../../HandleSystem/HandleSystem.hpp"


class MulNXUIContext;

class MulNXSingleUIContext {
public:
    MulNX::Base::any_unique_ptr pBuffer = nullptr;
	std::string name{};
	std::function<void(MulNXSingleUIContext*)>MyFunc = nullptr;

	template<typename T>
	MulNX::Base::DataRead<T> GetRead() {
		auto* buf = this->pBuffer.get<MulNX::Base::TripleBuffer<T>>();
		return MulNX::Base::DataRead<T>(static_cast<MulNX::Base::TripleBufferBase*>(buf));
	}
	template<typename T>
	MulNX::Base::DataWrite<T> GetWrite() {
		auto* buf = this->pBuffer.get<MulNX::Base::TripleBuffer<T>>();
		return MulNX::Base::DataWrite<T>(static_cast<MulNX::Base::TripleBufferBase*>(buf));
	}

	//按照线程管理进行成员分类

	//初始化即可
	MulNXHandle HModule{};
	MulNX::IMessageChannel* OwnerMsgChannel = nullptr;
	MulNX::IMessageChannel* MyMsgChannel = nullptr;

	//跨线程数据

	std::atomic<bool>Active = true;
	std::atomic<bool>WaitingResponse = false;
	//std::atomic<MulNX::Message*>pUpdateData = nullptr;


	MulNXUIContext* MainContext = nullptr;

	void Draw();
	bool SendToOwner(MulNX::Message&& Msg);
	MulNX::Message CreateMsg(uint32_t SubType);
	MulNXHandle CreateStringHandle(std::string&& Str);

	bool CallSingleUIContext(std::string&& Name);
	bool SetNextSingleUIContext(std::string&& Name);

	static MulNX::Base::any_unique_ptr Create(const MulNX::ModuleBase* const MB);

	MulNXSingleUIContext() = default;
	//禁止拷贝
	MulNXSingleUIContext(const MulNXSingleUIContext& Other) = delete;
};