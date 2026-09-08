find_package(GTest REQUIRED)

add_executable(tcamviewer_tests
    test_terminal.cpp
    test_renderer.cpp
    test_c_api.cpp
    test_decoder.cpp
)

target_link_libraries(tcamviewer_tests PRIVATE
    tcamviewer
    GTest::gtest
    GTest::gtest_main
    pthread
)

include(GoogleTest)
gtest_discover_tests(tcamviewer_tests)
