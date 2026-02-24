add_library(WinExt STATIC
    "WIN32Msg/WIN32Msg.cpp"
    "WinException/WinException.cpp"
    "HookUtility/HookUtility.cpp"

    "Memory/Assembler/Assembler.cpp"
    "Memory/DllModule/DllModule.cpp"
    "Memory/Pattern/Pattern.cpp"
    "Memory/Region/Region.cpp"
    "Memory/Region/ProtectionGuard.cpp"
    "Memory/Memory.cpp"
)

target_link_libraries(WinExt PUBLIC
    MulNXThirdParty
)