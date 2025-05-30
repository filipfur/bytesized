#include "filesystem.h"
#include "logging.h"

#include <cstdio>

#ifndef BYTESIZED_FILE_BUFFER_LENGTH
#define BYTESIZED_FILE_BUFFER_LENGTH 0xFFFF
#endif

static char __membuf[BYTESIZED_FILE_BUFFER_LENGTH] = {};
std::string_view filesystem::loadFile(const char *path) {
    FILE *file_ptr;
    size_t len;

    file_ptr = fopen(path, "r");
    if (file_ptr == NULL) {
        printf("file can't be opened (r): %s\n", path);
        return {};
    }
    len = fread(__membuf, 1, sizeof(__membuf) - 1, file_ptr);
    if (len == (sizeof(__membuf) - 1)) {
        LOG_WARN("File buffer is potentially insufficient.");
    }
    fclose(file_ptr);
    __membuf[len] = '\0';
    return {__membuf, len};
}