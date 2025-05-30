#include <gtest/gtest.h>

#include "dex.h"
#include "embed.h"

#ifndef TEST_INPUT_DIR
#define TEST_INPUT_DIR ""
#endif

TEST(TestEmbed, Basic) {
    static constexpr size_t len = dex::strlen(TEST_INPUT_DIR);
    EXPECT_GT(len, 0);
    EXPECT_STREQ(dex::filename(TEST_INPUT_DIR), "inputs");
    EXPECT_TRUE(true);
    const std::string textFile = std::string{TEST_INPUT_DIR} + "/text.txt";
    embed_to_cpp(textFile.c_str(), "gen/text.hpp", {});
    EXPECT_EQ(dex::to_hex(15), 'f');
    EXPECT_EQ(dex::to_hex(10), 'a');
    EXPECT_EQ(dex::to_hex(9), '9');
    EXPECT_EQ(dex::to_hex(0), '0');
}