#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

class GlowController final :public CSModuleBase {
    std::unique_ptr<MulNX::Memory::HookEx> hkSetGlowColor = nullptr;
    std::unordered_map<Steam64UID, uint32_t>playerColors;
    std::map<TeamNum, uint32_t>teamColors;
public:
    bool Init()override;
    void MySetGlowColor(CS2::CGlowProperty* pGlowProperty, uint32_t* color);
    void CheckMenu(Steam64UID uid);
    void SetMenu(Steam64UID uid);
};