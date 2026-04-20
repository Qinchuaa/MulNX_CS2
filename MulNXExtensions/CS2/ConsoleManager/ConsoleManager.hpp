#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ConsoleManager final :public CSModuleBase {
public:
    bool Init()override;
    bool UINodeFunc(MulNX::UINode* node);
};