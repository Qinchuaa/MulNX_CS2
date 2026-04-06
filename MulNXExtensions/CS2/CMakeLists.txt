add_library(MulNXCS2Ext
    "CSController/CSController.cpp"
    "CSController/InterfaceImpl.cpp"
    "CSController/Draw/DrawImpl.cpp"
    "CSController/Client/Client.cpp"
    "CSController/ConVarSystem/ConVarSystem.cpp"
    "CSController/GlobalVars/GlobalVars.cpp"
    "CSController/List/C_BaseEntity.cpp"

    "PlayerHub/PlayerHub.cpp"
    "AdvancedViewController/AdvancedViewController.cpp"
    "PlayerFlashController/PlayerFlashController.cpp"

    "ConsoleManager/ConsoleManager.cpp"
    "DemoHelper/DemoHelper.cpp"
    "GameCfgManager/GameCfgManager.cpp"
    "GameSettingsManager/GameSettingsManager.cpp"
    "HookManager/HookManager.cpp"
)

target_link_libraries(MulNXCS2Ext PUBLIC
    MulNX
    WinExt
)