#include <gtest/gtest.h>

#include "ecs.h"
#include <string>

struct Developer {
    float satisfaction;
    float income;
};
struct ProblemSolver {
    bool success;
};
struct RustUser {
    bool borrowChecking;
    uint32_t errorCode;
};
struct Gamer {
    bool isGaming;
};

typedef ecs::Component<Developer, 0> CDeveloper;
typedef ecs::Component<ProblemSolver, 1> CProblemSolver;
typedef ecs::Component<RustUser, 2> CRustUser;
typedef ecs::Component<Gamer, 3> CGamer;

TEST(TestECS, BasicSystem) {
    ecs::Entity *jerry = ecs::create_entity();
    ecs::attach<CDeveloper, CRustUser, CGamer>(jerry);
    CDeveloper::set({0.5f, 0.0f}, jerry);
    EXPECT_FLOAT_EQ(CDeveloper::get(jerry).satisfaction, 0.5f);

    ecs::Entity *sally = ecs::create_entity();
    ecs::attach<CDeveloper, CProblemSolver>(sally);

    for (size_t i{0}; i < 100; ++i) {
        ecs::System<CRustUser, CDeveloper>::for_each(
            [](ecs::Entity *, RustUser &rustUser, Developer &developer) {
                rustUser.borrowChecking = rand() % 2;
                if (rustUser.borrowChecking) {
                    rustUser.errorCode = rand() % 10;
                }
                if (rustUser.errorCode > 0) {
                    developer.satisfaction -= 0.2f;
                }
            });

        ecs::System<const CDeveloper, CGamer>::for_each(
            [](ecs::Entity *entity, const Developer &developer, Gamer &gamer) {
                if (developer.satisfaction < 0) {
                    gamer.isGaming = true;
                    CDeveloper::detach(entity);
                }
            });
        ecs::System<CDeveloper, CProblemSolver>::for_each(
            [](ecs::Entity *, Developer &developer, ProblemSolver &problemSolver) {
                problemSolver.success = true; // note: not using rust
                if (problemSolver.success) {
                    developer.income += 0.3f;
                }
            });
    }
    EXPECT_TRUE(CRustUser::has(jerry) && CGamer::get(jerry).isGaming);
    EXPECT_GT(CDeveloper::get(sally).income, CDeveloper::get(jerry).income);
}

struct DisplayName {
    char letter;
};

typedef ecs::Component<DisplayName, 16> CDisplayName;

TEST(TestECS, Combinations) {
    ecs::Entity *A = ecs::create_entity();
    ecs::Entity *B = ecs::create_entity();
    ecs::Entity *C = ecs::create_entity();
    CDisplayName::attach(A, B, C);
    CDisplayName::set({'A'}, A);
    CDisplayName::set({'B'}, B);
    CDisplayName::set({'C'}, C);
    std::string combinations = "";
    std::string permutations = "";
    ecs::System<const CDisplayName>::combine(
        [&permutations](ecs::Entity *, const DisplayName &name0, ecs::Entity *,
                        const DisplayName &name1) {
            permutations += name0.letter;
            permutations += name1.letter;
        },
        true);
    ecs::System<const CDisplayName>::combine(
        [&combinations](ecs::Entity *, const DisplayName &name0, ecs::Entity *,
                        const DisplayName &name1) {
            combinations += name0.letter;
            combinations += name1.letter;
        },
        false);
    EXPECT_EQ(permutations, "ABACBABCCACB");
    EXPECT_EQ(combinations, "ABACBC");
}