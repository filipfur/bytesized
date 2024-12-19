#include "primitive.h"

inline static void setupAttribPosNorUV() {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glBindVertexArray(0);
}

static gpu::Primitive *_builtinPrimitives[gpu::PRIMITIVE_COUNT];

gpu::Primitive *gpu::getBuiltinPrimitive(gpu::BuiltinPrimitive builtinPrimitive) {
    return _builtinPrimitives[builtinPrimitive];
}

void gpu::createBuiltinPrimitives() {
    _builtinPrimitives[BuiltinPrimitive::SCREEN] =
        createPrimitive((void *)gpu::screen_vertices, sizeof(gpu::screen_vertices),
                        gpu::screen_indices, sizeof(gpu::screen_indices));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glBindVertexArray(0);

    constexpr gpu::Plane<1, 1> grid(64.0f);
    _builtinPrimitives[BuiltinPrimitive::GRID] =
        createPrimitive(grid.vertices, sizeof(grid.vertices), grid.indices, sizeof(grid.indices));
    setupAttribPosNorUV();

    constexpr gpu::Plane<1, 1> plane;
    _builtinPrimitives[BuiltinPrimitive::PLANE] = createPrimitive(
        plane.vertices, sizeof(plane.vertices), plane.indices, sizeof(plane.indices));
    setupAttribPosNorUV();

    gpu::Cube cube;
    _builtinPrimitives[BuiltinPrimitive::CUBE] =
        createPrimitive(cube.vertices, sizeof(cube.vertices), cube.indices, sizeof(cube.indices));
    setupAttribPosNorUV();

    constexpr gpu::Sphere<16, 8> sphere;
    _builtinPrimitives[BuiltinPrimitive::SPHERE] = createPrimitive(
        sphere.vertices, sizeof(sphere.vertices), sphere.indices, sizeof(sphere.indices));
    setupAttribPosNorUV();

    constexpr gpu::Sphere<16, 8> cylinder(Sphere<16, 8>::CYLINDER);
    _builtinPrimitives[BuiltinPrimitive::CYLINDER] = createPrimitive(
        cylinder.vertices, sizeof(cylinder.vertices), cylinder.indices, sizeof(cylinder.indices));
    setupAttribPosNorUV();

    constexpr gpu::Sphere<16, 8> cone(Sphere<16, 8>::CONE_A);
    _builtinPrimitives[BuiltinPrimitive::CONE] =
        createPrimitive(cone.vertices, sizeof(cone.vertices), cone.indices, sizeof(cone.indices));
    setupAttribPosNorUV();
}