#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <yaml-cpp/yaml.h>

#include <thread>
#include <chrono>

namespace MulNX {
	//Usable Ctrl Shift Alt 虚拟键码 连击数
	class KeyCheckPack {
	public:
		bool Usable = false;
		bool Ctrl = false;
		bool Shift = false;
		bool Alt = false;

		unsigned char vkCode = 0;
		uint8_t ComboClick = 0;

		std::string GetMsg()const;
		void Refresh();
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

    class InputSystem final :public ModuleBase {
	private:
		std::atomic<bool> IfCreated{};//跨线程，原子化，标记是否已经创建好检测线程	

		const std::chrono::steady_clock::time_point ClockEpoch = std::chrono::steady_clock::now();//常量，只读，不需要原子化
		unsigned int CurrentTimeMs{};//线程内，非原子化，单位为毫秒
		std::atomic<unsigned int> Threshold = 200;//跨线程，原子化，单位为毫秒
		KeyState KeysState[256]{};//跨线程，内部有关跨线程的内容已经原子化
	public:
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

namespace YAML {
    template<>
    struct convert<MulNX::KeyCheckPack> {
        static Node encode(const MulNX::KeyCheckPack& KCP) {
            Node node;
            node["usable"] = KCP.Usable;
            node["ctrl"] = KCP.Ctrl;
            node["shift"] = KCP.Shift;
            node["alt"] = KCP.Alt;
            node["vkCode"] = static_cast<int>(KCP.vkCode);
            node["comboClick"] = static_cast<int>(KCP.ComboClick);
            return node;
        }
        static bool decode(const Node& node, MulNX::KeyCheckPack& KCP) {
            if (!node.IsMap()) {
                return false;
            }
            try {
                MulNX::KeyCheckPack temp;
                temp.Usable = node["usable"].as<bool>();
                temp.Ctrl = node["ctrl"].as<bool>();
                temp.Shift = node["shift"].as<bool>();
                temp.Alt = node["alt"].as<bool>();
                temp.vkCode = static_cast<unsigned char>(node["vkCode"].as<unsigned int>());
                temp.ComboClick = static_cast<uint8_t>(node["comboClick"].as<unsigned int>());

                KCP = std::move(temp);
                return true;
            }
            catch (const YAML::Exception&) {
                return false;
            }
        }
    };
}