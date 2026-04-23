#include "I18nManager.hpp"

bool MulNX::I18nManager::Init() {
    this->strings["log.success"] = "测试成功";
    return true;
}

const std::string& MulNX::I18nManager::Get(const std::string& key) {
    auto it = this->strings.find(key);
    if (it == this->strings.end())return key;
    return it->second;
}