#pragma once

#include <MulNX/MulNX.hpp>

class CSController;
class ConsoleManager final :public MulNX::ModuleBase {
private:
    int TargetTick = 0;
    CSController* pCSController = nullptr;
public:
    bool Init()override;
    void VirtualMain()override;
    bool UINodeFunc(MulNXUINode* node);
};