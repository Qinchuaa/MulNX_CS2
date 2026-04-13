#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <yaml-cpp/yaml.h>

#include <thread>
#include <chrono>
#include <DirectXMath.h>

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
		std::atomic<bool> Current{};//跨线程，原子化
		bool Previous{};//线程内，非原子化
		std::atomic<unsigned char> ComboClick{};//跨线程，原子化 不认为连击能超过127
		std::atomic<unsigned char> BufferComboClick{};//跨线程，原子化 同上
		unsigned int LastPressTimeMs{};//线程内，非原子化，单位为毫秒
	};

	// 自由摄像机位置控制器（仅位置控制）
	class FreeCameraController {
	public:
		DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f }; // 位置
		DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f }; // 旋转角度 (pitch, yaw, roll)
		std::atomic<float> MoveSpeed = 100.0f; // 移动速度 (单位/秒)

		void Update(InputSystem* inputSystem);
	};

    class InputSystem final :public ModuleBase {
        friend class FreeCameraController;
    private:
		std::atomic<bool> IfCreated{};//跨线程，原子化，标记是否已经创建好检测线程	

		const std::chrono::steady_clock::time_point ClockEpoch = std::chrono::steady_clock::now();//常量，只读，不需要原子化
		unsigned int CurrentTimeMs{};//线程内，非原子化，单位为毫秒
		std::atomic<unsigned int> Threshold = 200;//跨线程，原子化，单位为毫秒
		KeyState KeysState[256]{};//跨线程，内部有关跨线程的内容已经原子化

		FreeCameraController FreeCamera; // 自由摄像机控制器
		float LastUpdateTime = 0.0f; // 上次更新时间
	public:
		bool Init()override;

		bool UpdateKeysState();//更新
		bool IsKeyPressed(const unsigned char vkCode)const;//读取是否被按下
		bool CheckComboClick(const unsigned char vkCode, const unsigned char TargetCombo);//读取并移除连击缓冲
		bool CheckWithPack(const KeyCheckPack& Pack);//读取并移除连击缓冲
		unsigned char CheckComboClickUnremove(const unsigned char vkCode)const;//读取连击（非缓冲）
		void ResetThreshold(const unsigned int Threshold);//重新设置阈值

		// 自由摄像机相关方法
		FreeCameraController& GetFreeCamera() { return FreeCamera; }
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