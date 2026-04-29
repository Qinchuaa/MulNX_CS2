add_library(MulNXExtensions INTERFACE
)

add_subdirectory("CS2")
add_subdirectory("WinExt")
add_subdirectory("CameraSystem")
add_subdirectory("MiniMap")
add_subdirectory("VirtualUser")
add_subdirectory("MulNXController")
add_subdirectory("WebSocketManager")
add_subdirectory("MediaRemoter")

target_link_libraries(MulNXExtensions INTERFACE
    MulNX
    WinExt
    MulNXCS2Ext
    CameraSystem
    MiniMap
    VirtualUser
    MulNXController
    WebSocketManager
    MediaRemoter
)