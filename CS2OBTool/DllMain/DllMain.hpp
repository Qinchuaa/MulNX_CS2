#pragma once

using DWORD = unsigned long;

DWORD MulNX_CS2_Start(void*);

#include <MulNX/MulNX.hpp>

class MainDraw final:public MulNX::ModuleBase{
public:
    bool Init();
    void Window(MulNX::UINode* node);
};