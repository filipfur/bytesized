#pragma once

#include <glm/glm.hpp>
#include <vector>

typedef struct {
    glm::vec3 p[3];
} TRIANGLE;

typedef struct {
    glm::vec3 p[8];
    float val[8];
} GRIDCELL;

size_t Polygonise(GRIDCELL grid, float isolevel, std::vector<TRIANGLE> &triangles);