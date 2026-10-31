add_library(dpnf_platform STATIC
    src/PlatformInfo.cpp
)

target_include_directories(dpnf_platform
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dpnf_platform
    PRIVATE
        dpnf_warnings
        ${CMAKE_DL_LIBS}
)
