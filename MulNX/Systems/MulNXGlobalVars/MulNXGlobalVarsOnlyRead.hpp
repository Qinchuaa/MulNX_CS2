#pragma once

// MulNXiGlobalVarsOnlyRead类，只读变量存储类
// 用于存储系统信息等，可以安全跨线程读取的变量
// 不允许写操作
class MulNXGlobalVarsOnlyRead {
public:
	inline static constexpr const char Version[] = MulNXVersion;
	inline static constexpr const char TimeStamp[] = "Built: " __DATE__ " " __TIME__;
	inline static constexpr const char FullName[] = "Multiple Next eXtension";
	inline static constexpr const char DeveloperName[] = "Co1Swet";
	inline static constexpr const bool IsDebugVersion = true;
};