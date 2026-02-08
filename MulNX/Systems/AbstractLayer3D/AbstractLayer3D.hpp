#pragma once

#include"IAbstractLayer3D.hpp"

namespace MulNX {
	class AbstractLayer3D final : public IAbstractLayer3D {
	public:
		std::function<bool(const char* command)> CmdInterface = nullptr;
		void SetCmdInterface(std::function<bool(const char* command)> Func)override {
			this->CmdInterface = Func;
		}
		std::function<bool(const CameraSystemIO* const IO)> CameraSystemIOOverrideFunc = nullptr;
		void SetCameraSystemIOOverrideFunc(std::function<bool(const CameraSystemIO* const IO)> Func)override {
			this->CameraSystemIOOverrideFunc = Func;
		}
		std::function<MulNX::Base::Math::SpatialState()> GetSpatialStateFunc = nullptr;
		void SetGetSpatialStateFunc(std::function<MulNX::Base::Math::SpatialState()> Func)override {
			this->GetSpatialStateFunc = Func;
		}
		std::function<float* ()> GetViewMatrixFunc = nullptr;
		void SetGetViewMatrixFunc(std::function<float* ()> Func)override {
			this->GetViewMatrixFunc = Func;
		}
	private:
		std::atomic<float> CurrentTime = 0.0f;

		//阶段时间基准
		float PhaseStartTime = 0.0f;
		//阶段时长
		float PhaseDuration = 0.0f;

		std::atomic<uintptr_t> pCurrentTime = 0;

		//当前屏幕宽高，默认1080P
		int CurrentWindowWidth = 1920;
		int CurrentWindowHeight = 1080;
	public:
		AbstractLayer3D() : IAbstractLayer3D() {
			//this->Type = MulNX::ModuleType::AbstractLayer3D;
		}
		bool Init()override;
		void VirtualMain()override;
		void ProcessMsg(MulNX::Message* Msg)override;

		void UpdateTime();
		//接口实现
		void SetCurrentTimePointer(uintptr_t pCurrentTime)override;
		uintptr_t GetCurrentTimePointer()override;
		float GetTime()const override;

		//获取阶段已用时间（正计时）
		float GetPhaseElapsedTime()const override;
		//获取阶段剩余时间（倒计时）
		float GetPhaseRemainingTime()const override;
		//获取阶段总时长
		float GetPhaseDuration()const override;
		//设置阶段开始时间基准
		void SetPhaseStartTime(float startTime)override;
		//设置阶段总时长
		void SetPhaseDuration(float duration)override;

		MulNX::Base::Math::SpatialState GetSpatialState()const override;
		float* GetViewMatrix()const override;
		float GetWinWidth()const override;
		float GetWinHeight()const override;
		bool CameraSystemIOOverride(const CameraSystemIO* const IO)override;
		bool ExecuteCommand(const char* command)override;
		bool ExecuteCommand(const std::string& command)override;
		bool UpdatePlayerMsg(D_Player&& PlayMsg)override;
		bool SpecPlayer(int IndexInMap)override;
		D_Player& GetPlayerMsg(int Index)override;
	};
}