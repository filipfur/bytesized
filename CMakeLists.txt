cmake_minimum_required(VERSION 3.15)
project(bytesized)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BYTESIZED_USE_EMBED ON)
option(BYTESIZED_USE_ENGINE ON)
option(BYTESIZED_USE_COM ON)

set(BYTESIZED_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(BYTESIZED_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (NOT BYTESIZED_LOG_LEVEL)
    set(BYTESIZED_LOG_LEVEL 4)
endif()

add_compile_options(-Wall -Wshadow -Wextra -pedantic -Werror -DLOG_LEVEL=${BYTESIZED_LOG_LEVEL} -DBYTESIZED_DIR=${BYTESIZED_DIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(BYTESIZED_ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/bytesized_assets PARENT_SCOPE)
set(BYTESIZED_ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/bytesized_assets)
add_subdirectory(bytesized_lib)
if(BYTESIZED_USE_EMBED OR BYTESIZED_USE_ENGINE)
    add_subdirectory(bytesized_embed)
endif()
if(BYTESIZED_USE_COM)
    add_subdirectory(bytesized_com)
endif()
if(BYTESIZED_USE_ENGINE)
    add_subdirectory(bytesized_engine)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()
