#pragma once

#include <MulNX/Base/MulNXHandle/MulNXHandle.hpp>
#include "ISys/ISys.hpp"
#include <shared_mutex>
#include <thread>
#include <functional>

namespace MulNX {
    // 模块基类
	class ModuleBase {
        friend MulNX::Core::Core;
        friend C_ISys;
    private:
        // 消息管理器指针
        MulNX::IMessageManager* IMsgManager = nullptr;
        // 路径管理器指针
        MulNX::PathManager* pPathManager = nullptr;
    protected:
        // 父模块句柄
        MulNXHandle hParent{};
        // 模块名称，唯一标识
        std::string ModuleName{};
        // 核心管理器指针
        MulNX::Core::Core* Core = nullptr;
		// 全局变量指针
		MulNX::GlobalVars* GlobalVars = nullptr;
        // 线程运行状态
        std::atomic<bool>MyThreadRunning = false;
        // 线程执行间隔，默认以100Hz基准执行
        std::atomic<int> MyThreadDelta = 10;
	public:
        // 按键追踪器指针
        MulNX::InputSystem* pInputSystem = nullptr;
		// 组件句柄
		MulNXHandle HModule;
		// 3D抽象层指针
        IAbstractLayer3D* AL3D = nullptr;
        // 调试器指针
        IDebugger* IDebugger = nullptr;
        // 主要消息管道指针
        MulNX::IMessageChannel* MainMsgChannel = nullptr;
        // 用于指示UI不应该再发送消息
        std::atomic<bool> UIBusy = false;
        // 线程锁
        std::shared_mutex smutex;
    public:
		// 删除不需要的构造函数
		ModuleBase(const ModuleBase&) = delete;
		ModuleBase(ModuleBase&&) = delete;
		ModuleBase& operator=(const ModuleBase&) = delete;
		ModuleBase& operator=(ModuleBase&&) = delete;
        ModuleBase() = default;
		// 虚析构函数确保正确调用析构函数
        virtual ~ModuleBase() = default;
	private:
        // 虚函数要求：
        
        // 初始化
		virtual bool Init() = 0;

		// 消息处理函数，只需处理即可，消息会由入口点释放
        virtual void ProcessMsg(MulNX::Message& Msg) {};

		// 基本函数：

		// 基础初始化
        bool BaseInit();  
        // 入口点
    public:
        // 虚拟主循环，执行组件逻辑
        virtual void VirtualMain() {};
        // 初始化入口
		bool EntryInit(MulNX::Core::Core* Core);
	protected:
		// 消息处理入口
		void EntryProcessMsg();
        // 通过任意函数，发送一个UI节点
        bool SendUINode(std::string&& name, std::function<void(MulNXUINode*)>&& func);
        void SendTask(std::string&& workerName, std::function<bool()>&& task);
    public:
        // 设置模块名称
        bool SetName(std::string&& Name);
        std::string GetName()const;
        // 得到核心指针
        MulNX::Core::Core* GetCore()const { return this->Core; }
        // 设置父模块句柄
        void SetParent(MulNXHandle hModule);
        // 是否有父模块
        bool HasParent();
        // 便捷窗口显示标志
        std::atomic<bool> ShowWindow = false;
        // 模块时间控制接口
        void SetMyThreadDelta(int Delta);

        // 系统服务包装器(原则上是protected权限)
        C_ISys ISys();
    };
}