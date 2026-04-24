#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>

namespace YAML {
    class Node;
}

namespace MulNX {
    class I18nManager final :public MulNX::ModuleBase {
    private:
        std::unordered_map<std::string, std::string>strings{};
    public:
        bool Init()override;
        const std::string& Get(const std::string& key);
    };
}