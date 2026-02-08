#pragma once

#include <MulNX/MulNX.hpp>

class MulNXController final :public MulNX::ModuleBase {
    bool Init()override;

    void ProcessMsg(MulNX::Message* Msg)override;
    void VirtualMain()override;
};