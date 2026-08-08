#pragma once

#include "CutAccuracy/Math.hpp"
#include <optional>
#include <vector>

namespace CutAccuracy {

struct OrientedBox {
    Vec3 center{};
    Vec3 axisX{1,0,0};
    Vec3 axisY{0,1,0};
    Vec3 axisZ{0,0,1};
    Vec3 halfExtent{0.5,0.5,0.5};
};

struct SaberPlaneSample {
    double timeSeconds{0.0};
    Vec3 point{};   // point on instantaneous saber sweep/cut plane
    Vec3 normal{};  // plane normal
};

// Negative/zero means the plane intersects the box; positive means separated.
double planeBoxClearance(const SaberPlaneSample& sample, const OrientedBox& box);

// Finds the contiguous intersection interval nearest cutTime and linearly interpolates
// entry/exit zero-crossings. Returns nullopt if both boundaries are not observed.
std::optional<double> traversalTimeSeconds(
    const std::vector<SaberPlaneSample>& samples,
    const OrientedBox& frozenNoteBox,
    double cutTime,
    double epsilonSeconds = 1e-6
);

} // namespace CutAccuracy
