#include "CutAccuracy/Traversal.hpp"

#include <algorithm>
#include <cmath>

namespace CutAccuracy {

namespace {

struct PlaneBoxDistance {
    double signedDistance{0.0};
    double radius{0.0};
    double clearance{1e9};
    bool valid{false};
};

PlaneBoxDistance measurePlaneBox(const SaberPlaneSample& sample, const OrientedBox& box) {
    const Vec3 n = normalized(sample.normal);
    if (lengthSq(n) < 1e-12) return {};

    const double centerDistance = dot(n, box.center - sample.point);
    const double radius =
        std::abs(dot(n, normalized(box.axisX))) * box.halfExtent.x +
        std::abs(dot(n, normalized(box.axisY))) * box.halfExtent.y +
        std::abs(dot(n, normalized(box.axisZ))) * box.halfExtent.z;

    return {centerDistance, radius, std::abs(centerDistance) - radius, true};
}

double interpolateCrossing(double t0, double g0, double t1, double g1) {
    const double denom = g0 - g1;
    if (std::abs(denom) < 1e-12) return (t0+t1)*0.5;
    const double u = std::clamp(g0 / denom, 0.0, 1.0);
    return t0 + (t1-t0)*u;
}

std::optional<double> sparseCrossingTraversalTime(
    const std::vector<SaberPlaneSample>& samples,
    const std::vector<PlaneBoxDistance>& distances,
    double cutTime,
    double epsilonSeconds
) {
    std::optional<double> bestDuration;
    double bestCutDistance = 1e99;

    for (size_t i=1; i<samples.size(); ++i) {
        const auto& a = distances[i-1];
        const auto& b = distances[i];
        if (!a.valid || !b.valid) continue;

        const double high0 = a.signedDistance - a.radius;
        const double high1 = b.signedDistance - b.radius;
        const double low0 = a.signedDistance + a.radius;
        const double low1 = b.signedDistance + b.radius;

        const bool positiveToNegative = high0 >= 0.0 && low1 <= 0.0 && a.signedDistance >= b.signedDistance;
        const bool negativeToPositive = low0 <= 0.0 && high1 >= 0.0 && a.signedDistance <= b.signedDistance;
        if (!positiveToNegative && !negativeToPositive) continue;

        const double highCrossing = interpolateCrossing(
            samples[i-1].timeSeconds, high0,
            samples[i].timeSeconds, high1
        );
        const double lowCrossing = interpolateCrossing(
            samples[i-1].timeSeconds, low0,
            samples[i].timeSeconds, low1
        );

        const double entry = std::min(highCrossing, lowCrossing);
        const double exit = std::max(highCrossing, lowCrossing);
        const double duration = exit - entry;
        if (duration <= epsilonSeconds) continue;

        const double cutDistance =
            cutTime < entry ? entry - cutTime :
            cutTime > exit ? cutTime - exit :
            0.0;
        if (cutDistance < bestCutDistance) {
            bestCutDistance = cutDistance;
            bestDuration = duration;
        }
    }

    return bestDuration;
}

}

double planeBoxClearance(const SaberPlaneSample& sample, const OrientedBox& box) {
    return measurePlaneBox(sample, box).clearance;
}

std::optional<double> traversalTimeSeconds(
    const std::vector<SaberPlaneSample>& input,
    const OrientedBox& box,
    double cutTime,
    double epsilonSeconds
) {
    if (input.size() < 2) return std::nullopt;

    std::vector<SaberPlaneSample> samples = input;
    std::sort(samples.begin(), samples.end(), [](const auto& a, const auto& b) {
        return a.timeSeconds < b.timeSeconds;
    });

    std::vector<PlaneBoxDistance> distances(samples.size());
    std::vector<double> g(samples.size());
    for (size_t i=0;i<samples.size();++i) {
        distances[i] = measurePlaneBox(samples[i], box);
        g[i] = distances[i].clearance;
    }

    // Pick an intersecting sample closest to the actual cut event.
    std::optional<size_t> anchor;
    double bestDt = 1e99;
    for (size_t i=0;i<samples.size();++i) {
        if (g[i] <= 0.0) {
            const double dt = std::abs(samples[i].timeSeconds - cutTime);
            if (dt < bestDt) { bestDt = dt; anchor = i; }
        }
    }

    if (anchor) {
        size_t first = *anchor;
        while (first > 0 && g[first-1] <= 0.0) --first;
        size_t last = *anchor;
        while (last + 1 < samples.size() && g[last+1] <= 0.0) ++last;

        if (first > 0 && last + 1 < samples.size() && g[first-1] > 0.0 && g[last+1] > 0.0) {
            const double entry = interpolateCrossing(
                samples[first-1].timeSeconds, g[first-1],
                samples[first].timeSeconds, g[first]
            );
            const double exit = interpolateCrossing(
                samples[last].timeSeconds, g[last],
                samples[last+1].timeSeconds, g[last+1]
            );

            const double dt = exit-entry;
            if (dt > epsilonSeconds) return dt;
        }
    }

    return sparseCrossingTraversalTime(samples, distances, cutTime, epsilonSeconds);
}

} // namespace CutAccuracy
