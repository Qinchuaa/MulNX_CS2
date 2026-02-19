add_library(MulNXThirdParty
    "MulNXThirdParty/imgui_d11/imgui.cpp"
    "MulNXThirdParty/imgui_d11/imgui_draw.cpp"
    "MulNXThirdParty/imgui_d11/imgui_impl_dx11.cpp"
    "MulNXThirdParty/imgui_d11/imgui_impl_win32.cpp"
    "MulNXThirdParty/imgui_d11/imgui_stdlib.cpp"
    "MulNXThirdParty/imgui_d11/imgui_tables.cpp"
    "MulNXThirdParty/imgui_d11/imgui_widgets.cpp"

    "MulNXThirdParty/pugixml/pugixml.cpp"
)

target_link_libraries(MulNXThirdParty PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/MulNXThirdParty/minhook_debug_x64/lib/minhook.x64d.lib"
)

target_include_directories(MulNXThirdParty PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)