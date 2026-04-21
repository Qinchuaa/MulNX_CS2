#pragma once

#include "SolutionConfig.hpp"
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CameraSystem/Solution/Solution.hpp>

class CameraDrawer;

class ElementManager;
class ProjectManager;

//解决方案管理器，用于管理解决方案
class SolutionManager final :public MulNX::ModuleBase {
private:
    CameraDrawer* CamDrawer = nullptr;
    ElementManager* EManager = nullptr;
    ProjectManager* PManager = nullptr;

    //当前操作的解决方案指针
    Solution* CurrentSolution = nullptr;
    //按键调试缓存
    MulNX::KeyCheckPack Buffer_KCPack{};
    //是否打开解决方案按键绑定调试窗口
    std::atomic<bool> OpenSolutionKCPackDebugWindow = false;
    // 当前播放的解决方案
    Solution* Playing_pSolution = nullptr;
    // 是否处于播放状态
    // 播放完成之后需要变为false，切换解决方案要变成true
    bool Playing = false;
public:
    bool NeedRefresh = false;
    SolutionConfig Config{};
    
    //数据存储
    std::unordered_map<std::string, std::unique_ptr<Solution>> solutions{};
    bool MenuSolution(MulNX::UINode* node);
    bool UINodeFunc(MulNX::UINode* node);
    void Solution_ShowInLine(Solution* solution);
    void Solution_DebugWindow();

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;

    void HandleUpdate();
    void Traversal();

    bool Solution_Create(const std::string& Name);
    //保存所有解决方案到文件
    bool Solution_SaveAll();
    //从文件加载解决方案（反序列化，序列化在Solution）
    bool Solution_Load(const std::filesystem::path& FullPath);
    //删除解决方案
    bool Solution_Delete(const std::string& Name);
    //删除所有解决方案
    bool Solution_ClearAll();

    //预览功能相关

    //开启播放
    void Playing_Enable();
    //关闭播放
    void Playing_Disable();
    //通过名称设置当前播放的解决方案
    bool Playing_SetSolution(const std::string& SolutionName);
    //设置播放时间偏移
    void Playing_SetTimeSchema(const float Time);
    //调用播放
    void Playing_Call();
};