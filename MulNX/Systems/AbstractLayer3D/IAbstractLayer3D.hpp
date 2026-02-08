#pragma once

#include"../../Core/ModuleBase/ModuleBase.hpp"

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
	class IAbstractLayer3D :public MulNX::ModuleBase {
	protected:
		std::shared_mutex Mtx;
		D_GameData GameData{};
	public:
		std::shared_mutex& GetMtx() {
			return this->Mtx;
		}
		virtual void SetCmdInterface(std::function<bool(const char* command)> Func) = 0;
		virtual void SetCameraSystemIOOverrideFunc(std::function<bool(const CameraSystemIO* const IO)> Func) = 0;
		virtual void SetGetSpatialStateFunc(std::function<MulNX::Base::Math::SpatialState()> Func) = 0;
		virtual void SetGetViewMatrixFunc(std::function<float* ()> Func) = 0;
		IAbstractLayer3D() :ModuleBase() {
			//this->Type = MulNX::ModuleType::AbstractLayer3D;
		}
		virtual ~IAbstractLayer3D() = default;

		virtual void SetCurrentTimePointer(uintptr_t pCurrentTime) = 0;
		virtual uintptr_t GetCurrentTimePointer() = 0;
		virtual float GetTime()const = 0;

		//获取阶段已用时间（正计时）
		virtual float GetPhaseElapsedTime()const = 0;
		//获取阶段剩余时间（倒计时）
		virtual float GetPhaseRemainingTime()const = 0;
		//获取阶段总时长
		virtual float GetPhaseDuration()const = 0;
		//设置阶段开始时间基准
		virtual void SetPhaseStartTime(float startTime) = 0;
		//设置阶段总时长
		virtual void SetPhaseDuration(float duration) = 0;

		virtual MulNX::Base::Math::SpatialState GetSpatialState()const = 0;

		virtual float* GetViewMatrix()const = 0;
		virtual float GetWinWidth()const = 0;
		virtual float GetWinHeight()const = 0;


		virtual bool CameraSystemIOOverride(const CameraSystemIO* const IO) = 0;
		//执行命令
		virtual bool ExecuteCommand(const char* command) = 0;
		virtual bool ExecuteCommand(const std::string& command) = 0;


		//信息
		virtual bool UpdatePlayerMsg(D_Player&& PlayMsg) = 0;
		virtual D_Player& GetPlayerMsg(int Index) = 0;

		virtual bool SpecPlayer(int Index) = 0;
	};
}