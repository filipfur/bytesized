#pragma once

#include <glm/glm.hpp>
#include <vector>

struct VertexObject {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<uint16_t> indices;
};