#pragma once

#include "CutAccuracy/Math.hpp"
#include "CutAccuracy/Traversal.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"

#include <cmath>

namespace CutAccuracyQuest {

inline CutAccuracy::Vec3 ToCore(UnityEngine::Vector3 v) {
    return {v.x, v.y, v.z};
}

inline UnityEngine::Vector3 ToUnity(const CutAccuracy::Vec3& v) {
    return {
        static_cast<float>(v.x),
        static_cast<float>(v.y),
        static_cast<float>(v.z)
    };
}

inline CutAccuracy::Vec3 ProjectNotePlane(CutAccuracy::Vec3 v) {
    v.z = 0.0;
    return CutAccuracy::normalized(v);
}

inline CutAccuracy::OrientedBox FreezeBox(UnityEngine::Transform* t) {
    // The current model assumes the note mesh occupies a unit local cube. Keep this isolated
    // so collider-derived extents can replace it after Quest telemetry confirms the live size.
    constexpr double kLocalHalfExtent = 0.5;
    const auto scale = t->get_lossyScale();
    return {
        ToCore(t->get_position()),
        CutAccuracy::normalized(ToCore(t->get_right())),
        CutAccuracy::normalized(ToCore(t->get_up())),
        CutAccuracy::normalized(ToCore(t->get_forward())),
        {std::abs(scale.x) * kLocalHalfExtent,
         std::abs(scale.y) * kLocalHalfExtent,
         std::abs(scale.z) * kLocalHalfExtent}
    };
}

inline CutAccuracy::Plane WorldPlaneToLocal(
    UnityEngine::Transform* noteTransform,
    UnityEngine::Vector3 worldPoint,
    UnityEngine::Vector3 worldNormal
) {
    const auto localPoint = noteTransform->InverseTransformPoint(worldPoint);
    // Beat Saber notes are normally uniformly scaled. InverseTransformDirection
    // gives the local plane normal without adding translation.
    const auto localNormal = noteTransform->InverseTransformDirection(worldNormal);
    return CutAccuracy::Plane::throughPoint(ToCore(localNormal), ToCore(localPoint));
}

inline CutAccuracy::Vec3 WorldDirectionToLocalNotePlane(
    UnityEngine::Transform* noteTransform,
    const CutAccuracy::Vec3& worldDirection
) {
    if (!noteTransform) return {0,0,0};
    return ProjectNotePlane(ToCore(noteTransform->InverseTransformDirection(ToUnity(worldDirection))));
}

inline CutAccuracy::Vec3 LocalNoteUpWorld(UnityEngine::Transform* noteTransform) {
    if (!noteTransform) return {0,0,0};
    return ProjectNotePlane(ToCore(noteTransform->TransformDirection({0.0f, 1.0f, 0.0f})));
}

inline CutAccuracy::Vec3 LocalDirectionToNotePlane(UnityEngine::Transform* noteTransform, UnityEngine::Vector3 worldDirection) {
    if (!noteTransform) return {0,0,0};
    return ProjectNotePlane(ToCore(noteTransform->InverseTransformDirection(worldDirection)));
}

} // namespace CutAccuracyQuest
