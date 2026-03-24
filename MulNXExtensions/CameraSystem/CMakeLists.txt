add_library(CameraSystem
    "CameraSystem.cpp"
    "CameraDrawer/CameraDrawer.cpp"
    "CameraSystemIO/CameraSystemIO.cpp"

    "Elements/ElementBase.cpp"
    "ElementManager/ElementManager.cpp"
    "Elements/FirstPersonCameraPath/FirstPersonCameraPath.cpp"
    "Elements/FreeCameraPath/FreeCameraPath.cpp"
    "Elements/LockedCameraPath/LockedCameraPath.cpp"

    "Solution/Solution.cpp"
    "SolutionManager/SolutionManager.cpp"

    "Project/Project.cpp"
    "ProjectManager/ProjectManager.cpp"

    "Workspace/Workspace.cpp"
    "WorkspaceManager/WorkspaceManager.cpp"
)

target_link_libraries(CameraSystem PUBLIC
    MulNX
)