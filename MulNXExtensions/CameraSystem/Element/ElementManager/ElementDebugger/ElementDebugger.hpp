#pragma once

#include "../../Elements.hpp"

#include <MulNX/MulNX.hpp>

class ElementManager;
class IAbstractLayer3D;
class CameraDrawer;

class ElementDebugger final :public MulNX::ModuleBase {
    friend ElementManager;
private:
    CameraDrawer* CamDrawer = nullptr;
    ElementManager* EManager = nullptr;

    //初始化
    bool Init()override;
    // 依赖注入
    void InjectDependence(CameraDrawer* CamDrawer, ElementManager* EManager);


    //调试菜单入口点
    void DebugMenus(ElementBase* const pElement);

    //自由摄像机轨道调试菜单
    void DebugMenu_FreeCameraPath(FreeCameraPath* const FreeCamPath);
    //第一人称视角轨道调试菜单
    void DebugMenu_FirstPersonCameraPath(FirstPersonCameraPath* const FirstPersonCameraPath);
};