cmake_minimum_required(VERSION 3.20)

project(DPNF
    VERSION 0.1.0
    DESCRIPTION "DP Neural Framework (DPNF) - High-Performance Neural Graphics & Deep Learning Framework"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DPNF_BUILD_SANDBOX "Build the DPNF Sandbox application" ON)
option(DPNF_BUILD_TESTS "Build the DPNF GoogleTest suite" ON)
option(DPNF_ENABLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# Compiler warning flags
add_library(dpnf_warnings INTERFACE)
if(MSVC)
    target_compile_options(dpnf_warnings INTERFACE /W4 /permissive-)
    if(DPNF_ENABLE_WARNINGS_AS_ERRORS)
        target_compile_options(dpnf_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(dpnf_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wformat=2
    )
    if(DPNF_ENABLE_WARNINGS_AS_ERRORS)
        target_compile_options(dpnf_warnings INTERFACE -Werror)
    endif()
endif()

# Core Phase 0 Modules
add_subdirectory(Platform)
add_subdirectory(Config)
add_subdirectory(Core)

if(DPNF_BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif()

if(DPNF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
