#include "embed.h"
#include "dex.h"
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>

static char __membuf[1'000'000'000] = {};
static std::string_view _loadFile(const char *path) {
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

#define WRITE_TEXT(file, text)                                                                     \
    do {                                                                                           \
        static constexpr char data[] = text;                                                       \
        fwrite(data, sizeof(data) - 1, 1, file);                                                   \
    } while (0)

void embed_to_cpp(const char *in_file, const char *out_file, const ReplaceList &replaceList) {
    auto content = _loadFile(in_file);

    std::string str;
    if (!replaceList.empty()) {
        str.assign(content);
        for (const auto &replace : replaceList) {
            while (str.find(replace.first) != std::string::npos) {
                str.replace(str.find(replace.first), std::strlen(replace.first), replace.second);
            }
        }
        content = str;
    }

    in_file = dex::filename(in_file);
    std::string symbol_name{in_file};
    dex::replace(symbol_name, '.', '_');

    auto out_dir = std::filesystem::path(out_file).parent_path();
    if (!std::filesystem::exists(out_dir)) {
        std::filesystem::create_directories(out_dir);
    }

    FILE *file = fopen(out_file, "w");
    if (file == NULL) {
        printf("file can't be opened (w): %s\n", out_file);
    }
    WRITE_TEXT(file, "#pragma once\nstatic const unsigned char _embed_");
    fwrite(symbol_name.c_str(), symbol_name.length(), 1, file);
    WRITE_TEXT(file, "[] = {");

    static char arr[] = " 0x00,";
    for (size_t i{0}; i < content.length(); ++i) {
        arr[3] = dex::to_hex(content[i] >> 4 & 0xF);
        arr[4] = dex::to_hex(content[i] & 0xF);
        fwrite(arr, sizeof(arr) - 1, 1, file);
    }

    WRITE_TEXT(file, " 0x00};\n");

    fclose(file);
}
