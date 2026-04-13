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
public:
    bool NeedRefresh = false;
    SolutionConfig Config{};
private:
    ////////////////////////////////////////
    //当前操作的解决方案指针
    Solution* CurrentSolution = nullptr;
    ////////////////////////////////////////
    //操作调试窗口
private:
    //是否打开解决方案按键绑定调试窗口
    std::atomic<bool> OpenSolutionKCPackDebugWindow = false;
    //是否打开解决方案名称修改调试窗口
    std::atomic<bool> OpenSolutionNameDebugWindow = false;
    ////////////////////////////////////////
public:

    //数据存储
    std::vector<std::unique_ptr<Solution>> Solutions{};

    ////////////////////////////////////////
    //播放

    // 当前播放的解决方案
    Solution* Playing_pSolution = nullptr;
    // 是否处于播放状态
    // 播放完成之后需要变为false，切换解决方案要变成true
    bool Playing = false;

    ////////////////////////////////////////

    // 解决方案管理器基本函数
    // 初始化函数
    bool Init()override;
    // UI绘制
    bool UINodeFunc(MulNXUINode* node);
    // 逻辑主函数
    void VirtualMain();
    // 遍历，用于迭代处理每个解决方案
    void Traversal();
    // 刷新，更深层次的遍历，主要用于清理失效元素
    void Refresh();



    //创建解决方案
    bool Solution_Create(const std::string& Name);
    //保存所有解决方案到文件
    bool Solution_SaveAll();
    //从文件加载解决方案（反序列化，序列化在Solution）
    bool Solution_Load(const std::filesystem::path& FullPath);
    //获取解决方案对应的迭代器
    std::vector<std::unique_ptr<Solution>>::iterator Solution_GetIterator(const std::string& Name);
    //获取解决方案指针
    Solution* Solution_Get(const std::string& Name);
    //删除解决方案
    bool Solution_Delete(Solution* Solution);
    bool Solution_Delete(const std::string& Name);
    //删除所有解决方案
    bool Solution_ClearAll();
    //获取所有解决方案名称容器（危险函数，只有摄像机系统内部可用）
    const std::vector<std::string> Solution_GetNames()const;
    //展示单个解决方案信息在一行上
    void Solution_ShowInLine(Solution* solution);
    //按行展示所有解决方案
    void Solution_ShowAllInLines();
private:
    //解决方案调试窗口及菜单
    void Solution_DebugWindow();
    //按键调试缓存
    MulNX::KeyCheckPack Buffer_KCPack{};
    //名称修改缓存
    std::string Buffer_Name{};
    //名称修改窗口
    void Solution_Name_DebugWindow();

    //预览功能相关
public:
    //开启播放
    void Playing_Enable();
    //关闭播放
    void Playing_Disable();
    //通过指针设置当前播放的解决方案
    bool Playing_SetSolution(Solution* const solution);
    //通过名称设置当前播放的解决方案
    bool Playing_SetSolution(const std::string& SolutionName);


    //设置播放时间偏移
    void Playing_SetTimeSchema(const float Time);
    //调用播放
    void Playing_Call();
};