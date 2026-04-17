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
        // 指向抽象层的指针，用于调用抽象层的时间函数
        IAbstractLayer3D* pAL3D;
        //是否在虚拟时间轴播放（偏移时间轴播放）
        bool virtualTimePlaying = false;
        // MulNX时间参考点
        std::chrono::steady_clock::time_point startTime;
        // 用于计算虚拟时间的缓冲变量
        float refreshTime = 0.0f;
        // 比例，用于控制虚拟时间的流速，默认为1.0f（与真实时间相同）
        float scale = 1.0f;
        // 上一次获取的真实时间，用于检测时间回跳等异常情况
        float lastRealTime = 0.0f;
        // 内部更新函数
        void update();
    public:
        TimeBridge() = delete;
        TimeBridge(IAbstractLayer3D* pAL3D);

        bool RefreshVirtual(bool virtualTimePlaying, float scale);
        float GetReal();
        bool JumpReal(float time);
        bool JumpRealRel(float time);
        float GetVirtual();
        float Get();
    };

    class IAbstractLayer3D :public MulNX::ModuleBase {
        friend TimeBridge;
    private:
        TimeBridge timeBridge{ this };
        virtual float GetTime() = 0;
        virtual bool JumpTime(const float time) = 0;
    protected:
        D_GameData AL3DGameData{};
    public:
		virtual ~IAbstractLayer3D() = default;
        // 返回时间源，由实现创建独占指针，这里返回原始指针
        TimeBridge* Time();

        virtual MulNX::Math::View GetView() = 0;

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