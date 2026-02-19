add_library(WinExt STATIC
    "WIN32Msg/WIN32Msg.cpp"
    "WinException/WinException.cpp"
    "HookUtility/HookUtility.cpp"
    "Memory/Memory.cpp"
)

target_link_libraries(WinExt PUBLIC
    MulNXThirdParty
)