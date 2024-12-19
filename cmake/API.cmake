
function(bytesized_use_import TARGET)
    add_subdirectory(${BYTESIZED_DIR}/import)
    target_link_libraries(${TARGET} PRIVATE bytesized_import)
endfunction()