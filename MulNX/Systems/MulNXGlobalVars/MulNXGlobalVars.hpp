#pragma once

#include "../../Core/ModuleBase/ModuleBase.hpp"

#include "MulNXGlobalVarsOnlyRead.hpp"

#include <atomic>

// MulNXiGlobalVars类，存储全局变量
// 这些全局变量为线程安全的，可以在多线程环境下使用
// 只存储系统关键变量，避免滥用全局变量
// 其它模块间变量互相访问，需要自行保证线程安全

namespace MulNX {
	class GlobalVars final :public ModuleBase {
	private:
		void PublishTickAll();
		void Tick();
	public:

        // 框架系统是否就绪
        std::atomic<bool>SystemReady = false;
        // 调试功能设置
        // 调试模式下提供更多功能，但可能影响性能和稳定性
        std::atomic<bool>DebugMode = false;
        // 是否正在游戏对局中
		std::atomic<bool>InGamePlaying = false;
        // 是否正在播放摄像机轨道
		std::atomic<bool>CampathPlaying = false;
        // 核心心跳
		std::atomic<uint32_t>CoreTick = 0;
        // 关键配置：是否让部分高危错误直接引发崩溃
        std::atomic<bool>DangerousErrorShouldTerminate = false;

		bool Init()override;
		void VirtualMain()override;
	};
}