#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

class GlowController final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook> hkSetGlowColor = nullptr;
    std::unordered_map<Steam64UID, uint32_t>playerColors;
    std::map<CS2::ui8TeamNum, uint32_t>teamColors;
public:
    bool Init()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    void MySetGlowColor(CS2::CGlowProperty* pGlowProperty, uint32_t* color);
    void Menu(MulNX::UINode* node);
    void MenuPlayer(MulNX::UINode* node);
    void MenuTeam(MulNX::UINode* node);
};