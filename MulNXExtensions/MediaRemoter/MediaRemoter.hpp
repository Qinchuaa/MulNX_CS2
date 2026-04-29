#pragma once

#include <MulNX/MulNX.hpp>

class MediaRemoter final :public MulNX::ModuleBase {
public:
    bool Init()override;
};