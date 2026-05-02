# 构建与 UI 重构进度清单

本文档仅保留后续常用入口与待办事项，不再记录已完成条目。

## 直接入口

- 构建脚本：`scripts/build_release.ps1`
- UI 主入口：`CS2OBTool/DllMain/DllMain.cpp`
- 当前文档：`!docs/构建与UI重构进度清单.md`

## 直接执行

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_release.ps1
```

如只想构建、不重新打包：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_release.ps1 -SkipPackage
```

## 当前产物

- DLL：`bin/MulNX/MulNX/DLLToCS/CS2OBTool.dll`
- Injector：`bin/MulNX/MulNX/CS2Injector.exe`
- Zip：`bin/MulNX_release.zip`

## 当前待办

- [ ] 如需继续 UI 优化，在现有主控面板基础上迭代
- [ ] 如需继续稳定性修复，优先排查 `ObserverController` 一类读内存异常
- [ ] 如需再次发布，直接执行 `scripts/build_release.ps1`

## 固定环境

- CMake：`E:\Coding\Cmake\bin\cmake.exe`
- MSVC Root：`E:\Coding\TypeC\VC\Tools\MSVC\14.50.35717`
- Windows SDK：`C:\Program Files (x86)\Windows Kits\10`
