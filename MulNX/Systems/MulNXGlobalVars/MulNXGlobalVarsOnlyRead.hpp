#pragma once

// MulNXiGlobalVarsOnlyRead类，只读变量存储类
// 用于存储系统信息等，可以安全跨线程读取的变量
// 不允许写操作
class MulNXGlobalVarsOnlyRead {
public:
	static constexpr const char MulNXVersion[] = "V0.2.0";
	static constexpr const char TimeStamp[] = "Built: " __DATE__ " " __TIME__;
	static constexpr const char FullName[] = "Multiple Next eXtension";
	static constexpr const char DeveloperName[] = "Co1Swet";
	static constexpr bool IsDebugVersion = true;
};