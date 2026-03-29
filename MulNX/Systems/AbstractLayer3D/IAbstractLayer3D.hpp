#pragma once

#include <MulNX/Base/Math/Math.hpp>
#include <MulNX/Core/ModuleBase/ModuleBase.hpp>

//1到10为玩家，0为本地
class D_Player {
public:
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 EyePosition;
	DirectX::XMFLOAT3 Rotation;
	int HP;
	int Team;
	bool Alive;
	int IndexInEntityList;
	int IndexInMap;
};

class D_GameData {
public:
	D_Player Players[11];

};

namespace MulNX {
    class TimeBridge {
        // 存储游戏原始时间指针
        float* rawTimePointer = nullptr;
        // 模式为0时使用游戏时间，模式为1时使用MulNX时间
        uint8_t timeMode = 0;
        // MulNX时间参考点
        std::chrono::steady_clock::time_point startTime;
        // 当时间模式切换时，记录游戏时间，以便继续模仿，用于子弹时间等
        float bufferTime = 0.0f;
        // 比例，用于控制MulNX时间流速
        float scale = 1.0f;
    public:
        // 切换时间模式，内部维护状态
        void ChangeMode(const uint8_t mode);
        void SetScale(const float scale);
        float GetTime();
    };

    class IAbstractLayer3D :public MulNX::ModuleBase {
    protected:
        D_GameData AL3DGameData{};
    public:
		virtual ~IAbstractLayer3D() = default;

        // 正在废弃
        virtual float GetTime()const = 0;
        // 返回时间源，由实现创建独占指针，这里返回原始指针
        virtual TimeBridge* GetTimeBridge() { return nullptr; };

        virtual MulNX::Math::View GetView()const = 0;

        virtual float* GetViewMatrix() = 0;
        virtual float GetWinWidth()const = 0;
        virtual float GetWinHeight()const = 0;

        virtual bool CameraSystemIOOverride(const CameraSystemIO* const IO) = 0;
		//执行命令
        virtual bool ExecuteCommand(const std::string& command) = 0;

		//信息
        virtual D_Player& GetPlayerMsg(int Index) = 0;

        virtual bool SpecPlayer(int Index) = 0;
        virtual void spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot) = 0;
        virtual void SetDOF(const MulNX::Math::DOFParam& dof) = 0;
        virtual void ClearViewOverride() = 0;
    };
}