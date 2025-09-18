cmake_minimum_required(VERSION 3.16)
project(uvc-util VERSION 1.2.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Check if we're on macOS
if(NOT APPLE)
    message(FATAL_ERROR "This project is designed for macOS only due to IOKit dependencies")
endif()

# Find required frameworks
find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

# Add source files
set(SOURCES
    src/UVCType.cpp
    src/UVCValue.cpp
    src/UVCController.cpp
    src/main.cpp
)

set(HEADERS
    src/UVCType.hpp
    src/UVCValue.hpp
    src/UVCController.hpp
)

# Create the executable
add_executable(uvc-util-cpp ${SOURCES} ${HEADERS})

# Set target properties
set_target_properties(uvc-util-cpp PROPERTIES
    OUTPUT_NAME uvc-util
    MACOSX_BUNDLE FALSE
)

# Link frameworks
target_link_libraries(uvc-util-cpp
    ${IOKIT_FRAMEWORK}
    ${COREFOUNDATION_FRAMEWORK}
)

# Include directories
target_include_directories(uvc-util-cpp PRIVATE src)

# Compiler-specific options
target_compile_options(uvc-util-cpp PRIVATE
    -Wno-deprecated-declarations  # IOKit has some deprecated APIs
    -Wno-unused-parameter
)

# Install target
install(TARGETS uvc-util-cpp
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "Building uvc-util for macOS")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "IOKit Framework: ${IOKIT_FRAMEWORK}")
message(STATUS "CoreFoundation Framework: ${COREFOUNDATION_FRAMEWORK}")
