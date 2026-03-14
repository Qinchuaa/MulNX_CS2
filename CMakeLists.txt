cmake_minimum_required(VERSION 3.31)

# 项目定义
project("MulNX_CS2")

# 启用 LTO（仅 Release 配置）
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO?) # IPO 原来要手动启用呀
if(IPO?)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_CONFIG Release)
    message(STATUS "IPO/LTO 在 Release 模式下可用")
else()
    message(WARNING "IPO/LTO 不支持！")
endif()

# C++ 标准设置
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加编译选项
add_compile_options(/MP) # 让MSVC这个编译器明白，现代CPU一般有多个核心！！！
add_compile_options(/utf-8) # 让MSVC这个编译器明白，utf-8已经成为主流！！！
add_compile_options(/EHsc) # 让MSVC这个编译器明白，C++是有异常处理的！！！
add_compile_options(/Zc:__cplusplus) # 让MSVC这个编译器明白，现在的C++版本是c23，不是c98！！！

# 定义全局宏
add_compile_definitions(NOMINMAX) # 不要改我的min max啊！
add_compile_definitions(WIN32_LEAN_AND_MEAN) # 我得把这些幽默的依赖删除！
add_compile_definitions(ASIO_STANDALONE) # 搞半天还得自己配版本实验！

# 以当前目录为唯一的起点，被所有子目录共享！！！
# 除了部分第三方库手调一下，这就是唯一的定义！！！
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 添加子目录
add_subdirectory("MulNXThirdParty")
add_subdirectory("MulNX")
add_subdirectory("MulNXExtensions")
add_subdirectory("CS2OBTool")
add_subdirectory("CS2Injector")

# 定义目标路径
set(InjecterPath ${CMAKE_SOURCE_DIR}/bin/MulNX/MulNX)
set(DllPath ${InjecterPath}/DLLToCS)

# 处理 CS2Injector 目标
set_target_properties(CS2Injector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${InjecterPath}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${InjecterPath}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${InjecterPath}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${InjecterPath}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${InjecterPath}"
)

# 处理 CS2OBTool 目标
set_target_properties(CS2OBTool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${DllPath}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${DllPath}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${DllPath}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${DllPath}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${DllPath}"
)