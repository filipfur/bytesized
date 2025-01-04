#pragma once

#include <list>
#include <optional>
#include <vector>

#include "glm/glm.hpp"
#include "halfedge.h"
#include "vertexobject.h"

namespace geom {
class ConvexHull : public Polyhedra {
  public:
    ConvexHull(const glm::vec3 *points, size_t length);
    ~ConvexHull() noexcept;

    bool addPoint(const glm::vec3 &p, Face *face = nullptr, std::list<Face *> *Q = nullptr);
    void addPoints(std::vector<glm::vec3> &P);

    VertexObject vertexObject() const;

    std::vector<glm::vec3> points() const;
    std::vector<glm::vec3> uniquePoints() const;

#if 0
    std::list<Face>::iterator begin() { return _faces.begin(); }
    std::list<Face>::iterator end() { return _faces.end(); }
    std::list<Face>::const_iterator begin() const { return _faces.begin(); }
    std::list<Face>::const_iterator end() const { return _faces.end(); }
#endif

    std::optional<glm::vec3> rayIntersect(const glm::vec3 &p, const glm::vec3 &d) const;

  private:
    void connect(HalfEdge *self, HalfEdge *next, HalfEdge *opposite, Face *face);
};
} // namespace geom