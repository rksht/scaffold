#pragma once

#include <scaffold/types.h>

#include <emmintrin.h>

namespace fo {

struct Vector2;
struct Vector3;
struct Vector4;

struct Vector2 {
    float x, y;

    Vector2() = default;

    constexpr Vector2(float x, float y)
        : x(x)
        , y(y) {}

    constexpr Vector2(const Vector3 &v);
};

struct Vector4;

struct Vector3 {
    float x, y, z;

    Vector3() = default;

    Vector3(const Vector2 &v, float z)
        : x(v.x)
        , y(v.y)
        , z(z) {}

    /// Constructs a Vector3 from a Vector4 by simply ignoring the w coordinate. A bit of implicit behavior
    /// here, but let' see how it goes.
    constexpr Vector3(const Vector4 &v);

    constexpr Vector3(float x, float y, float z)
        : x(x)
        , y(y)
        , z(z) {}

    /// Array like accessor
    constexpr float operator[](unsigned i) const { return reinterpret_cast<const float *>(this)[i]; }
    constexpr float &operator[](unsigned i) { return reinterpret_cast<float *>(this)[i]; }
};

inline Vector2::Vector2(const Vector3 &v)
    : x(v.x)
    , y(v.y) {}

struct alignas(16) Vector4 {
    float x, y, z, w;

    Vector4() = default;

    // Yep. Gotta give the w explicitly.
    constexpr Vector4(const Vector3 &v, float w)
        : x(v.x)
        , y(v.y)
        , z(v.z)
        , w(w) {}

    constexpr Vector4(float x, float y, float z, float w)
        : x(x)
        , y(y)
        , z(z)
        , w(w) {}

    Vector4(__m128 pack) { _mm_store_ps(reinterpret_cast<float *>(this), pack); }

    __m128 get_xmm() const { return _mm_load_ps(reinterpret_cast<const float *>(this)); }

    /// Array like accessor
    constexpr float operator[](unsigned i) const { return reinterpret_cast<const float *>(this)[i]; }
    constexpr float &operator[](unsigned i) { return reinterpret_cast<float *>(this)[i]; }
};

static_assert(sizeof(Vector4) == 4 * sizeof(float), "");

inline constexpr Vector3::Vector3(const Vector4 &v)
    : x(v.x)
    , y(v.y)
    , z(v.z) {}

struct alignas(16) Quaternion {
    float x, y, z, w;
};

struct Matrix3x3 {
    Vector3 x, y, z;
};

struct alignas(16) Matrix3x3_aligned16 {
    Vector3 x, y, z;
    Vector3 _pad;
};

struct alignas(16) Matrix4x4 {
    Vector4 x, y, z, t;
};

using Matrix4x4_aligned16 = Matrix4x4;

struct AABB {
    Vector3 min, max;
};

struct OOBB {
    Matrix4x4 tm;
    AABB aabb;
};
} // namespace fo
