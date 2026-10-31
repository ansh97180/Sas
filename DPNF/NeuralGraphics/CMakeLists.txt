add_library(dpnf_graphics STATIC
    src/NeuralGraphicsPipeline.cpp
)

target_include_directories(dpnf_graphics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dpnf_graphics
    PUBLIC
        dpnf_neural
        dpnf_gpu
    PRIVATE
        dpnf_warnings
)
