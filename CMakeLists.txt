cmake_minimum_required(VERSION 3.20)

# 设置编译器编码选项
if(MSVC)
    # 源文件编码
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

    # 执行时编码
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/execution-charset:utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/execution-charset:utf-8>")

    # 源字符集
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/source-charset:utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project("MulNX")

add_subdirectory("MulNXThirdParty")
add_subdirectory("MulNX")
add_subdirectory("MulNXExtensions")
add_subdirectory("CS2OBTool")
add_subdirectory("CS2Injector")

# 设置注入器路径
set(InjectedPath ${CMAKE_SOURCE_DIR}/bin/MulNX/MulNX)
set_target_properties(CS2Injector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${InjectedPath}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${InjectedPath}
)

# 设置DLL路径
set(DllPath ${InjectedPath}/DLLToCS)
set_target_properties(CS2OBTool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DllPath}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DllPath}
)