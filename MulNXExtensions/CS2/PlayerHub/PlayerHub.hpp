#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/CSController/List/C_BaseEntity.hpp>

class CSController;
class PlayerHub final :public MulNX::ModuleBase {
private:
    CSController* CS = nullptr;
    // 用于存储玩家名字的覆盖数据，索引对应玩家索引（1-10）
    std::array<std::atomic<std::shared_ptr<std::string>>, 10> overridePlayerNames{};
public:
    bool Init()override;
    void ThreadMain()override;
    bool Window(MulNXUINode* node);
};