#pragma once

#include<Windows.h>
#include<filesystem>

//前向声明实现类
class MulNXeCoreImpl;

class IPCer;
class MulNXeMainWindowManager;
class ConsoleManager;
class ConfigManager;

class MulNXeCore {
private:
	//构造和析构函数
	MulNXeCore();
	MulNXeCoreImpl* pImpl;
public:
	static MulNXeCore& GetInstance() {
		static MulNXeCore MulNXe;
		return MulNXe;
	}

	//是否运行中
	bool IsRunning;
	
	~MulNXeCore();
	//初始化
	bool Init();
	
	void VirtualMain();
	//模块接口
	IPCer& IPCer();
	ConfigManager& ConfigManager();
};