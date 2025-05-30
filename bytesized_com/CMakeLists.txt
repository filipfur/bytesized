add_library(bytesized_com
    src/socket.cpp
    src/com_thread.cpp
)

find_package(SDL2 REQUIRED)

target_include_directories(bytesized_com PUBLIC include ${SDL2_INCLUDE_DIR})
target_link_libraries(bytesized_com PUBLIC ${SDL2_LIBRARY})
