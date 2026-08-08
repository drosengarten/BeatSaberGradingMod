#include "CutAccuracy/Traversal.hpp"

#include <algorithm>
#include <cmath>

namespace CutAccuracy {

double planeBoxClearance(const SaberPlaneSample& sample, const OrientedBox& box) {
    const Vec3 n = normalized(sample.normal);
    if (lengthSq(n) < 1e-12) return 1e9;

    const double centerDistance = dot(n, box.center - sample.point);
    const double radius =
        std::abs(dot(n, normalized(box.axisX))) * box.halfExtent.x +
        std::abs(dot(n, normalized(box.axisY))) * box.halfExtent.y +
        std::abs(dot(n, normalized(box.axisZ))) * box.halfExtent.z;

    return std::abs(centerDistance) - radius;
}

namespace {

double interpolateCrossing(double t0, double g0, double t1, double g1) {
    const double denom = g0 - g1;
    if (std::abs(denom) < 1e-12) return (t0+t1)*0.5;
    const double u = std::clamp(g0 / denom, 0.0, 1.0);
    return t0 + (t1-t0)*u;
}
}

std::optional<double> traversalTimeSeconds(
    const std::vector<SaberPlaneSample>& input,
    const OrientedBox& box,
    double cutTime,
    double epsilonSeconds
) {
    if (input.size() < 3) return std::nullopt;

    std::vector<SaberPlaneSample> samples = input;
    std::sort(samples.begin(), samples.end(), [](const auto& a, const auto& b) {
        return a.timeSeconds < b.timeSeconds;
    });

    std::vector<double> g(samples.size());
    for (size_t i=0;i<samples.size();++i) g[i] = planeBoxClearance(samples[i], box);

    // Pick an intersecting sample closest to the actual cut event.
    std::optional<size_t> anchor;
    double bestDt = 1e99;
    for (size_t i=0;i<samples.size();++i) {
        if (g[i] <= 0.0) {
            const double dt = std::abs(samples[i].timeSeconds - cutTime);
            if (dt < bestDt) { bestDt = dt; anchor = i; }
        }
    }
    if (!anchor) return std::nullopt;

    size_t first = *anchor;
    while (first > 0 && g[first-1] <= 0.0) --first;
    size_t last = *anchor;
    while (last + 1 < samples.size() && g[last+1] <= 0.0) ++last;

    if (first == 0 || last + 1 >= samples.size()) return std::nullopt;
    if (g[first-1] <= 0.0 || g[last+1] <= 0.0) return std::nullopt;

    const double entry = interpolateCrossing(
        samples[first-1].timeSeconds, g[first-1],
        samples[first].timeSeconds, g[first]
    );
    const double exit = interpolateCrossing(
        samples[last].timeSeconds, g[last],
        samples[last+1].timeSeconds, g[last+1]
    );

    const double dt = exit-entry;
    return dt > epsilonSeconds ? std::optional<double>(dt) : std::nullopt;
}

} // namespace CutAccuracy
