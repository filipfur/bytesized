#include "filesystem.h"

static char __membuf[16777216] = {};
std::string_view filesystem::loadFile(const char *path) {
    FILE *file_ptr;
    size_t len;

    file_ptr = fopen(path, "r");
    if (file_ptr == NULL) {
        printf("file can't be opened (r): %s\n", path);
        return {};
    }
    len = fread(__membuf, 1, (sizeof __membuf) - 1, file_ptr);
    fclose(file_ptr);
    __membuf[len] = '\0';
    return {__membuf, len};
}