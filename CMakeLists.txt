cmake_minimum_required(VERSION 3.31)

# 项目定义
project("MulNX")

# 启用 LTO（仅 Release 配置）
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)

if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_CONFIG Release)
    set(CMAKE_INTERPROCED URAL_OPTIMIZATION TRUE)
    message(STATUS "IPO/LTO enabled for Release builds")
else()
    message(WARNING "IPO/LTO is not supported by the compiler. Skipping.")
endif()

# C++ 标准设置
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC 专用设置
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
endif()

# 添加子目录
add_subdirectory("MulNXThirdParty")
add_subdirectory("MulNX")
add_subdirectory("MulNXExtensions")
add_subdirectory("CS2OBTool")
add_subdirectory("CS2Injector")

# 配置输出路径
set(InjectedPath ${CMAKE_SOURCE_DIR}/bin/MulNX/MulNX)
set_target_properties(CS2Injector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${InjectedPath}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${InjectedPath}
)

set(DllPath ${InjectedPath}/DLLToCS)
set_target_properties(CS2OBTool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DllPath}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DllPath}
)