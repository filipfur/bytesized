#include <gtest/gtest.h>

#include "primer.h"

struct Thing {
    Thing *ptr;
};

void dispose_thing(Thing **thing) { *thing = nullptr; }

TEST(TestPrimer, Basic) {
    EXPECT_EQ(1, 1);
    EXPECT_FLOAT_EQ(primer::angleDistance(20.0f, 160.2f), 140.2f);
    EXPECT_FLOAT_EQ(primer::angleDistance(20.0f, -20.2f), -40.2f);
    EXPECT_FLOAT_EQ(primer::angleDistance(20.0f, -100.2f), -120.2f);
    EXPECT_FLOAT_EQ(primer::angleDistance(180.0f, -180.0f), 0.0f);
    EXPECT_FLOAT_EQ(primer::angleDistance(179.0f, -179.0f), 2.0f);
    EXPECT_FLOAT_EQ(primer::angleDistance(-179.0f, 179.0f), -2.0f);

    EXPECT_FLOAT_EQ(primer::normalizedAngle(220.0f), -140.0f);
    EXPECT_FLOAT_EQ(primer::normalizedAngle(181.0f), -179.0f);
    EXPECT_FLOAT_EQ(primer::normalizedAngle(-181.0f), 179.0f);

    Thing thing_a;
    Thing thing_b;
    thing_a.ptr = &thing_b;
    EXPECT_NE(thing_a.ptr, nullptr);
    dispose_thing(&thing_a.ptr);
    EXPECT_EQ(thing_a.ptr, nullptr);
}