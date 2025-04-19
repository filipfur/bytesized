#include "geom_primitive.h"
#include "primer.h"

glm::vec3 geom::_closestPointOBB(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &e,
                                 const glm::mat4 &m) {
    glm::vec3 X;
    glm::vec3 d = p - a;
    X = a;
    glm::vec3 u[3] = {
        glm::normalize(glm::vec3(m[0])),
        glm::normalize(glm::vec3(m[1])),
        glm::normalize(glm::vec3(m[2])),
    };
    for (size_t i{0}; i < 3; ++i) {
        float dist = glm::dot(d, u[i]);
        // dist = std::min(dist, e[i]);
        // dist = std::max(dist, -e[i]);
        if (dist > e[i])
            dist = e[i];
        if (dist < -e[i])
            dist = -e[i];
        X += dist * u[i];
    }
    return X;
}

static std::pair<glm::vec3, float> _obbSphereSeparation(const glm::vec3 &a, const glm::vec3 &e,
                                                        const glm::vec3 &b, float r,
                                                        const glm::mat4 &m) {
    const glm::vec3 ab = b - a;
    float minDist = FLT_MAX;
    size_t ax = 0;

    glm::vec3 u[3] = {
        glm::normalize(glm::vec3(m[0])),
        glm::normalize(glm::vec3(m[1])),
        glm::normalize(glm::vec3(m[2])),
    };

    for (size_t i{0}; i < 3; ++i) {
        float l0 = glm::dot(b, u[i]) - r;
        float l1 = glm::dot(b, u[i]) + r;

        float l2 = glm::dot(a, u[i]) - e[i];
        float l3 = glm::dot(a, u[i]) + e[i];

        float d0 = l0 - l3;
        float d1 = l2 - l1;

        if ((d0 > 0) == (d1 > 0)) {
            float d = glm::min(glm::abs(d0), glm::abs(d1));
            if (d < minDist) {
                minDist = d;
                ax = i;
            }
        }
    }
    if (minDist == FLT_MAX) {
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    }
    return {glm::dot(u[ax], ab) > 0 ? u[ax] : -u[ax], minDist};
}

