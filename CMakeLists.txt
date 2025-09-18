cmake_minimum_required(VERSION 3.16)
project(uvc-util-multi
    VERSION 1.2.0
    DESCRIPTION "USB Video Class (UVC) control management utility for Mac OS X"
    LANGUAGES C CXX OBJC)

# 仅支持macOS平台
if(NOT APPLE)
    message(FATAL_ERROR "This project is only supported on macOS")
endif()

# 设置最低macOS版本
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)

# 构建选项
option(BUILD_CPP_VERSION "Build C++ version of uvc-util" ON)
option(BUILD_OBJC_VERSION "Build Objective-C version of uvc-util" ON)
option(BUILD_BOTH_VERSIONS "Build both versions (overrides individual options)" OFF)

# 如果选择构建两个版本，则启用所有选项
if(BUILD_BOTH_VERSIONS)
    set(BUILD_CPP_VERSION ON)
    set(BUILD_OBJC_VERSION ON)
endif()

# 确保至少选择一个版本
if(NOT BUILD_CPP_VERSION AND NOT BUILD_OBJC_VERSION)
    message(FATAL_ERROR "At least one version must be built. Enable BUILD_CPP_VERSION or BUILD_OBJC_VERSION")
endif()

# 构建C++版本
if(BUILD_CPP_VERSION)
    message(STATUS "Configuring C++ version...")
    add_subdirectory(cpp-version)

    # 创建根目录的uvc-util-cpp符号链接
    add_custom_target(cpp_link ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                cpp-version/uvc-util
                uvc-util-cpp
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS uvc-util-cpp
        COMMENT "Creating uvc-util-cpp symlink"
    )
endif()

# 构建Objective-C版本
if(BUILD_OBJC_VERSION)
    message(STATUS "Configuring Objective-C version...")
    add_subdirectory(objc-version)

    # 创建根目录的uvc-util-objc符号链接
    add_custom_target(objc_link ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                objc-version/uvc-util
                uvc-util-objc
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS uvc-util-objc
        COMMENT "Creating uvc-util-objc symlink"
    )
endif()

# 创建默认的uvc-util符号链接
if(BUILD_CPP_VERSION AND BUILD_OBJC_VERSION)
    # 如果两个版本都构建，默认链接到C++版本
    add_custom_target(default_link ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                cpp-version/uvc-util
                uvc-util
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS uvc-util-cpp
        COMMENT "Creating default uvc-util symlink (pointing to C++ version)"
    )
elseif(BUILD_CPP_VERSION)
    # 只构建C++版本
    add_custom_target(default_link ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                cpp-version/uvc-util
                uvc-util
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS uvc-util-cpp
        COMMENT "Creating default uvc-util symlink (pointing to C++ version)"
    )
elseif(BUILD_OBJC_VERSION)
    # 只构建Objective-C版本
    add_custom_target(default_link ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                objc-version/uvc-util
                uvc-util
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS uvc-util-objc
        COMMENT "Creating default uvc-util symlink (pointing to Objective-C version)"
    )
endif()

# 安装规则
if(BUILD_CPP_VERSION)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/cpp-version/uvc-util
            DESTINATION bin
            RENAME uvc-util-cpp)
endif()

if(BUILD_OBJC_VERSION)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/objc-version/uvc-util
            DESTINATION bin
            RENAME uvc-util-objc)
endif()

# 安装默认版本
if(BUILD_CPP_VERSION)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/cpp-version/uvc-util
            DESTINATION bin
            RENAME uvc-util)
elseif(BUILD_OBJC_VERSION)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/objc-version/uvc-util
            DESTINATION bin
            RENAME uvc-util)
endif()

# 打印配置信息
message(STATUS "=== uvc-util Multi-Version Build Configuration ===")
message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "Build C++ version: ${BUILD_CPP_VERSION}")
message(STATUS "Build Objective-C version: ${BUILD_OBJC_VERSION}")

if(BUILD_CPP_VERSION AND BUILD_OBJC_VERSION)
    message(STATUS "Default version: C++ (use 'uvc-util-objc' for Objective-C version)")
elseif(BUILD_CPP_VERSION)
    message(STATUS "Default version: C++")
elseif(BUILD_OBJC_VERSION)
    message(STATUS "Default version: Objective-C")
endif()
message(STATUS "==================================================")
