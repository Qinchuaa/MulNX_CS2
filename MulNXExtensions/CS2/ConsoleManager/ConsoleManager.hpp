#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ConsoleManager final :public CSModuleBase {
public:
    bool Init()override;
    void VirtualMain()override;
    bool UINodeFunc(MulNXUINode* node);
};