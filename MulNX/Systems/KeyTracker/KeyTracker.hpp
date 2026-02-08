#pragma once

#include "../../Core/ModuleBase/ModuleBase.hpp"

#include <thread>
#include <chrono>

namespace pugi {
	class xml_node;
}

namespace MulNX {
	//Usable Ctrl Shift Alt 虚拟键码 连击数
	class KeyCheckPack {
	public:
		bool Usable = false;
		bool Ctrl = false;
		bool Shift = false;
		bool Alt = false;

		unsigned char vkCode = 0;
		unsigned char ComboClick = 0;

		std::string GetMsg()const;
		void Refresh();
		bool WriteXMLNode(pugi::xml_node& node_KeyCheckPack);
		bool ReadXMLNode(pugi::xml_node& node_KeyCheckPack);

	};

	class KeyState {
	public:
		//在KeyTracker用private创建KeyState的情况下，部分非原子化的变量没有合法访问的接口
		std::atomic<bool> Current{};//跨线程，原子化
		bool Previous{};//线程内，非原子化
		std::atomic<unsigned char> ComboClick{};//跨线程，原子化 不认为连击能超过127
		std::atomic<unsigned char> BufferComboClick{};//跨线程，原子化 同上
		unsigned int LastPressTimeMs{};//线程内，非原子化，单位为毫秒
	};

	class KeyTracker final :public ModuleBase {
	private:
		std::atomic<bool> IfCreated{};//跨线程，原子化，标记是否已经创建好检测线程	

		const std::chrono::steady_clock::time_point ClockEpoch = std::chrono::steady_clock::now();//常量，只读，不需要原子化
		unsigned int CurrentTimeMs{};//线程内，非原子化，单位为毫秒
		std::atomic<unsigned int> Threshold = 200;//跨线程，原子化，单位为毫秒
		KeyState KeysState[256]{};//跨线程，内部有关跨线程的内容已经原子化
	public:
		KeyTracker() : ModuleBase() {
			//this->Type = ModuleType::KeyTracker;
		}

		bool Init()override;
		void ThreadMain()override;

		bool UpdateKeysState();//更新
		bool IsKeyPressed(const unsigned char vkCode)const;//读取是否被按下
		bool CheckComboClick(const unsigned char vkCode, const unsigned char TargetCombo);//读取并移除连击缓冲
		bool CheckWithPack(const KeyCheckPack& Pack);//读取并移除连击缓冲
		unsigned char CheckComboClickUnremove(const unsigned char vkCode)const;//读取连击（非缓冲）
		void ResetThreshold(const unsigned int Threshold);//重新设置阈值
	};
}