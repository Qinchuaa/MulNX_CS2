if(WIN32)
    # 设置代码页为UTF-8
    execute_process(COMMAND chcp 65001 OUTPUT_QUIET ERROR_QUIET)

    # 设置环境变量
    set(ENV{CMakeForceUTF8} "ON")
    set(ENV{PYTHONIOENCODING} "utf-8")

    # 确保CMake使用UTF-8
    if(POLICY CMP0142)
        cmake_policy(SET CMP0142 NEW)
    endif()
endif()

add_executable(CS2Injector WIN32
    "Include/Charsets/Charsets.cpp"
    "Main/Main.cpp"
    "MulNXeCore/MulNXeCore.cpp"
    "MulNXeCore/ConfigManager/ConfigManager.cpp"
    "MulNXeCore/IPCer/IPCer.cpp"

    "ThirdParty/ImGui/imgui.cpp"
    "ThirdParty/ImGui/imgui_draw.cpp"
    "ThirdParty/ImGui/imgui_impl_dx11.cpp"
    "ThirdParty/ImGui/imgui_impl_win32.cpp"
    "ThirdParty/ImGui/imgui_tables.cpp"
    "ThirdParty/ImGui/imgui_widgets.cpp"

    "ThirdParty/pugixml/pugixml.cpp"
)

target_link_libraries(CS2Injector PUBLIC
    MulNXExtensions
)

add_dependencies(CS2Injector CS2OBTool)