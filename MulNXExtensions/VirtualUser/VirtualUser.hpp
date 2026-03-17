#pragma once

#include <MulNX/MulNX.hpp>

class ICameraSystem;

class VirtualUser final :public MulNX::ModuleBase {
private:
    ICameraSystem* CameraSystem = nullptr;
public:
    VirtualUser() : ModuleBase() {
        //this->Type = MulNX::ModuleType::VirtualUser;
    }

    bool Init()override final;

    void VirtualMain()override final;
    void ProcessMsg(MulNX::Message& Msg)override final;

    void Menu();
};