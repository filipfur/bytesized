cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED)

project(embed)

add_library(embed src/embed.cpp)
target_include_directories(embed PUBLIC include)

add_executable(embed_main src/embed_main.cpp)
target_link_libraries(embed_main PRIVATE embed)

function(embed_assets assets headers_out)
    set(headers)
    foreach(asset ${assets})
        get_filename_component(file_hpp ${asset} NAME_WE)
        get_filename_component(file_ext ${asset} EXT)
        string(REPLACE "." "" file_ext "${file_ext}")
        set(file_hpp "${CMAKE_BINARY_DIR}/gen/embed/${file_hpp}_${file_ext}.hpp")
        add_custom_command(
            OUTPUT ${file_hpp}
            COMMAND embed_main ${asset} -o ${file_hpp}
            DEPENDS embed_main ${asset}
            VERBATIM)
        list(APPEND headers ${file_hpp})
    endforeach()
    set(${headers_out} ${headers} PARENT_SCOPE)
endfunction()


include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()