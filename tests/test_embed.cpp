#include <gtest/gtest.h>

#include "bdf.h"
#include "embed/boxxy_bdf.hpp"

TEST(TestEmbed, BDF) {
    bdf::Font *font = bdf::createFont((const char *)_embed_boxxy_bdf, sizeof(_embed_boxxy_bdf));
    EXPECT_EQ(font->pointsize, 14);
    EXPECT_EQ(font->dpix, 72);
    EXPECT_EQ(font->dpiy, 72);
}