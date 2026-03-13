#pragma once

#include "../EntityList/EntityList.hpp"

class C_LocalPlayer {
public:
	mutable std::shared_mutex LocalPlayerMutex;

	C_Entity Entity{};
	DirectX::XMFLOAT3* PositionA{};
	DirectX::XMFLOAT3* PositionB{};
	DirectX::XMFLOAT3* ViewAngles{};
	float* ViewMatrix;
	std::atomic<float>* pGlobalFOV = nullptr;

	// 获取综合信息
	std::ostringstream GetMsg();

	// 读取
	MulNX::Math::View GetView()const;
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetRotationEuler();
	float GetFov();
	float* GetMatrix();

	// 设置
	void SetViewAngle(const DirectX::XMFLOAT3& Angles);
	bool SetPosition(const DirectX::XMFLOAT4& PosAndFOV);

	// 更新
	int Update();
};