// ICameraSystem.hpp
// 任何使用摄像机系统的组件应当包含该头文件

#pragma once

#include <MulNX/MulNX.hpp>

typedef unsigned int ImU32;

class ICameraSystem :public MulNX::ModuleBase {
public:
    ICameraSystem() : ModuleBase() {
        //this->Type = MulNX::ModuleType::CameraSystem;
    }

    virtual void ResetCameraModule(const float CameraHigh, const float CameraX, const float CameraY, const float AxisLenth, const ImU32 Colour) = 0;
    virtual void DrawCameraByPAR(const DirectX::XMFLOAT3& Position, const DirectX::XMFLOAT3& Rotation, const char* label) = 0;

    virtual bool CallProject(const std::string& ProjectName) = 0;

    virtual bool CallSolution(const std::string& SolutionName) = 0;
    virtual bool CallSolution(const MulNX::Message& Msg) = 0;

    virtual bool ShutDown() = 0;

    virtual bool Save() = 0;
};