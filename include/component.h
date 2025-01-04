#include "ecs.h"

#include "geom_primitive.h"
#include "trs.h"
#include <glm/glm.hpp>

struct Pawn {
    TRS *trs;
};
struct Actor : public Pawn {
    glm::vec3 velocity;
};
struct Collider {
    geom::Geometry *geometry;
    bool transforming;
};
struct Controller {
    glm::vec3 walkControl;
    float walkSpeed;
    int jumpControl;
    float jumpSpeed;
    geom::Geometry *floorSense;
    bool onGround;
};
typedef ecs::Component<Pawn, 0> CPawn;
typedef ecs::Component<Actor, 1> CActor;
typedef ecs::Component<Collider, 2> CCollider;
typedef ecs::Component<Controller, 3> CController;