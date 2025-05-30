#pragma once

#include <list>

using ReplaceList = std::list<std::pair<const char *, const char *>>;

void embed_to_cpp(const char *path, const char *out_dir, const ReplaceList &replaceList);