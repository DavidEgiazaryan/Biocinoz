#pragma once

#include <iosfwd>

struct Vec3 {
    float x{};
    float y{};
    float z{};

    Vec3() = default;
    Vec3(float x_, float y_, float z_);

    [[nodiscard]] Vec3 operator+(const Vec3& other) const;
    [[nodiscard]] Vec3 operator-(const Vec3& other) const;
    [[nodiscard]] Vec3 operator*(float scalar) const;

    Vec3& operator+=(const Vec3& other);

    [[nodiscard]] float length() const;

    [[nodiscard]] static float distance(const Vec3& a, const Vec3& b);
};

std::ostream& operator<<(std::ostream& os, const Vec3& v);
