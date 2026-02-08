add_library(CS2OBTool SHARED
    "DllMain/DllMain.cpp"
)

target_link_libraries(CS2OBTool PUBLIC
    MulNXExtensions
)