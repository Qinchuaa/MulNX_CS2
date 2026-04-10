#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class PlayerHub final :public CSModuleBase {
private:
    
public:
    std::vector<CSModuleBase*> ModulesAboutPlayer{};
    
    bool Init()override;
    bool Window(MulNXUINode* node);
};