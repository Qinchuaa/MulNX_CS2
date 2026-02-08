#pragma once

#include"../../Core/ModuleBase/ModuleBase.hpp"

#include"MulNXiGlobalVarsOnlyRead.hpp"

#include<atomic>

//MulNXiGlobalVars类，存储全局变量
//这些全局变量为线程安全的，可以在多线程环境下使用
//只存储系统关键变量，避免滥用全局变量
//其它模块间变量互相访问，需要自行保证线程安全

namespace MulNX {
	class GlobalVars final :public ModuleBase {
	private:
		void PublishTickAll();
		void Tick();
	public:
		GlobalVars() : ModuleBase() {
			//this->Type = ModuleType::MulNXiGlobalVars;
		}

		std::atomic<bool>SystemReady = false;

		std::atomic<bool>DebugMode = false;
		std::atomic<bool>InGamePlaying = false;
		std::atomic<bool>CampathPlaying = false;
		std::atomic<uint32_t>CoreTick = 0;

		bool Init()override;
		void VirtualMain()override;
	};
}