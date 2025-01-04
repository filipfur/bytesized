#include "geom_convexhull.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <list>
#include <set>

#include "glm/gtc/random.hpp"
#include "primer.h"

bool popAlongDirection(std::vector<glm::vec3> &s, const glm::vec3 &D, glm::vec3 &rval) {
    auto [index, distance] = primer::indexAlongDirection(s, D);
    if (index < 0) {
        return false;
    }
    rval = s[index];
    s.erase(s.begin() + index);
    return true;
}

void checkFace(std::set<geom::Face *> &visible, geom::Face *face, const glm::vec3 &p) {
    if (glm::dot(face->normal, p - face->half_edge->vertex) > FLT_EPSILON) {
        if (visible.emplace(face).second) {
            checkFace(visible, face->half_edge->opposite->face, p);
            checkFace(visible, face->half_edge->next->opposite->face, p);
            checkFace(visible, face->half_edge->next->next->opposite->face, p);
        }
    }
}

geom::ConvexHull::ConvexHull(const glm::vec3 *points, size_t length) {
    std::vector<glm::vec3> P{points, points + length};
    glm::vec3 dir = glm::vec3{1.0f, 0.0f, 0.0f}; // glm::ballRand(1.0f);
    glm::vec3 A;
    glm::vec3 B;

    assert(popAlongDirection(P, dir, A));
    assert(popAlongDirection(P, -A, B));

    glm::vec3 AB = B - A;
    float t = -(glm::dot(AB, A) / glm::dot(AB, AB));
    glm::vec3 r = A + AB * t;

    assert(glm::dot(r, r) >= 0);

    glm::vec3 C;
    assert(popAlongDirection(P, r, C));

    glm::vec3 AC = C - A;
    glm::vec3 ABC = glm::cross(AB, AC);

    if (glm::dot(ABC, -A) > 0) {
        std::swap(B, C);
        std::swap(AC, AB);
        ABC = -ABC;
    }

    glm::vec3 D;
    assert(popAlongDirection(P, -ABC, D));

    assert(!primer::isNear(A, D) && !primer::isNear(B, D) && !primer::isNear(C, D));

    Face &abc = faces.emplace_back(glm::normalize(ABC));
    Face &acd = faces.emplace_back(glm::normalize(glm::cross(C - A, D - A)));
    Face &adb = faces.emplace_back(glm::normalize(glm::cross(D - A, B - A)));
    Face &bdc = faces.emplace_back(glm::normalize(glm::cross(D - B, C - B)));

    HalfEdge *ab = &halfEdges.emplace_back(A, &abc);
    HalfEdge *bc = &halfEdges.emplace_back(B, &abc);
    HalfEdge *ca = &halfEdges.emplace_back(C, &abc);

    HalfEdge *ac = &halfEdges.emplace_back(A, &acd);
    HalfEdge *cd = &halfEdges.emplace_back(C, &acd);
    HalfEdge *da = &halfEdges.emplace_back(D, &acd);

    HalfEdge *ad = &halfEdges.emplace_back(A, &adb);
    HalfEdge *db = &halfEdges.emplace_back(D, &adb);
    HalfEdge *ba = &halfEdges.emplace_back(B, &adb);

    HalfEdge *bd = &halfEdges.emplace_back(B, &bdc);
    HalfEdge *dc = &halfEdges.emplace_back(D, &bdc);
    HalfEdge *cb = &halfEdges.emplace_back(C, &bdc);

    connect(ab, bc, ba, &abc);
    connect(bc, ca, cb, &abc);
    connect(ca, ab, ac, &abc);

    connect(ac, cd, ca, &acd);
    connect(cd, da, dc, &acd);
    connect(da, ac, ad, &acd);

    connect(ad, db, da, &adb);
    connect(db, ba, bd, &adb);
    connect(ba, ad, ab, &adb);

    connect(bd, dc, db, &bdc);
    connect(dc, cb, cd, &bdc);
    connect(cb, bd, bc, &bdc);

    addPoints(P);
}

geom::ConvexHull::~ConvexHull() noexcept {
    for (auto &face : faces) {
        face.unlink();
    }
    faces.clear();
    halfEdges.clear();
}

