add_library(dpnf_config STATIC
    src/FrameworkConfig.cpp
)

target_include_directories(dpnf_config
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dpnf_config
    PRIVATE
        dpnf_warnings
)
