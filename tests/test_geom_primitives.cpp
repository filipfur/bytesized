#include <gtest/gtest.h>

#include "geom_primitive.h"
#include "primer.h"

static const glm::vec3 O{0.0f, 0.0f, 0.0f};

bool operator==(const glm::vec3 &a, const glm::vec3 &b) { return primer::isNear(a, b); }

struct TestGeomPrimitives : public testing::Test {
    void SetUp() override {
        sphere = geom::createSphere();
        aabb = geom::createAABB();

        a.translation = O;
        a.rotation = {1.0f, 0.0f, 0.0f, 0.0f};
        a.scale = {1.0f, 1.0f, 1.0f};

        b.translation = O;
        b.rotation = {1.0f, 0.0f, 0.0f, 0.0f};
        b.scale = {1.0f, 1.0f, 1.0f};

        sphere->construct(O, 1.0f);
        sphere->trs = nullptr;
        aabb->construct(glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{1.0f, 1.0f, 1.0f});
        aabb->trs = nullptr;
    }

    TRS a;
    TRS b;
    geom::Sphere *sphere;
    geom::AABB *aabb;
};

TEST_F(TestGeomPrimitives, TestSphereTransform) {
    EXPECT_EQ(sphere->type(), geom::Geometry::Type::SPHERE);
    EXPECT_EQ(sphere->trs, nullptr);

    EXPECT_FLOAT_EQ(sphere->_radii, 1.0f);
    EXPECT_FLOAT_EQ(sphere->radii(), 1.0f);
    EXPECT_EQ(sphere->_center, O);
    EXPECT_EQ(sphere->origin(), O);

    a.translation = {1.0f, 2.0f, 3.0f};
    a.scale = {2.4f, 2.4f, 2.4f};
    sphere->trs = &a;

    EXPECT_FLOAT_EQ(sphere->_radii, 1.0f);
    EXPECT_FLOAT_EQ(sphere->radii(), 2.4f);
    EXPECT_EQ(sphere->_center, O);
    EXPECT_EQ(sphere->origin(), glm::vec3(1.0f, 2.0f, 3.0f));
}

TEST_F(TestGeomPrimitives, TestAABBTransform) {
    auto min_max0 = aabb->minMax();
    EXPECT_EQ(min_max0.first, glm::vec3(-1.0f, -1.0f, -1.0f));
    EXPECT_EQ(min_max0.second, glm::vec3(1.0f, 1.0f, 1.0f));
    EXPECT_EQ(aabb->origin(), O);

    auto min_max1 = aabb->minMax();
    aabb->trs = &a;
    EXPECT_EQ(min_max1.first, glm::vec3(-1.0f, -1.0f, -1.0f));
    EXPECT_EQ(min_max1.second, glm::vec3(1.0f, 1.0f, 1.0f));
    EXPECT_EQ(aabb->origin(), a.t());
}

TEST_F(TestGeomPrimitives, TestSphereAABB) {
    sphere->trs = &a;

    {
        a.translation = {1.9f, 0.0f, 0.0f};
        EXPECT_TRUE(aabb->intersects(*sphere));
        auto [n, l] = aabb->separation(*sphere);
        EXPECT_EQ(n, glm::vec3(1.0f, 0.0f, 0.0f));
        EXPECT_FLOAT_EQ(l, 0.1f);
    }
    {
        a.translation = {0.0f, 1.9f, 0.0f};
        EXPECT_TRUE(aabb->intersects(*sphere));
        auto [n, l] = aabb->separation(*sphere);
        EXPECT_EQ(n, glm::vec3(0.0f, 1.0f, 0.0f));
        EXPECT_FLOAT_EQ(l, 0.1f);
    }
    {
        a.translation = {0.0f, 0.0f, 1.9f};
        EXPECT_TRUE(aabb->intersects(*sphere));
        auto [n, l] = aabb->separation(*sphere);
        EXPECT_EQ(n, glm::vec3(0.0f, 0.0f, 1.0f));
        EXPECT_FLOAT_EQ(l, 0.1f);
    }
    {
        a.translation = {-1.9f, 0.0f, 0.0f};
        EXPECT_TRUE(aabb->intersects(*sphere));
        auto [n, l] = aabb->separation(*sphere);
        EXPECT_EQ(n, glm::vec3(-1.0f, 0.0f, 0.0f));
        EXPECT_FLOAT_EQ(l, 0.1f);
    }
    {
        a.translation = {0.0f, -1.9f, 0.0f};
        EXPECT_TRUE(aabb->intersects(*sphere));
        auto [n, l] = aabb->separation(*sphere);
        EXPECT_EQ(n, glm::vec3(0.0f, -1.0f, 0.0f));
        EXPECT_FLOAT_EQ(l, 0.1f);
    }
    {
        a.translation = {0.0f, 0.0f, -1.9f};
        EXPECT_TRUE(aabb->intersects(*sphere));
        auto [n, l] = aabb->separation(*sphere);
        EXPECT_EQ(n, glm::vec3(0.0f, 0.0f, -1.0f));
        EXPECT_FLOAT_EQ(l, 0.1f);
    }
}