#pragma once

#include"../../Base/Base.hpp"

namespace MulNX {
	// 模块基类
	class ModuleBase {
		friend MulNX::Core::Core;
	protected:
		// 核心管理器指针
		MulNX::Core::Core* Core;
		// 全局变量指针
		MulNX::GlobalVars* GlobalVars = nullptr;
		// 按键追踪器指针
		MulNX::KeyTracker* KT = nullptr;
	public:
		// 主要消息管道指针
		MulNX::IMessageChannel* MainMsgChannel = nullptr;
	public:
		// 当前消息指针
		std::atomic<MulNX::Message*> CurrentMsg = nullptr;
	public:
		// 组件句柄
		MulNXHandle HModule;
		// 调试器指针
		IDebugger* IDebugger = nullptr;
		// 3D抽象层指针
		IAbstractLayer3D* AL3D = nullptr;
	private:
		// 消息管理器指针
		MulNX::IMessageManager* IMsgManager = nullptr;
	protected:
		// 窗口显示标志
		std::atomic<bool> ShowWindow = false;
		// 运行标志
		std::atomic<bool> Running = false;
		// 线程对象成员
		std::thread MyThread;
		// 线程运行状态
		std::atomic<bool>MyThreadRunning = false;
		// 线程执行间隔，默认以100Hz基准执行
		std::atomic<int> MyThreadDelta = 10;
		// 线程锁
		std::shared_mutex MyThreadMutex;
	public:
		std::shared_mutex& GetMutex() { return this->MyThreadMutex; }
	public:
		// 删除不需要的构造函数
		ModuleBase(const ModuleBase&) = delete;
		ModuleBase(ModuleBase&&) = delete;
		ModuleBase& operator=(const ModuleBase&) = delete;
		ModuleBase& operator=(ModuleBase&&) = delete;
		// 提供输入核心管理器指针的构造函数，进行绑定
		ModuleBase();
		// 虚析构函数确保正确调用析构函数
		virtual ~ModuleBase();
	protected:
		// 线程析构辅助函数
		void CloseMyThread();
	public:
		// 通用函数：

		// 模块时间控制接口
		void SetMyThreadDelta(int Delta);

		// 虚函数要求：
	private:
		// 初始化，拉取各种依赖
		virtual bool Init() = 0;

		// 虚拟主循环，执行组件逻辑
		virtual void VirtualMain();
		// 线程主循环，执行组件线程逻辑
		virtual void ThreadMain();
		// 消息处理函数，只需处理即可，消息会由入口点释放
		virtual void ProcessMsg(MulNX::Message* Msg);

		// 菜单绘制
		virtual void Menu();
		// 窗口绘制
		virtual void Windows();


		// 基本函数
	protected:
		// 基础初始化
		bool BaseInit();
		// 基础主循环
		void BaseVirtualMain();
		// 基础消息处理
		void BaseProcessMsg();
		// 基础菜单
		void BaseMenu();
		// 基础窗口
		void BaseWindows();

		// 入口点
	public:
		// 初始化入口
		bool EntryInit(MulNX::Core::Core* Core);
		// 主循环入口
		void EntryVirtualMain();
		// 创建线程入口
		bool EntryCreateThread();
	protected:
		// 消息处理入口
		void EntryProcessMsg();
	private:
		// 菜单入口
		void EntryMenu();
	public:
		// 窗口入口
		void EntryWindows();
		// 窗口控制
		void OpenWindow();
		void CloseWindow();
		bool IsWindowOpen()const;

		MulNX::Core::Core* GetCore()const {
			return this->Core;
		}

		// 工具函数

		// 自动注册
		void IRegiste();
		// 自动订阅消息类型
		void ISubscribe(MulNX::MsgType MsgType);
		// 自动发送消息
		void IPublish(MulNX::Message&& Msg);
		// 根据类型自动构建消息并发送
		void IPublish(MulNX::MsgType Msg);
		// 自动创建私有消息管道
		MulNX::IMessageChannel* ICreateAndGetMessageChannel();
	};

	// 自动子窗口管理类，一般在Menu函数中使用
	class AutoChild {
	private:
		const ModuleBase* Module = nullptr;
	public:
		// 构造时开始子窗口
		AutoChild(const ModuleBase* Module, const std::string& Name, const float HeightRatio = 1.0f, const float WidthRatio = 1.0f);
		// 析构时结束子窗口
		~AutoChild();
		// 禁止拷贝
		AutoChild(const AutoChild&) = delete;
		AutoChild& operator=(const AutoChild&) = delete;
	};
}