#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

class GlowController final :public CSModuleBase {
    std::unique_ptr<MulNX::Memory::HookEx> hkSetGlowColor = nullptr;
    std::unordered_map<Steam64UID, uint32_t>playerColors;
    std::map<CS2::ui8TeamNum, uint32_t>teamColors;
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    void MySetGlowColor(CS2::CGlowProperty* pGlowProperty, uint32_t* color);
    void Menu(MulNXUINode* node);
    void MenuPlayer(MulNXUINode* node);
    void MenuTeam(MulNXUINode* node);
};