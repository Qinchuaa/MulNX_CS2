add_library(MulNXThirdParty
    "imgui_d11/imgui.cpp"
    "imgui_d11/imgui_draw.cpp"
    "imgui_d11/imgui_impl_dx11.cpp"
    "imgui_d11/imgui_impl_win32.cpp"
    "imgui_d11/imgui_stdlib.cpp"
    "imgui_d11/imgui_tables.cpp"
    "imgui_d11/imgui_widgets.cpp"
    "pugixml/pugixml.cpp"
    "minhook/src/buffer.c"
    "minhook/src/hook.c"
    "minhook/src/trampoline.c"
    "minhook/src/hde/hde32.c"
    "minhook/src/hde/hde64.c"
)

target_include_directories(MulNXThirdParty PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/websocketpp
)