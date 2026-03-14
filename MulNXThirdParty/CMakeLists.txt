add_library(MulNXThirdParty
    "MulNXThirdParty/imgui_d11/imgui.cpp"
    "MulNXThirdParty/imgui_d11/imgui_draw.cpp"
    "MulNXThirdParty/imgui_d11/imgui_impl_dx11.cpp"
    "MulNXThirdParty/imgui_d11/imgui_impl_win32.cpp"
    "MulNXThirdParty/imgui_d11/imgui_stdlib.cpp"
    "MulNXThirdParty/imgui_d11/imgui_tables.cpp"
    "MulNXThirdParty/imgui_d11/imgui_widgets.cpp"

    "MulNXThirdParty/pugixml/pugixml.cpp"
    
    "MulNXThirdParty/minhook/src/buffer.c"
    "MulNXThirdParty/minhook/src/hook.c"
    "MulNXThirdParty/minhook/src/trampoline.c"
    "MulNXThirdParty/minhook/src/hde/hde32.c"
    "MulNXThirdParty/minhook/src/hde/hde64.c"
)

target_include_directories(MulNXThirdParty PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/MulNXThirdParty
)