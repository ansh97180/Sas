find_package(Vulkan REQUIRED)

add_library(dpnf_gpu STATIC
    src/VulkanBackend.cpp
)

target_include_directories(dpnf_gpu
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(dpnf_gpu
    PUBLIC
        dpnf_tensor
        Vulkan::Vulkan
    PRIVATE
        dpnf_warnings
)
