#include "embed.h"
#include <cstdio>
#include <cstring>
#include <string>

enum ParseMode { PARSE_NONE, PARSE_IN, PARSE_OUT };
int main(int argc, char *argv[]) {
    printf("embed_main: %s\n", argv[0]);
    ParseMode mode = PARSE_NONE;
    const char *outfile = nullptr;
    const char *infile = nullptr;
    for (int i{1}; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0) {
            mode = PARSE_IN;
        }
        if (strcmp(argv[i], "-o") == 0) {
            mode = PARSE_OUT;
        } else {
            switch (mode) {
            default:
            case PARSE_IN:
                infile = argv[i];
                break;
            case PARSE_OUT:
                outfile = argv[i];
                break;
            }
            mode = PARSE_NONE;
        }
    }
    if (infile == nullptr) {
        printf("Error: No input file.\n");
        return 1;
    }
    std::string str;
    if (outfile == nullptr) {
        str = std::string{infile} + ".hpp";
        outfile = str.c_str();
    }
    embed_to_cpp(infile, outfile);
    return 0;
}