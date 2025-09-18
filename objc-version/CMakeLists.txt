cmake_minimum_required(VERSION 3.16)
project(uvc-util-objc
    VERSION 1.2.0
    DESCRIPTION "USB Video Class (UVC) control management utility for Mac OS X - Objective-C Version"
    LANGUAGES C OBJC)

# 设置C和Objective-C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_OBJC_STANDARD 99)
set(CMAKE_OBJC_STANDARD_REQUIRED ON)

# 仅支持macOS平台
if(NOT APPLE)
    message(FATAL_ERROR "This project is only supported on macOS")
endif()

# 设置最低macOS版本
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)

# 编译器标志 - 使用手动内存管理（MRC）
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} -Wall -Wextra -fno-objc-arc")

# 源文件
set(SOURCES
    src/uvc-util.m
    src/UVCController.m
    src/UVCType.m
    src/UVCValue.m
)

# 头文件
set(HEADERS
    src/UVCController.h
    src/UVCType.h
    src/UVCValue.h
)

# 头文件目录
include_directories(src)

# 创建可执行文件
add_executable(uvc-util-objc ${SOURCES} ${HEADERS})

# 查找并链接必需的框架
find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

# 链接框架
target_link_libraries(uvc-util-objc
    ${FOUNDATION_FRAMEWORK}
    ${IOKIT_FRAMEWORK}
    ${COREFOUNDATION_FRAMEWORK}
)

# 设置可执行文件属性
set_target_properties(uvc-util-objc PROPERTIES
    MACOSX_BUNDLE FALSE
    OUTPUT_NAME "uvc-util"
)

# 编译选项
target_compile_options(uvc-util-objc PRIVATE
    -Wno-deprecated-declarations  # 忽略过时API警告
    -Wno-unused-parameter
)

# 安装规则
install(TARGETS uvc-util-objc
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "Building Objective-C version of uvc-util for macOS")
message(STATUS "Deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "Foundation framework: ${FOUNDATION_FRAMEWORK}")
message(STATUS "IOKit framework: ${IOKIT_FRAMEWORK}")
message(STATUS "CoreFoundation framework: ${COREFOUNDATION_FRAMEWORK}")
