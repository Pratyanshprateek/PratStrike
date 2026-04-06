#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

namespace math
{
    constexpr float PI = 3.14159265358979323846f;
    constexpr float EPSILON = 0.00001f;
}

inline float clamp(float v, float lo, float hi)
{
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

inline float lerpF(float a, float b, float t)
{
    return a + (b - a) * clamp(t, 0.0f, 1.0f);
}

inline float degToRad(float degrees)
{
    return degrees * (math::PI / 180.0f);
}

inline float radToDeg(float radians)
{
    return radians * (180.0f / math::PI);
}

struct Vector2
{
    float x;
    float y;

    constexpr Vector2() noexcept
        : x(0.0f), y(0.0f)
    {
    }

    constexpr Vector2(float xValue, float yValue) noexcept
        : x(xValue), y(yValue)
    {
    }

    constexpr Vector2 operator+(const Vector2& other) const noexcept
    {
        return Vector2(x + other.x, y + other.y);
    }

    constexpr Vector2 operator-(const Vector2& other) const noexcept
    {
        return Vector2(x - other.x, y - other.y);
    }

    constexpr Vector2 operator*(float scalar) const noexcept
    {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2 operator/(float scalar) const noexcept
    {
        return (std::fabs(scalar) <= math::EPSILON) ? Vector2() : Vector2(x / scalar, y / scalar);
    }

    constexpr Vector2& operator+=(const Vector2& other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vector2& operator-=(const Vector2& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vector2& operator*=(float scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vector2& operator/=(float scalar) noexcept
    {
        if (std::fabs(scalar) <= math::EPSILON)
        {
            x = 0.0f;
            y = 0.0f;
            return *this;
        }

        x /= scalar;
        y /= scalar;
        return *this;
    }

    bool operator==(const Vector2& other) const noexcept
    {
        return std::fabs(x - other.x) <= math::EPSILON && std::fabs(y - other.y) <= math::EPSILON;
    }

    bool operator!=(const Vector2& other) const noexcept
    {
        return !(*this == other);
    }

    float length() const noexcept
    {
        return std::sqrt(lengthSq());
    }

    float lengthSq() const noexcept
    {
        return (x * x) + (y * y);
    }

    Vector2 normalized() const noexcept
    {
        const float len = length();
        return (len <= math::EPSILON) ? Vector2() : (*this / len);
    }

    float dot(const Vector2& other) const noexcept
    {
        return (x * other.x) + (y * other.y);
    }

    float distance(const Vector2& other) const noexcept
    {
        return (*this - other).length();
    }

    Vector2 lerp(const Vector2& target, float t) const noexcept
    {
        return Vector2(lerpF(x, target.x, t), lerpF(y, target.y, t));
    }

    Vector2 rotated(float degrees) const noexcept
    {
        const float radians = degToRad(degrees);
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        return Vector2(
            (x * cosine) - (y * sine),
            (x * sine) + (y * cosine)
        );
    }

    float angle() const noexcept
    {
        return radToDeg(std::atan2(x, -y));
    }
};

inline constexpr Vector2 operator*(float scalar, const Vector2& value) noexcept
{
    return value * scalar;
}

struct Rect
{
    float x;
    float y;
    float w;
    float h;

    constexpr Rect() noexcept
        : x(0.0f), y(0.0f), w(0.0f), h(0.0f)
    {
    }

    constexpr Rect(float xValue, float yValue, float widthValue, float heightValue) noexcept
        : x(xValue), y(yValue), w(widthValue), h(heightValue)
    {
    }

    bool intersects(const Rect& other) const noexcept
    {
        return x < (other.x + other.w) &&
               (x + w) > other.x &&
               y < (other.y + other.h) &&
               (y + h) > other.y;
    }

    bool contains(const Vector2& point) const noexcept
    {
        return point.x >= x &&
               point.x <= (x + w) &&
               point.y >= y &&
               point.y <= (y + h);
    }

    Vector2 center() const noexcept
    {
        return Vector2(x + (w * 0.5f), y + (h * 0.5f));
    }

    Rect expanded(float margin) const noexcept
    {
        return Rect(x - margin, y - margin, w + (margin * 2.0f), h + (margin * 2.0f));
    }
};

inline int randomInt(int lo, int hi)
{
    static thread_local std::mt19937 generator(std::random_device{}());
    if (lo > hi)
    {
        const int temp = lo;
        lo = hi;
        hi = temp;
    }

    std::uniform_int_distribution<int> distribution(lo, hi);
    return distribution(generator);
}

inline float randomFloat(float lo, float hi)
{
    static thread_local std::mt19937 generator(std::random_device{}());
    if (lo > hi)
    {
        const float temp = lo;
        lo = hi;
        hi = temp;
    }

    std::uniform_real_distribution<float> distribution(lo, hi);
    return distribution(generator);
}