geom::OBB &geom::OBB::construct(const glm::vec3 &center, const glm::vec3 &extents, TRS *trs) {
    this->_center = center;
    this->trs = trs;
    this->_extents = extents;
    return *this;
}
geom::OBB &geom::OBB::construct(const glm::vec3 *points, size_t length, TRS *trs) {
    auto [min, max] = primer::minMaxOf(points, length, nullptr);
    glm::vec3 e_ = (max - min) * 0.5f;
    glm::vec3 c_ = min + e_;
    return construct(c_, e_, trs);
}
bool geom::OBB::_intersects(geom::Plane &) { return false; }
bool geom::OBB::_intersects(geom::Sphere &other) {
    const glm::vec3 sc = other.origin();
    // glm::vec3 ab = b - a;
    // glm::mat3 R = glm::mat3_cast(trs->r());
    // return _aabbSphereIntersects(a, extents(), glm::transpose(R) * ab, other.radii());
    glm::vec3 p = _closestPointOBB(sc, origin(), extents(), trs->model());
    glm::vec3 v = p - sc;
    float r = other.radii();
    return glm::dot(v, v) <= r * r;
}
bool geom::OBB::_intersects(geom::OBB &other) {
    geom::OBB &a = *this;
    geom::OBB &b = other;
    const glm::mat4 &mA = a.trs->model();
    const glm::mat4 &mB = b.trs->model();
    glm::vec3 u_a[3] = {
        glm::normalize(glm::vec3(mA[0])),
        glm::normalize(glm::vec3(mA[1])),
        glm::normalize(glm::vec3(mA[2])),
    };
    glm::vec3 u_b[3] = {
        glm::normalize(glm::vec3(mB[0])),
        glm::normalize(glm::vec3(mB[1])),
        glm::normalize(glm::vec3(mB[2])),
    };
    glm::vec3 a_e = a.extents();
    glm::vec3 b_e = b.extents();
    float ra, rb;
    static glm::mat3 R, AbsR;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R[i][j] = glm::dot(glm::normalize(u_a[i]), glm::normalize(u_b[j]));
        }
    }
    // Compute translation vector t
    glm::vec3 t = b.origin() - a.origin();
    // Bring translation into a’s coordinate frame
    t = glm::vec3(glm::dot(t, u_a[0]), glm::dot(t, u_a[1]), glm::dot(t, u_a[2]));

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and // their cross product is (near)
    // null (see text for details) for (int i = 0; i < 3; i++)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            AbsR[i][j] = glm::abs(R[i][j]) + FLT_EPSILON;
        }
    }

    float d;

    // Test axes L = A0, L = A1, L = A2
    for (int i = 0; i < 3; i++) {
        ra = a_e[i];
        rb = b_e[0] * AbsR[i][0] + b_e[1] * AbsR[i][1] + b_e[2] * AbsR[i][2];
        d = glm::abs(t[i]);
        if (d > ra + rb)
            return false;
    }
    // Test axes L = B0, L = B1, L = B2
    for (int i = 0; i < 3; i++) {
        ra = a_e[0] * AbsR[0][i] + a_e[1] * AbsR[1][i] + a_e[2] * AbsR[2][i];
        rb = b_e[i];
        d = glm::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]);
        if (d > ra + rb)
            return false;
    }
    // Test axis L = A0 x B0
    ra = a_e[1] * AbsR[2][0] + a_e[2] * AbsR[1][0];
    rb = b_e[1] * AbsR[0][2] + b_e[2] * AbsR[0][1];
    d = glm::abs(t[2] * R[1][0] - t[1] * R[2][0]);
    if (d > ra + rb)
        return false;
    // Test axis L = A0 x B1
    ra = a_e[1] * AbsR[2][1] + a_e[2] * AbsR[1][1];
    rb = b_e[0] * AbsR[0][2] + b_e[2] * AbsR[0][0];
    d = glm::abs(t[2] * R[1][1] - t[1] * R[2][1]);
    if (d > ra + rb)
        return false;
    // Test axis L = A0 x B2
    ra = a_e[1] * AbsR[2][2] + a_e[2] * AbsR[1][2];
    rb = b_e[0] * AbsR[0][1] + b_e[1] * AbsR[0][0];
    d = glm::abs(t[2] * R[1][2] - t[1] * R[2][2]);
    if (d > ra + rb)
        return false;
    // Test axis L = A1 x B0
    ra = a_e[0] * AbsR[2][0] + a_e[2] * AbsR[0][0];
    rb = b_e[1] * AbsR[1][2] + b_e[2] * AbsR[1][1];
    d = glm::abs(t[0] * R[2][0] - t[2] * R[0][0]);
    if (d > ra + rb)
        return false;
    // Test axis L = A1 x B1
    ra = a_e[0] * AbsR[2][1] + a_e[2] * AbsR[0][1];
    rb = b_e[0] * AbsR[1][2] + b_e[2] * AbsR[1][0];
    d = glm::abs(t[0] * R[2][1] - t[2] * R[0][1]);
    if (d > ra + rb)
        return false;
    // Test axis L = A1 x B2
    ra = a_e[0] * AbsR[2][2] + a_e[2] * AbsR[0][2];
    rb = b_e[0] * AbsR[1][1] + b_e[1] * AbsR[1][0];
    d = glm::abs(t[0] * R[2][2] - t[2] * R[0][2]);
    if (d > ra + rb)
        return false;
    // Test axis L = A2 x B0
    ra = a_e[0] * AbsR[1][0] + a_e[1] * AbsR[0][0];
    rb = b_e[1] * AbsR[2][2] + b_e[2] * AbsR[2][1];
    d = glm::abs(t[1] * R[0][0] - t[0] * R[1][0]);
    if (d > ra + rb)
        return false;
    // Test axis L = A2 x B1
    ra = a_e[0] * AbsR[1][1] + a_e[1] * AbsR[0][1];
    rb = b_e[0] * AbsR[2][2] + b_e[2] * AbsR[2][0];
    d = glm::abs(t[1] * R[0][1] - t[0] * R[1][1]);
    if (d > ra + rb)
        return false;
    // Test axis L = A2 x B2
    ra = a_e[0] * AbsR[1][2] + a_e[1] * AbsR[0][2];
    rb = b_e[0] * AbsR[2][1] + b_e[1] * AbsR[2][0];
    d = glm::abs(t[1] * R[0][2] - t[0] * R[1][2]);
    if (d > ra + rb)
        return false;
    // Since no separating axis is found, the OBBs must be intersecting
    return true;
}

