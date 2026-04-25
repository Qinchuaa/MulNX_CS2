#pragma once

#include <MulNX/MulNX.hpp>

DWORD MulNX_CS2_Start(void*);

class MainDraw final:public MulNX::ModuleBase{
public:
    bool Init();
    void Window(MulNX::UINode* node);
};