bool geom::ConvexHull::addPoint(const glm::vec3 &p, geom::Face *face, std::list<Face *> *Q) {
    if (face == nullptr) {
        for (geom::Face &f : faces) {
            if (glm::dot(f.normal, p - f.half_edge->vertex) > FLT_EPSILON) {
                face = &f;
                break;
            }
        }
    }
    assert(face != nullptr);
    std::set<geom::Face *> visible;
    checkFace(visible, face, p);
    if (visible.size() < 1) {
        return false;
    }
    auto fIt = visible.begin();
    while (fIt != visible.end()) {
        Face *f = *fIt;
        HalfEdge *e0 = f->half_edge;
        HalfEdge *e1 = f->half_edge->next;
        HalfEdge *e2 = f->half_edge->next->next;
        f->unlink();
        halfEdges.remove_if([e0, e1, e2](const geom::HalfEdge &he) {
            return (&he == e0) || (&he == e1) || (&he == e2);
        });
        if (Q) {
            Q->remove(f);
        }
        faces.remove_if([f](const auto &face) { return f->id == face.id; });

        fIt = visible.erase(fIt);
    }

    HalfEdge *boundary{nullptr};
    for (const auto &f : faces) {
        HalfEdge *he = f.half_edge;
        do {
            if (he->opposite == nullptr) {
                boundary = he;
                break;
            } else {
                he = he->next;
            }
        } while (he != f.half_edge);
        if (boundary) {
            break;
        }
    }
    HalfEdge *he = boundary;
    HalfEdge *prev{nullptr};
    std::vector<Face *> newFaces;
    do {
        if (prev) {
            const glm::vec3 &U = he->vertex;
            const glm::vec3 &V = prev->vertex;
            const glm::vec3 &W = p;

            const glm::vec3 UV = V - U;
            const glm::vec3 UW = W - U;
            const glm::vec3 UVW = glm::cross(UV, UW);
            const glm::vec3 UVW_norm = glm::normalize(UVW);

            Face &uvw = faces.emplace_back(glm::normalize(UVW_norm));

            HalfEdge *uv = &halfEdges.emplace_back(U, &uvw);
            HalfEdge *vw = &halfEdges.emplace_back(V, &uvw);
            HalfEdge *wu = &halfEdges.emplace_back(W, &uvw);

            connect(uv, vw, prev, &uvw);
            prev->opposite = uv;
            connect(vw, wu, nullptr, &uvw);
            connect(wu, uv, nullptr, &uvw);

            newFaces.push_back(&uvw);
            if (Q) {
                Q->push_back(&uvw);
            }
        }
        prev = he;
        he = he->next;
        while (he != boundary && he->opposite != nullptr) {
            he = he->opposite->next;
        }
    } while (prev->opposite == nullptr);

    for (size_t i{0}; i < newFaces.size(); ++i) {
        Face *f = newFaces[i];
        auto n = newFaces.size();
        auto k1 = (i == 0) ? (n - 1) : (i - 1);
        auto k2 = (i == (n - 1)) ? 0 : (i + 1);
        Face *fp = newFaces[k1];
        Face *fn = newFaces[k2];
        assert(f->half_edge->opposite == nullptr);       // ca
        assert(f->half_edge->prev->opposite == nullptr); // bc
        f->half_edge->opposite = fn->half_edge->prev;    // bc(i) -> ca(i+1)
        f->half_edge->prev->opposite = fp->half_edge;    // ca(i) -> ca(i-1)
    }
    return true;
}

void geom::ConvexHull::addPoints(std::vector<glm::vec3> &P) {
    std::list<Face *> Q;
    std::for_each(std::begin(faces), std::end(faces), [&Q](Face &face) { Q.push_back(&face); });
    while (!Q.empty() && !P.empty()) {
        geom::Face *face = Q.front();
        Q.erase(Q.begin());

        glm::vec3 p;
        if (!popAlongDirection(P, face->normal, p)) {
            continue;
        }
        if (!addPoint(p, face, &Q)) {
            P.push_back(p); // ?
        }
    }
}

