add_library(bytesized_embed embed.c)
target_include_directories(bytesized_embed PUBLIC .)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED)