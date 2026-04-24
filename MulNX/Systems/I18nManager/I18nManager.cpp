#include "I18nManager.hpp"

#include <yaml-cpp/yaml.h>

bool MulNX::I18nManager::Init() {
    auto path = this->ISys().PathGet("Language");
    auto filePath = path / "lan.yaml";

    this->strings.clear();
    YAML::Node root = YAML::LoadFile(filePath.string());
    if (!root.IsMap()) return false;
    for (auto it = root.begin(); it != root.end(); ++it) {
        this->strings[it->first.as<std::string>()] = it->second.as<std::string>();
    }
    
    return true;
}

const std::string& MulNX::I18nManager::Get(const std::string& key) {
    auto it = this->strings.find(key);
    if (it == this->strings.end())return key;
    return it->second;
}