VertexObject geom::ConvexHull::vertexObject() const {
    VertexObject vertexObject;
    vertexObject.positions.reserve(faces.size() * 3);
    vertexObject.normals.reserve(faces.size() * 3);
    vertexObject.uvs.reserve(faces.size() * 3);
    vertexObject.indices.reserve(faces.size() * 3);
    for (const auto &face : faces) {
        vertexObject.positions.insert(vertexObject.positions.end(),
                                      {face.half_edge->vertex, face.half_edge->next->vertex,
                                       face.half_edge->next->next->vertex});
        vertexObject.normals.insert(vertexObject.normals.end(),
                                    {face.normal, face.normal, face.normal});
        vertexObject.uvs.insert(
            vertexObject.uvs.end(),
            {glm::vec2{0.0f, 0.0f}, glm::vec2{0.5f, 0.5f}, glm::vec2{1.0f, 1.0f}});
        uint16_t i = static_cast<uint16_t>(vertexObject.indices.size());
        vertexObject.indices.insert(vertexObject.indices.end(),
                                    {i, (uint16_t)(i + 1), (uint16_t)(i + 2)});
    }
    return vertexObject;
}

struct Vec3Comparator {
    bool operator()(const glm::vec3 &lhs, const glm::vec3 &rhs) const {
        if (lhs.x != rhs.x)
            return lhs.x < rhs.x;
        if (lhs.y != rhs.y)
            return lhs.y < rhs.y;
        return lhs.z < rhs.z;
    }
};

std::vector<glm::vec3> geom::ConvexHull::points() const {
    std::vector<glm::vec3> rval;
    for (const auto &face : faces) {
        rval.insert(rval.end(), {face.half_edge->vertex, face.half_edge->next->vertex,
                                 face.half_edge->next->next->vertex});
    }
    return rval;
}

std::vector<glm::vec3> geom::ConvexHull::uniquePoints() const {
    std::set<glm::vec3, Vec3Comparator> rval;
    for (const auto &face : faces) {
        rval.emplace(face.half_edge->vertex);
        rval.emplace(face.half_edge->next->vertex);
        rval.emplace(face.half_edge->next->next->vertex);
    }
    return {rval.begin(), rval.end()};
}

void geom::ConvexHull::connect(HalfEdge *self, HalfEdge *next, HalfEdge *opposite, Face *face) {
    self->next = next;
    next->prev = self;
    self->opposite = opposite;
    self->face = face;
    face->half_edge = self;
    face->D = glm::dot(-face->half_edge->vertex, face->normal);
}

std::optional<glm::vec3> geom::ConvexHull::rayIntersect(const glm::vec3 &ray_origin,
                                                        const glm::vec3 &ray_dir) const {
    std::optional<glm::vec3> intersection;
    float minDistance{0.0f};
    for (auto &face : faces) {
        const glm::vec3 &a = face.half_edge->vertex;
        const glm::vec3 &b = face.half_edge->next->vertex;
        const glm::vec3 &c = face.half_edge->next->next->vertex;
        const glm::vec3 ab = b - a;
        const glm::vec3 ac = c - a;
        const glm::vec3 h = glm::cross(ray_dir, ac);
        float alpha = glm::dot(ab, h);
        if (alpha > -FLT_EPSILON && alpha < FLT_EPSILON) {
            continue;
        }
        float f = 1.0f / alpha;
        const glm::vec3 s = ray_origin - a;
        float u = glm::dot(s, h) * f;
        if (u < 0.0f || u > 1.0f) {
            continue;
        }
        const glm::vec3 q = glm::cross(s, ab);
        float v = glm::dot(ray_dir, q) * f;
        if (v < 0.0f || (u + v) > 1.0f) {
            continue;
        }
        float t = glm::dot(ac, q) * f;
        if (t > FLT_EPSILON) {
            glm::vec3 candid = ray_origin + ray_dir * t;
            glm::vec3 delta = candid - ray_origin;
            float distance = glm::dot(delta, delta);
            if (intersection.has_value()) {
                if (distance < minDistance) {
                    intersection = candid;
                    minDistance = distance;
                }
            } else {
                intersection = candid;
                minDistance = distance;
            }
        }
    }
    return intersection;
}