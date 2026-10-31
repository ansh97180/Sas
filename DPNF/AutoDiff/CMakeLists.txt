add_library(dpnf_autodiff STATIC
    src/Variable.cpp
)

target_include_directories(dpnf_autodiff
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dpnf_autodiff
    PUBLIC
        dpnf_tensor
    PRIVATE
        dpnf_warnings
)
