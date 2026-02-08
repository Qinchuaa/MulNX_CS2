add_library(MiniMap
    "MiniMap.cpp"
)

target_link_libraries(MiniMap PUBLIC
    MulNX
    MulNXCS2Ext
)