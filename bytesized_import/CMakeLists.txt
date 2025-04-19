add_library(bytesized_import INTERFACE)
target_include_directories(bytesized_import INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(bytesized_import INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/import.cpp)