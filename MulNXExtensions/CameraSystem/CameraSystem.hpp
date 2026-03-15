// CameraSystem.hpp
// 摄像机系统
// 工作流抽象：元素->解决方案->项目->工作区
// 每个层次分本体和管理器，生命周期均由管理器负责
// 控制器之间的依赖在初始化阶段进行注入
// 所有对其它层次可能造成影响的操作必须借助该层次处理器执行

// 生命周期描述：
// 先打开指定工作区，此时该工作区的所有项目都会被加载到内存，等待快捷键或点击加载（此时解决方案和元素不加载到内存）
// 当项目被激活时（对应代码中的ActiveProject），该项目的所有解决方案和元素都会从磁盘中被加载，解决方案此时可以被快捷键激活
// 当切换项目时，旧的ActiveProject会自动保存以保留现场
// 当保存工作区时，所有的当前设置会被保存到磁盘
// 新建工作区或新建项目时，所有的文件夹会自动创建

// 关于插值：
// 统一在边界（包括等于）返回false
// Call系列函数返回false应当意味着没有可用Frame产出（保证有值产出时性能）
// 具体的播放启停原理需要其它手段协助（允许启停的卡顿，当然也很微小，但还是要尽最大可能让有值可用时性能最高）

#pragma once

#include "ICameraSystem.hpp"

#include "CameraDrawer/CameraDrawer.hpp"
#include "ElementManager/ElementManager.hpp"
#include "SolutionManager/SolutionManager.hpp"
#include "ProjectManager/ProjectManager.hpp"
#include "WorkspaceManager/WorkspaceManager.hpp"

#include <vector>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <filesystem>

// 摄像机系统
class CameraSystem final :public ICameraSystem {
private:
    // 渲染器
    CameraDrawer CamDrawer{};
    // 四大管理器
    ElementManager EManager{};
    SolutionManager SManager{};
    ProjectManager PManager{};
    WorkspaceManager WManager{};
public:
    // 初始化
    bool Init()override;
    // 菜单
    bool UINodeFunc(MulNXUINode* ThisNode)override;
    // 逻辑主函数
    void VirtualMain()override;
private:
    // 摄像机系统大菜单元素菜单
    void MenuElement();
    // 摄像机系统大菜单解决方案菜单
    void MenuSolution();
    // 摄像机系统大菜单项目菜单
    void MenuProject();
    // 摄像机系统大菜单工作区菜单
    void MenuWorkspace();
public:
    // 清空内存，准备加载新工作区
    void MemoryClear();
public:
    // 接口实现：

    void ResetCameraModule(const float CameraHigh, const float CameraX, const float CameraY, const float AxisLenth, const ImU32 Colour)override;
    void DrawCameraByPAR(const DirectX::XMFLOAT3& Position, const DirectX::XMFLOAT3& Rotation, const char* label)override;

    bool CallProject(const std::string& ProjectName)override;

    bool CallSolution(const std::string& SolutionName)override;
    bool CallSolution(const MulNX::Message& Msg)override;

    bool ShutDown()override;

    bool Save()override;
};