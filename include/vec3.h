#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>

// Classe básica que representa vetores ou pontos em 3D
class Vec3 {
public:
  double x, y, z;

  Vec3() : x(0), y(0), z(0) {}
  Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

  Vec3 operator+(const Vec3 &v) const {
    return Vec3(x + v.x, y + v.y, z + v.z);
  }
  Vec3 operator-(const Vec3 &v) const {
    return Vec3(x - v.x, y - v.y, z - v.z);
  }
  Vec3 operator*(double t) const { return Vec3(x * t, y * t, z * t); }
  Vec3 operator/(double t) const { return Vec3(x / t, y / t, z / t); }
  Vec3 operator*(const Vec3 &v) const {
    return Vec3(x * v.x, y * v.y, z * v.z);
  }

  double dot(const Vec3 &v) const { return x * v.x + y * v.y + z * v.z; }

  Vec3 cross(const Vec3 &v) const {
    return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
  }

  double length() const { return sqrt(x * x + y * y + z * z); }

  Vec3 normalize() const {
    double len = length();
    if (len > 0)
      return Vec3(x / len, y / len, z / len);
    return Vec3(0, 0, 0);
  }

  Vec3 clamp(double min = 0.0, double max = 1.0) const {
    return Vec3(std::fmax(min, std::fmin(max, x)),
                std::fmax(min, std::fmin(max, y)),
                std::fmax(min, std::fmin(max, z)));
  }
};

#endif
