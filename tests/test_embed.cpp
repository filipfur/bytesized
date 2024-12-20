#include <gtest/gtest.h>

#include "bdf.h"
#include "embed.h"

TEST(TestEmbed, BDF) {
    bdf::Font *font = bdf::createFont(__bdf__boxxy, __bdf__boxxy_len);
    EXPECT_EQ(font->pointsize, 14);
    EXPECT_EQ(font->dpix, 72);
    EXPECT_EQ(font->dpiy, 72);
}