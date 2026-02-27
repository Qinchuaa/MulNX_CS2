#pragma once

#include "IAbstractLayer3D.hpp"

namespace MulNX {
	class AbstractLayer3D : public IAbstractLayer3D {
    private:
        
        std::atomic<float> AL3DCurrentTime = 0.0f;

		//阶段时间基准
		float AL3DPhaseStartTime = 0.0f;
		//阶段时长
		float AL3DPhaseDuration = 0.0f;

		//当前屏幕宽高，默认1080P
		int AL3DCurrentWindowWidth = 1920;
        int AL3DCurrentWindowHeight = 1080;
    protected:
        D_GameData AL3DGameData{};
    public:

		float GetWinWidth()const override;
		float GetWinHeight()const override;
		bool SpecPlayer(int IndexInMap)override;
		D_Player& GetPlayerMsg(int Index)override;
	};
}