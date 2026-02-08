#pragma once

#include"Elements/FreeCameraPath/FreeCameraPath.hpp"
#include"Elements/FirstPersonCameraPath/FirstPersonCameraPath.hpp"

//Element模板定义，元素必须继承自ElementBase
template<typename T>
concept Element = std::derived_from<T, ElementBase>;