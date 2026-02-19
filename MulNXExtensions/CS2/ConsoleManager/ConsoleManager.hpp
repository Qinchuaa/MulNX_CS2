#pragma once

#include <MulNX/MulNX.hpp>

class ConsoleManager final :public MulNX::ModuleBase {
private:
    int TargetTick = 0;
public:
    ConsoleManager() : ModuleBase() {
        //this->Type = MulNX::ModuleType::ConsoleManager;
    }

    bool Init()override;
    void VirtualMain()override;
    bool UINodeFunc(MulNXUINode* ThisNode)override;
};