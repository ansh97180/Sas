find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(dpnf_tests
    src/CoreFoundationTests.cpp
)

target_link_libraries(dpnf_tests
    PRIVATE
        dpnf_core
        dpnf_warnings
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

add_test(NAME DPNF_Phase0_UnitTests COMMAND dpnf_tests)
