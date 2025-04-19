#pragma once

#include <string_view>

namespace filesystem {
std::string_view loadFile(const char *path);
}