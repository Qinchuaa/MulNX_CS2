add_library(MulNX
    "Base/any_smart_ptr/any_smart_ptr.hpp"
    "Base/CharUtility/CharUtility.cpp"
    "Base/Math/Math.cpp"
    "Base/TripleBuffer/TripleBuffer.cpp"
    "Base/MulNXHandle/MulNXHandle.cpp"
    "Base/Base.cpp"

    "Config/Config.cpp"

    "Core/Core.cpp"
    "Core/CoreStarterBase/CoreStarterBase.cpp"
    "Core/ModuleBase/ModuleBase.cpp"
    "Core/ModuleBase/ISys/ISys.cpp"
    "Core/ModuleManager/ModuleManager.cpp"
    "Core/Cores.hpp"


    "Systems/AbstractLayer3D/IAbstractLayer3D.cpp"
    "Systems/Debugger/Debugger.cpp"
    "Systems/HandleSystem/HandleSystem.cpp"
    "Systems/IPCer/IPCer.cpp"
    "Systems/KeyTracker/KeyTracker.cpp"
    "Systems/MessageManager/MulNXMessage/MulNXMessage.cpp"
    "Systems/MessageManager/MessageChannel/MessageChannel.cpp"
    "Systems/MessageManager/MessageManager.cpp"
    "Systems/MulNXGlobalVars/MulNXGlobalVars.cpp"
    "Systems/MulNXUISystem/MulNXUISystem.cpp"
    "Systems/MulNXUISystem/MulNXUIContext/MulNXUIContext.cpp"
    "Systems/MulNXUISystem/MulNXUIContext/MulNXUINode/MulNXUINode.cpp"
    "Systems/PathManager/PathManager.cpp"
    "Systems/PathManager/FilePathNode/FilePathNode.cpp"

    "MulNX.cpp"
)
target_link_libraries(MulNX PUBLIC
    MulNXThirdParty
 )