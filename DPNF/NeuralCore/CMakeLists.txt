add_library(dpnf_neural STATIC
    src/Module.cpp
)

target_include_directories(dpnf_neural
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dpnf_neural
    PUBLIC
        dpnf_autodiff
    PRIVATE
        dpnf_warnings
)
