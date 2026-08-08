#include "CutAccuracy/Geometry.hpp"

#include <algorithm>
#include <cmath>

namespace CutAccuracy {

namespace {

void appendUnique(std::vector<Vec3>& pts, const Vec3& p, double eps) {
    for (const auto& q : pts) {
        if (nearlyEqual(p, q, eps)) return;
    }
    pts.push_back(p);
}

Face cleanupFace(Face face, double eps) {
    if (face.empty()) return face;

    Face out;
    out.reserve(face.size());
    for (const auto& p : face) {
        if (out.empty() || !nearlyEqual(out.back(), p, eps)) out.push_back(p);
    }
    if (out.size() > 1 && nearlyEqual(out.front(), out.back(), eps)) out.pop_back();

    if (out.size() < 3) return {};
    return out;
}

} // namespace

ConvexPolyhedron ConvexPolyhedron::unitCube(double h) {
    const Vec3 p000{-h,-h,-h};
    const Vec3 p001{-h,-h, h};
    const Vec3 p010{-h, h,-h};
    const Vec3 p011{-h, h, h};
    const Vec3 p100{ h,-h,-h};
    const Vec3 p101{ h,-h, h};
    const Vec3 p110{ h, h,-h};
    const Vec3 p111{ h, h, h};

    // Cyclic ordering is all clipping needs. Face winding does not matter to volume().
    return {{
        {p000,p100,p110,p010}, // -z
        {p001,p011,p111,p101}, // +z
        {p000,p001,p101,p100}, // -y
        {p010,p110,p111,p011}, // +y
        {p000,p010,p011,p001}, // -x
        {p100,p101,p111,p110}  // +x
    }};
}

std::vector<Vec3> ConvexPolyhedron::uniqueVertices(double eps) const {
    std::vector<Vec3> verts;
    for (const auto& face : faces) {
        for (const auto& p : face) appendUnique(verts, p, eps);
    }
    return verts;
}

ConvexPolyhedron ConvexPolyhedron::clipped(const Plane& plane, double eps) const {
    ConvexPolyhedron out;
    std::vector<Vec3> capPoints;

    for (const auto& face : faces) {
        if (face.size() < 3) continue;
        Face clippedFace;

        for (size_t i = 0; i < face.size(); ++i) {
            const Vec3& a = face[i];
            const Vec3& b = face[(i + 1) % face.size()];
            const double da = plane.signedDistance(a);
            const double db = plane.signedDistance(b);
            const bool inA = da >= -eps;
            const bool inB = db >= -eps;

            if (inA) clippedFace.push_back(a);

            if (inA != inB) {
                const double denom = da - db;
                if (std::abs(denom) > eps) {
                    const double t = std::clamp(da / denom, 0.0, 1.0);
                    const Vec3 p = lerp(a, b, t);
                    clippedFace.push_back(p);
                    appendUnique(capPoints, p, 1e-7);
                }
            } else {
                if (std::abs(da) <= eps) appendUnique(capPoints, a, 1e-7);
                if (std::abs(db) <= eps) appendUnique(capPoints, b, 1e-7);
            }
        }

        clippedFace = cleanupFace(std::move(clippedFace), 1e-8);
        if (clippedFace.size() >= 3) out.faces.push_back(std::move(clippedFace));
    }

    if (capPoints.size() >= 3) {
        Vec3 c{};
        for (const auto& p : capPoints) c += p;
        c = c / static_cast<double>(capPoints.size());

        const Vec3 n = normalized(plane.n);
        Vec3 helper = std::abs(n.x) < 0.8 ? Vec3{1,0,0} : Vec3{0,1,0};
        Vec3 u = normalized(cross(n, helper));
        if (lengthSq(u) < 1e-12) u = normalized(cross(n, Vec3{0,0,1}));
        const Vec3 v = cross(n, u);

        std::sort(capPoints.begin(), capPoints.end(), [&](const Vec3& a, const Vec3& b) {
            const Vec3 da = a - c;
            const Vec3 db = b - c;
            const double aa = std::atan2(dot(da, v), dot(da, u));
            const double ab = std::atan2(dot(db, v), dot(db, u));
            return aa < ab;
        });

        Face cap = cleanupFace(std::move(capPoints), 1e-8);
        if (cap.size() >= 3) out.faces.push_back(std::move(cap));
    }

    return out;
}

double ConvexPolyhedron::volume() const {
    const auto verts = uniqueVertices();
    if (verts.size() < 4) return 0.0;

    Vec3 c{};
    for (const auto& p : verts) c += p;
    c = c / static_cast<double>(verts.size());

    double vol = 0.0;
    for (const auto& face : faces) {
        if (face.size() < 3) continue;
        const Vec3 a = face[0] - c;
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            const Vec3 b = face[i] - c;
            const Vec3 d = face[i+1] - c;
            vol += std::abs(dot(a, cross(b, d))) / 6.0;
        }
    }
    return vol;
}

ConvexPolyhedron makeMiniNote(const Vec3& directionAxis, bool positiveHalf) {
    Vec3 axis = normalized(directionAxis);
    if (lengthSq(axis) < 1e-12) axis = {0, 1, 0};
    Plane split = Plane::throughPoint(axis, {0,0,0});
    if (!positiveHalf) split = split.flipped();
    return ConvexPolyhedron::unitCube().clipped(split);
}

Vec3 splitAxisForCutDirection(CutDirection direction) {
    switch (direction) {
        case CutDirection::Up:
        case CutDirection::Down:
            return {0, 1, 0};
        case CutDirection::Left:
        case CutDirection::Right:
            return {1, 0, 0};
        case CutDirection::UpRight:
        case CutDirection::DownLeft:
            return normalized(Vec3{1, 1, 0});
        case CutDirection::UpLeft:
        case CutDirection::DownRight:
            return normalized(Vec3{-1, 1, 0});
        case CutDirection::Any:
        case CutDirection::None:
        default:
            return {0, 0, 0};
    }
}

Vec3 splitAxisForCutDirection(CutDirection direction, double angleOffsetDegrees) {
    Vec3 axis = splitAxisForCutDirection(direction);
    if (lengthSq(axis) < 1e-12 || std::abs(angleOffsetDegrees) < 1e-9) return axis;
    const double r = angleOffsetDegrees * 3.14159265358979323846 / 180.0;
    const double c = std::cos(r);
    const double sn = std::sin(r);
    return normalized(Vec3{axis.x * c - axis.y * sn, axis.x * sn + axis.y * c, 0.0});
}

bool hasDirectionalSplit(CutDirection direction) {
    return lengthSq(splitAxisForCutDirection(direction)) > 1e-12;
}

MiniNoteVolumes cutPolyhedronVolumes(
    const ConvexPolyhedron& polyhedron,
    const Plane& saberCutPlaneLocal
) {
    const double total = polyhedron.volume();
    if (total <= 1e-12) return {};

    const double pos = polyhedron.clipped(saberCutPlaneLocal).volume();
    const double neg = std::max(0.0, total - pos);
    return {pos, neg, total};
}

MiniNoteVolumes cutMiniNoteVolumes(
    const Vec3& directionAxis,
    bool positiveHalf,
    const Plane& saberCutPlaneLocal
) {
    return cutPolyhedronVolumes(makeMiniNote(directionAxis, positiveHalf), saberCutPlaneLocal);
}

} // namespace CutAccuracy
