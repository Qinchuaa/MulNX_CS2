add_library(MulNXCS2Ext
    "CSClasses/CSDll/CSDll.cpp"
    "CSClasses/GlobalVars/GlobalVars.cpp"
    "CSClasses/tree/tree.cpp"

    "PlayerFlashController/PlayerFlashController.cpp"

    "PlayerHub/PlayerHub.cpp"
    "PlayerHub/GlowController/GlowController.cpp"
    "PlayerHub/NameController/NameController.cpp"
    "PlayerHub/SmokeController/SmokeController.cpp"
    "PlayerHub/ObserverController/ObserverController.cpp"
    "PlayerHub/ProjectileTracker/ProjectileTracker.cpp"

    "ConsoleManager/ConsoleManager.cpp"
    "DemoHelper/DemoHelper.cpp"
    "GameCfgManager/GameCfgManager.cpp"
    "GameSettingsManager/GameSettingsManager.cpp"
    "HookManager/HookManager.cpp"

    "CSModuleBase.cpp"
)

add_subdirectory("CSController")

target_link_libraries(MulNXCS2Ext PUBLIC
    MulNX
    WinExt
    CSController
)