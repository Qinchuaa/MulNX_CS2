#pragma once

#include <MulNX/MulNX.hpp>

class MulNXController final :public MulNX::ModuleBase {
    bool Init()override;
    bool UINodeFunc(MulNXUINode* ThisNode)override;
    void ProcessMsg(MulNX::Message* Msg)override;
    void VirtualMain()override;
};