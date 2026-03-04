#pragma once

#include "../../Core/ModuleBase/ModuleBase.hpp"

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
        // 当前屏幕宽高，默认1080P
        
        int AL3DCurrentWindowWidth = 1920;
        int AL3DCurrentWindowHeight = 1080;
        D_GameData AL3DGameData{};
    public:
		virtual ~IAbstractLayer3D() = default;

        virtual float GetTime()const = 0;

        virtual MulNX::Base::Math::SpatialState GetSpatialState()const = 0;

        virtual float* GetViewMatrix()const = 0;
        virtual float GetWinWidth()const = 0;
        virtual float GetWinHeight()const = 0;

        virtual bool CameraSystemIOOverride(const CameraSystemIO* const IO) = 0;
		//执行命令
        virtual bool ExecuteCommand(const std::string& command) = 0;

		//信息
        virtual D_Player& GetPlayerMsg(int Index) = 0;

        virtual bool SpecPlayer(int Index) = 0;
	};
}