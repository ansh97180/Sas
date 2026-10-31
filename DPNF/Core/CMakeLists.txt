find_package(Threads REQUIRED)

add_library(dpnf_core STATIC
    src/Error.cpp
    src/Logger.cpp
    src/ResourceTracker.cpp
    src/ThreadPool.cpp
    src/CoreSystem.cpp
)

target_include_directories(dpnf_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dpnf_core
    PUBLIC
        dpnf_platform
        dpnf_config
        Threads::Threads
    PRIVATE
        dpnf_warnings
)
