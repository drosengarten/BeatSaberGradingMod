#pragma once

#include <algorithm>
#include <cmath>

namespace CutAccuracy {

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    constexpr Vec3() = default;
    constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
};

inline double dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}

inline double lengthSq(const Vec3& v) { return dot(v, v); }
inline double length(const Vec3& v) { return std::sqrt(lengthSq(v)); }

inline Vec3 normalized(const Vec3& v) {
    const double len = length(v);
    return len > 1e-12 ? v / len : Vec3{};
}

inline Vec3 lerp(const Vec3& a, const Vec3& b, double t) {
    return a + (b-a)*t;
}

inline bool nearlyEqual(double a, double b, double eps=1e-8) {
    return std::abs(a-b) <= eps;
}

inline bool nearlyEqual(const Vec3& a, const Vec3& b, double eps=1e-8) {
    return lengthSq(a-b) <= eps*eps;
}

struct Plane {
    // Half-space convention for clipping: dot(n, p) + d >= 0 is retained.
    Vec3 n{0.0, 1.0, 0.0};
    double d{0.0};

    static Plane throughPoint(Vec3 normal, Vec3 point) {
        normal = normalized(normal);
        return {normal, -dot(normal, point)};
    }

    double signedDistance(const Vec3& p) const {
        return dot(n, p) + d;
    }

    Plane flipped() const { return {n * -1.0, -d}; }
};

} // namespace CutAccuracy
