#pragma once

#include <cmath>

namespace doomlike::domain {

struct Vec2 {
    float x {0.0F};
    float y {0.0F};

    constexpr Vec2 operator+(const Vec2& other) const noexcept { return {x + other.x, y + other.y}; }
    constexpr Vec2 operator-(const Vec2& other) const noexcept { return {x - other.x, y - other.y}; }
    constexpr Vec2 operator*(float scalar) const noexcept { return {x * scalar, y * scalar}; }

    constexpr Vec2& operator+=(const Vec2& other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }
};

inline float dot(const Vec2& lhs, const Vec2& rhs) noexcept
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

inline float lengthSquared(const Vec2& value) noexcept
{
    return dot(value, value);
}

inline float length(const Vec2& value) noexcept
{
    return std::sqrt(lengthSquared(value));
}

inline Vec2 normalize(const Vec2& value) noexcept
{
    const float len = length(value);
    if (len <= 0.0001F) {
        return {};
    }
    return {value.x / len, value.y / len};
}

inline constexpr float kPi = 3.14159265358979323846F;

inline constexpr float degToRad(const float degrees) noexcept
{
    return degrees * (kPi / 180.0F);
}

inline float wrapAngle(float radians) noexcept
{
    while (radians > kPi) {
        radians -= 2.0F * kPi;
    }
    while (radians < -kPi) {
        radians += 2.0F * kPi;
    }
    return radians;
}

inline float angleBetween(const Vec2& from, const Vec2& to) noexcept
{
    return std::atan2(to.y - from.y, to.x - from.x);
}

}  // namespace doomlike::domain

