enable_testing()

find_package(GTest REQUIRED)

add_executable(test_embed
    main.cpp
    test_embed.cpp
)
target_link_libraries(test_embed
    GTest::gtest
    embed
)
target_compile_definitions(test_embed PRIVATE TEST_INPUT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/inputs\")

add_test(NAME embed_tests COMMAND test_embed)