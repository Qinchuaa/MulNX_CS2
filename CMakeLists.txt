cmake_minimum_required(VERSION 3.20)

# 项目定义
project("MulNX")

# C++ 标准设置
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC 专用设置
if(MSVC)
    # 统一设置源文件字符集和执行字符集为 UTF-8
    add_compile_options(/utf-8)

    # 防止 windows.h 定义 min/max 宏
    add_compile_definitions(NOMINMAX)
    # 定义 WIN32_LEAN_AND_MEAN 以减少 windows.h 包含的内容
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