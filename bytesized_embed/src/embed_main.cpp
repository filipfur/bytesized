#include "embed.h"
#include <cstdio>
#include <cstring>
#include <string>

enum ParseMode { PARSE_NONE, PARSE_IN, PARSE_REPLACE_A, PARSE_REPLACE_B, PARSE_OUT };
int main(int argc, char *argv[]) {
    printf("embed_main: %s\n", argv[0]);
    ParseMode mode = PARSE_NONE;
    const char *outfile = nullptr;
    const char *infile = nullptr;
    const char *replaceA = nullptr;
    const char *replaceB = nullptr;
    ReplaceList replaceList;
    for (int i{1}; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0) {
            mode = PARSE_IN;
        } else if (strcmp(argv[i], "-o") == 0) {
            mode = PARSE_OUT;
        } else if (strcmp(argv[i], "-r") == 0) {
            mode = PARSE_REPLACE_A;
        } else {
            switch (mode) {
            default:
            case PARSE_IN:
                infile = argv[i];
                mode = PARSE_NONE;
                break;
            case PARSE_OUT:
                outfile = argv[i];
                mode = PARSE_NONE;
                break;
            case PARSE_REPLACE_A:
                replaceA = argv[i];
                mode = PARSE_REPLACE_B;
                break;
            case PARSE_REPLACE_B:
                replaceB = argv[i];
                printf("replace %s with %s\n", replaceA, replaceB);
                replaceList.emplace_back(replaceA, replaceB);
                mode = PARSE_NONE;
                break;
            }
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
    embed_to_cpp(infile, outfile, replaceList);
    return 0;
}