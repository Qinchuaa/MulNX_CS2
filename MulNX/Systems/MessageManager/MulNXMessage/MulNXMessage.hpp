#pragma once

#include"../../../Base/Base.hpp"

namespace MulNX {
    enum class MsgType :uint32_t {
		Null,// 无效消息，用于实现默认构造

		Core_Begin,
		Core_Shutdown,
		Core_Tick1,
		Core_Tick5,
		Core_Tick10,
		Core_Tick15,
		Core_Tick20,
		Core_Tick30,
		Core_Tick45,
		Core_Tick60,
		Core_Tick30min,

		//Core_Check,
		//Core_CheckBack,
		Core_ReHook,

		Debugger_SetMaxInfoCount,// 调试系统，设置记录的最多的信息数量
        Debugger_SaveToFile,// 调试系统，保存日志到文件
        
        ModuleManager_RequestModuleInfo,// 模块管理器，组件向模块管理器请求目前加载的所有模块的信息
		ModuleManager_ResponseModuleInfo,// 模块管理器，模块管理器向请求者回应信息

		UISystem_Start,// 由核心向UI系统发送，启动UI系统

		UISystem_UIPull,// UI系统级，UI通知组件拉取信息
		UISystem_ModulePush,// UI系统级，组件响应信息，更新相关上下文

		UISystem_UIRequest,// 上下文级，UI请求组件提供某些数据
		UISystem_UICommand,// 上下文级，UI通知组件处理UI命令
		UISystem_ModuleResponse,// 上下文级，组件通知UI命令已处理完毕，可继续进行

		MQTT_ModulePublish,// 组件向MQTT组件发送发布消息请求，MQTT组件在合适时机对外发布消息
		MQTT_ModuleSubscribe,// 组件向MQTT组件发送订阅消息请求，MQTT组件在合适时机进行订阅，并路由消息到组件

		Game_NewRound,
		Game_RoundStart,

		Game_BombPlanted,
		Game_BombDefused,
		Game_Boomed,

		Game_RoundEnd,

		CameraSystem_CallSolution,
		CameraSystem_PlayingShutdown,

		Command_SpecPlayer// 指定观战某个玩家，Param1存玩家索引
	};

	// MulNX消息基类
	class Message {
	public:
		// 消息类型，用于区分消息
		MsgType Type;
		// 消息子类型，用于细分消息，一般用于私有消息
		uint32_t SubType;
		// 句柄，用于传递任意类型数据，需要通过句柄系统取出
		MulNXHandle Handle{};
		// Int型参数，这只是一个整数参数，可以用它传递想传递的整数
		int ParamInt;
		// 浮点型参数，这只是一个浮点参数，可以用它传递想传递的浮点数
		float ParamFloat;
		// 消息管道指针，一般用于指示消息来源
		IMessageChannel* pMsgChannel = nullptr;

		Message() = delete;
		Message(MsgType Type) :Type(Type) {}
		Message(MsgType Type, uint32_t SubType)
			: Type(Type)
			, SubType(SubType) {
		}
		Message(const Message& Other) = default;

		void Clear();
	};
}
