enable_testing()

find_package(GTest REQUIRED)

add_executable(test_bytesized
    main.cpp
    test_primer.cpp
    test_ecs.cpp
    test_persist.cpp
    test_embed.cpp
    test_geom_primitives.cpp
)

target_link_libraries(test_bytesized
    GTest::gtest #GTest::gtest_main
    bytesized
)

add_test(NAME bytesized_tests COMMAND test_bytesized)