#pragma once

#include <MulNX/MulNX.hpp>


class C_GameSceneNode {
public:
    std::ostringstream GetMsg()const;

    uintptr_t Address{};
    DirectX::XMFLOAT3 Position{};
    DirectX::XMFLOAT3 RotationEuler{};
};