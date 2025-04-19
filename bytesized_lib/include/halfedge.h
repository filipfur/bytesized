#pragma once

#include <cassert>
#include <cstdint>
#include <list>

#include "glm/glm.hpp"

namespace geom {
struct HalfEdge {
    HalfEdge(const glm::vec3 &vertex_, struct Face *face_) : vertex{vertex_}, face{face_} {
        static uint32_t nextId{0};
        id = nextId++;
    }

    uint32_t id;
    glm::vec3 vertex;
    struct Face *face;
    HalfEdge *next;
    HalfEdge *prev;
    HalfEdge *opposite;
};

struct Face {
    Face(const glm::vec3 &normal_) : normal{normal_} {
        assert(!std::isnan(normal.x));
        assert(glm::dot(normal, normal) > FLT_EPSILON);
        static uint32_t nextId{0};
        id = nextId++;
    }

    void unlink() {
        HalfEdge *he = this->half_edge;
        do {
            HalfEdge *next = he->next;
            if (he->opposite) {
                he->opposite->opposite = nullptr;
                he->opposite = nullptr;
            }
            he->prev = nullptr;
            he->next = nullptr;
            he->face = nullptr;
            // delete(he) ! do something
            he = next;
        } while (he != this->half_edge);
        this->half_edge = nullptr;
    }

    glm::vec3 center() {
        return (half_edge->vertex + half_edge->next->vertex + half_edge->next->next->vertex) *
               0.333333f;
    }

    uint32_t id;
    HalfEdge *half_edge;
    glm::vec3 normal;
    float D;
};

struct Polyhedra {
    std::list<HalfEdge> halfEdges;
    std::list<Face> faces;
};

} // namespace geom