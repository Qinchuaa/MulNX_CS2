add_library(CameraSystem
    "CameraSystem.cpp"
    "CameraDrawer/CameraDrawer.cpp"
    "CameraSystemIO/CameraSystemIO.cpp"

    "Element/ElementBase/ElementBase.cpp"
    "Element/ElementManager/ElementManager.cpp"
    "Element/ElementManager/ElementDebugger/ElementDebugger.cpp"
    "Element/Elements/FirstPersonCameraPath/FirstPersonCameraPath.cpp"
    "Element/Elements/FreeCameraPath/FreeCameraPath.cpp"
    "Element/Elements/LockedCameraPath/LockedCameraPath.cpp"

    "Solution/Solution/Solution.cpp"
    "Solution/SolutionManager/SolutionManager.cpp"

    "Project/Project/Project.cpp"
    "Project/ProjectManager/ProjectManager.cpp"

    "Workspace/Workspace/Workspace.cpp"
    "Workspace/WorkspaceManager/WorkspaceManager.cpp"
)

target_link_libraries(CameraSystem PUBLIC
    MulNX
)