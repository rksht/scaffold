#pragma once

#include <scaffold/types.h>

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

    constexpr float operator[](unsigned i) const {
#if defined(_MSC_VER) || defined(__clang__)
        if (i == 0)
            return x;
        if (i == 1)
            return y;
#else
        return reinterpret_cast<const float *>(this)[i];

#endif
    }
    float &operator[](unsigned i) { return reinterpret_cast<float *>(this)[i]; }
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
    constexpr float operator[](unsigned i) const {
#if defined(_MSC_VER) || defined(__clang__)
        if (i == 0)
            return x;
        if (i == 1)
            return y;
        if (i == 2)
            return z;
#else
        return reinterpret_cast<const float *>(this)[i];

#endif
    }
    float &operator[](unsigned i) { return reinterpret_cast<float *>(this)[i]; }
};

inline constexpr Vector2::Vector2(const Vector3 &v)
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

    /// Array like accessor
    constexpr float operator[](unsigned i) const {
#if defined(_MSC_VER) || defined(__clang__)
        if (i == 0)
            return x;
        if (i == 1)
            return y;
        if (i == 2)
            return z;
        if (i == 3)
            return w;
#else
        return reinterpret_cast<const float *>(this)[i];

#endif
    }
    float &operator[](unsigned i) { return reinterpret_cast<float *>(this)[i]; }
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

struct Matrix4x4 {
    Vector4 x, y, z, t;
};

static_assert(alignof(Matrix4x4) >= 16, "");

struct AABB {
    Vector3 min, max;
};

#if 0
// This... idk, never gonna use this representation.
struct OOBB {
    Matrix4x4 tm;
    AABB aabb;
};
#endif

struct OBB {
    Vector3 center; // center
    Vector3 xyz[3]; // local x, y, z axes
    Vector3 he;     // half-extents along local axes
};

// Integer vectors

struct IVector2 {
    i32 x, y;

    IVector2() = default;
    constexpr IVector2(i32 x, i32 y)
        : x(x)
        , y(y) {}

    constexpr explicit IVector2(const Vector2 &v2)
        : x((i32)v2.x)
        , y((i32)v2.y) {}

    constexpr explicit operator Vector2() const { return Vector2{ (f32)x, (f32)y }; }
};

struct IVector3 {
    i32 x, y, z;

    IVector3() = default;
    constexpr IVector3(i32 x, i32 y, i32 z)
        : x(x)
        , y(y)
        , z(z) {}

    constexpr explicit IVector3(const Vector3 &v3)
        : x((i32)v3.x)
        , y((i32)v3.y)
        , z((i32)v3.z) {}

    constexpr explicit operator Vector3() const { return { (f32)x, (f32)y, (f32)z }; }
};

struct alignas(16) IVector4 {
    i32 x, y, z, w;

    IVector4() = default;
    IVector4(i32 x, i32 y, i32 z, i32 w)
        : x(x)
        , y(y)
        , z(z)
        , w(w) {}

    constexpr explicit IVector4(const Vector4 &v4)
        : x((i32)v4.x)
        , y((i32)v4.y)
        , z((i32)v4.z)
        , w((i32)v4.w) {}

    constexpr explicit operator Vector4() const { return { (f32)x, (f32)y, (f32)z, (f32)w }; }
};

} // namespace fo
