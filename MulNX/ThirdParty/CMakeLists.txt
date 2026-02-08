add_library(ThirdParty
    "imgui_d11/imgui.cpp"
    "imgui_d11/imgui_draw.cpp"
    "imgui_d11/imgui_impl_dx11.cpp"
    "imgui_d11/imgui_impl_win32.cpp"
    "imgui_d11/imgui_stdlib.cpp"
    "imgui_d11/imgui_tables.cpp"
    "imgui_d11/imgui_widgets.cpp"

    "pugixml/pugixml.cpp"
)

target_link_libraries(ThirdParty PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/minhook_debug_x64/lib/minhook.x64d.lib"
)

target_include_directories(ThirdParty PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)