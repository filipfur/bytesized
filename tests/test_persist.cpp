#include <gtest/gtest.h>

#include "persist.h"

TEST(TestPersist, Basic) {
    persist::open("testPersist.bin");
    persist::write(persist::World{"kappa", 2, 4, 123, 0});
    persist::write(persist::Scene{"keepo", 3, 0, 0});
    persist::write(persist::Node{"node.001", 1, 10});
    persist::write(persist::Node{"node.002", 2, 20});
    persist::write(persist::Node{"node.003", 3, 30});
    persist::write(persist::Scene{"nodderwon", 1, 0xACDC, 777});
    persist::write(persist::Node{"jerry", 1, 10});
    persist::write(persist::Entity{0, 1, {}, {}, {}, 10, 1000});
    persist::write(persist::Entity{0, 1, {}, {}, {}, 20, 2000});
    persist::write(persist::Entity{0, 2, {}, {}, {}, 30, 3000});
    persist::write(persist::Entity{1, 0, {}, {}, {}, 40, 4000});
    persist::close();

    persist::World *world = persist::load("testPersist.bin");
    persist::Scene *scene0 = (persist::Scene *)(world + 1);
    persist::Node *node0 = (persist::Node *)(scene0 + 1);
    persist::Scene *scene1 = (persist::Scene *)(node0 + scene0->nodes);
    persist::Node *node1 = (persist::Node *)(scene1 + 1);
    EXPECT_EQ(world->info, 123);
    EXPECT_EQ(scene0->nodes, 3);
    EXPECT_STREQ(world->name, "kappa");
    EXPECT_STREQ(scene0->name, "keepo");
    EXPECT_STREQ((node0 + 1)->name, "node.002");
    EXPECT_TRUE((node0 + 2)->info == 3 && (node0 + 2)->extra == 30);
    EXPECT_STREQ(scene1->name, "nodderwon");
    EXPECT_EQ(scene1->info, 0xACDC);
    EXPECT_EQ(scene1->extra, 777);
    EXPECT_STREQ(node1->name, "jerry");
    EXPECT_STREQ(world->scene(1)->node(0)->name, "jerry");
    EXPECT_STREQ(world->scene(0)->node(1)->name, "node.002");
    EXPECT_EQ(world->firstEntity()->extra, 1000);
    EXPECT_EQ((world->firstEntity() + 3)->extra, 4000);
}