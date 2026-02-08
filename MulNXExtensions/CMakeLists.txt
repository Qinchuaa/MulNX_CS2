add_library(MulNXExtensions
    "Win32Starter/Win32Starter.cpp"
)

add_subdirectory("CS2")
add_subdirectory("WinExt")
add_subdirectory("CameraSystem")
add_subdirectory("MiniMap")
add_subdirectory("VirtualUser")

target_link_libraries(MulNXExtensions PUBLIC
    MulNX
    WinExt
    MulNXCS2Ext
    CameraSystem
    MiniMap
    VirtualUser
)