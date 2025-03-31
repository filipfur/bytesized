#include <gtest/gtest.h>

#include "recycler.hpp"

struct Unit {
    float val;
    char str[20];
};

static recycler<Unit, 20> units;

TEST(TestRecycler, Basic) {
    Unit *u0 = units.acquire();
    Unit *u1 = units.acquire();
    Unit *u2 = units.acquire();
    units.free(u0);
    units.free(u1);
    Unit *u3 = units.acquire();
    units.free(u2);
    Unit *u4 = units.acquire();
    Unit *u5 = units.acquire();
    ASSERT_EQ(u1, u3);
    ASSERT_EQ(u0, u5);
    ASSERT_EQ(u2, u4);
}