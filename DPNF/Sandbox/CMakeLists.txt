add_executable(dpnf_sandbox
    src/main.cpp
)

target_link_libraries(dpnf_sandbox
    PRIVATE
        dpnf_core
        dpnf_warnings
)
