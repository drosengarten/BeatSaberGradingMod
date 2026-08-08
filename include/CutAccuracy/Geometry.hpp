#pragma once

#include "CutAccuracy/Math.hpp"
#include <vector>

namespace CutAccuracy {

using Face = std::vector<Vec3>;

struct ConvexPolyhedron {
    std::vector<Face> faces;

    static ConvexPolyhedron unitCube(double halfExtent = 0.5);
    ConvexPolyhedron clipped(const Plane& plane, double eps = 1e-9) const;
    double volume() const;
    std::vector<Vec3> uniqueVertices(double eps = 1e-8) const;
};

struct MiniNoteVolumes {
    double positiveSide{0.0};
    double negativeSide{0.0};
    double total{0.0};
};

enum class CutDirection {
    Up,
    Down,
    Left,
    Right,
    UpLeft,
    UpRight,
    DownLeft,
    DownRight,
    Any,
    None
};

// Returns the note-local axis normal to the mini-note split plane. Opposite
// cut directions intentionally share one split axis: an up-note and a down-note
// are both divided into the same upper/lower halves.
Vec3 splitAxisForCutDirection(CutDirection direction);
Vec3 splitAxisForCutDirection(CutDirection direction, double angleOffsetDegrees);
bool hasDirectionalSplit(CutDirection direction);

// The note is split through its center by a plane perpendicular to directionAxis.
// directionAxis is expressed in note-local coordinates.
ConvexPolyhedron makeMiniNote(const Vec3& directionAxis, bool positiveHalf);

MiniNoteVolumes cutPolyhedronVolumes(
    const ConvexPolyhedron& polyhedron,
    const Plane& saberCutPlaneLocal
);

MiniNoteVolumes cutMiniNoteVolumes(
    const Vec3& directionAxis,
    bool positiveHalf,
    const Plane& saberCutPlaneLocal
);

} // namespace CutAccuracy
