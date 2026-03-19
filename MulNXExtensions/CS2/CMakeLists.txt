add_library(MulNXCS2Ext
    "CSController/CSController.cpp"
    "CSController/InterfaceImpl.cpp"
    "CSController/ConVarSystem/ConVarSystem.cpp"
    "CSController/CSGameRules/CSGameRules.cpp"
    "CSController/EntityList/EntityList.cpp"
    "CSController/EntityList/Entity/Entity.cpp"
    "CSController/EntityList/Entity/Controller/Controller.cpp"
    "CSController/EntityList/Entity/Pawn/Pawn.cpp"
    "CSController/EntityList/Entity/Pawn/CameraServices/CameraServices.cpp"
    "CSController/EntityList/Entity/Pawn/GameSceneNode/GameSceneNode.cpp"
    "CSController/EntityList/Entity/Pawn/ObserverServices/ObserverServices.cpp"
    "CSController/GlobalVars/GlobalVars.cpp"
    "CSController/LocalPlayer/LocalPlayer.cpp"
    "CSCOntroller/PlantedC4/PlantedC4.cpp"
    "CSCOntroller/List/List.cpp"

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