add_library(dpnf_memory STATIC
    src/MemoryPool.cpp
)

target_include_directories(dpnf_memory
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dpnf_memory
    PUBLIC
        dpnf_core
    PRIVATE
        dpnf_warnings
)