std::pair<glm::vec3, float> geom::OBB::_separation(Plane &) { return {{0.0f, 0.0f, 0.0f}, 0.0f}; }
std::pair<glm::vec3, float> geom::OBB::_separation(Sphere &other) {
    return _obbSphereSeparation(origin(), extents(), other.origin(), other.radii(), trs->model());
}
std::pair<glm::vec3, float> geom::OBB::_separation(OBB &other) {
    geom::OBB &a = *this;
    geom::OBB &b = other;
    const glm::mat4 &mA = a.trs->model();
    const glm::mat4 &mB = b.trs->model();
    glm::vec3 u_a[3] = {
        glm::normalize(glm::vec3(mA[0])),
        glm::normalize(glm::vec3(mA[1])),
        glm::normalize(glm::vec3(mA[2])),
    };
    glm::vec3 u_b[3] = {
        glm::normalize(glm::vec3(mB[0])),
        glm::normalize(glm::vec3(mB[1])),
        glm::normalize(glm::vec3(mB[2])),
    };
    glm::vec3 a_e = a._extents * a.trs->s();
    glm::vec3 b_e = b._extents * b.trs->s();
    float ra, rb;
    static glm::mat3 R, AbsR;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R[i][j] = glm::dot(glm::normalize(u_a[i]), glm::normalize(u_b[j]));
        }
    }
    // Compute translation vector t
    glm::vec3 t = b.origin() - a.origin();
    // Bring translation into a’s coordinate frame
    t = glm::vec3(glm::dot(t, u_a[0]), glm::dot(t, u_a[1]), glm::dot(t, u_a[2]));

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and // their cross product is (near)
    // null (see text for details) for (int i = 0; i < 3; i++)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            AbsR[i][j] = glm::abs(R[i][j]) + FLT_EPSILON;
        }
    }

    float minDist = FLT_MAX;
    glm::vec3 minAxis{0.0f, 0.0f, 0.0f};
    float d;
    float dist;

    // Test axes L = A0, L = A1, L = A2
    for (int i = 0; i < 3; i++) {
        ra = a_e[i];
        rb = b_e[0] * AbsR[i][0] + b_e[1] * AbsR[i][1] + b_e[2] * AbsR[i][2];
        d = glm::abs(t[i]);
        if (d > ra + rb)
            return {{0.0f, 0.0f, 0.0f}, 0.0f};
        dist = (ra + rb) - d;
        if (dist < minDist && dist > 0) {
            minDist = dist;
            minAxis = u_a[i];
        }
    }
    // Test axes L = B0, L = B1, L = B2
    for (int i = 0; i < 3; i++) {
        ra = a_e[0] * AbsR[0][i] + a_e[1] * AbsR[1][i] + a_e[2] * AbsR[2][i];
        rb = b_e[i];
        d = glm::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]);
        if (d > ra + rb)
            return {{0.0f, 0.0f, 0.0f}, 0.0f};
        dist = (ra + rb) - d;
        if (dist < minDist && dist > 0) {
            minDist = dist;
            minAxis = u_b[i];
        }
    }
    // Test axis L = A0 x B0
    ra = a_e[1] * AbsR[2][0] + a_e[2] * AbsR[1][0];
    rb = b_e[1] * AbsR[0][2] + b_e[2] * AbsR[0][1];
    d = glm::abs(t[2] * R[1][0] - t[1] * R[2][0]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[0], u_b[0]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A0 x B1
    ra = a_e[1] * AbsR[2][1] + a_e[2] * AbsR[1][1];
    rb = b_e[0] * AbsR[0][2] + b_e[2] * AbsR[0][0];
    d = glm::abs(t[2] * R[1][1] - t[1] * R[2][1]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[0], u_b[1]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A0 x B2
    ra = a_e[1] * AbsR[2][2] + a_e[2] * AbsR[1][2];
    rb = b_e[0] * AbsR[0][1] + b_e[1] * AbsR[0][0];
    d = glm::abs(t[2] * R[1][2] - t[1] * R[2][2]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[0], u_b[2]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A1 x B0
    ra = a_e[0] * AbsR[2][0] + a_e[2] * AbsR[0][0];
    rb = b_e[1] * AbsR[1][2] + b_e[2] * AbsR[1][1];
    d = glm::abs(t[0] * R[2][0] - t[2] * R[0][0]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[1], u_b[0]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A1 x B1
    ra = a_e[0] * AbsR[2][1] + a_e[2] * AbsR[0][1];
    rb = b_e[0] * AbsR[1][2] + b_e[2] * AbsR[1][0];
    d = glm::abs(t[0] * R[2][1] - t[2] * R[0][1]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[1], u_b[1]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A1 x B2
    ra = a_e[0] * AbsR[2][2] + a_e[2] * AbsR[0][2];
    rb = b_e[0] * AbsR[1][1] + b_e[1] * AbsR[1][0];
    d = glm::abs(t[0] * R[2][2] - t[2] * R[0][2]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[1], u_b[2]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A2 x B0
    ra = a_e[0] * AbsR[1][0] + a_e[1] * AbsR[0][0];
    rb = b_e[1] * AbsR[2][2] + b_e[2] * AbsR[2][1];
    d = glm::abs(t[1] * R[0][0] - t[0] * R[1][0]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[2], u_b[0]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A2 x B1
    ra = a_e[0] * AbsR[1][1] + a_e[1] * AbsR[0][1];
    rb = b_e[0] * AbsR[2][2] + b_e[2] * AbsR[2][0];
    d = glm::abs(t[1] * R[0][1] - t[0] * R[1][1]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[2], u_b[1]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A2 x B2
    ra = a_e[0] * AbsR[1][2] + a_e[1] * AbsR[0][2];
    rb = b_e[0] * AbsR[2][1] + b_e[1] * AbsR[2][0];
    d = glm::abs(t[1] * R[0][2] - t[0] * R[1][2]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[2], u_b[2]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Since no separating axis is found, the OBBs must be intersecting
    // glm::vec3 n = glm::normalize(minAxis);
    // print_glm(minDist);
    assert(!glm::isnan(minAxis.x) && !glm::isnan(minAxis.y) && !glm::isnan(minAxis.z));
    return {minAxis, minDist};
}

glm::vec3 geom::OBB::extents() { return trs->s() * _extents; }
glm::vec3 geom::OBB::origin() { return trs->t() + _center; }
glm::vec3 geom::OBB::supportPoint(const glm::vec3 &D) {
    glm::vec3 d = glm::mat3(trs->model()) * D;
    return {
        _center.x + (d.x > 0 ? _extents.x : -_extents.x),
        _center.y + (d.y > 0 ? _extents.y : -_extents.y),
        _center.z + (d.z > 0 ? _extents.z : -_extents.z),
    };
}