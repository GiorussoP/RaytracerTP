#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "vec3.h"
#include <cmath>
#include <string>
#include <vector>

// Raio para o raytracing
struct Ray {
  Vec3 origin;
  Vec3 direction;

  Ray(const Vec3 &o, const Vec3 &d) : origin(o), direction(d.normalize()) {}

  // Parametrização do raio
  Vec3 at(double t) const { return origin + direction * t; }
};

// Tipos de pigmento
enum PigmentType { SOLID, CHECKER, TEXMAP };

struct Pigment {
  PigmentType type;
  Vec3 color1, color2; // Cores para sólido e xadrez
  double scale;        // Escala para padrão xadrez

  // Para mapeamento de textura
  std::string texturePath;
  double p0[4], p1[4];
  std::vector<Vec3> textureData;
  int textureWidth, textureHeight;

  Pigment()
      : type(SOLID), color1(1, 1, 1), color2(0, 0, 0), scale(1.0),
        textureWidth(0), textureHeight(0) {
    for (int i = 0; i < 4; i++)
      p0[i] = p1[i] = 0;
  }
};

// Acabamento dos objetos
struct Finish {
  double ka;    // Coeficiente ambiente
  double kd;    // Coeficiente difuso
  double ks;    // Coeficiente especular
  double alpha; // Expoente especular
  double kr;    // Coeficiente de reflexão
  double kt;    // Coeficiente de transmissão
  double ior;   // Índice de refração (n1/n2)

  Finish() : ka(0), kd(0), ks(0), alpha(1), kr(0), kt(0), ior(1) {}
};

// Plano para as faces do poliedro
struct Plane {
  double a, b, c, d; // ax + by + cz + d = 0

  Plane(double a, double b, double c, double d) {
    double len = std::sqrt(a * a + b * b + c * c);
    if (len > 0) {
      this->a = a / len;
      this->b = b / len;
      this->c = c / len;
      this->d = d / len;
    } else {
      this->a = a;
      this->b = b;
      this->c = c;
      this->d = d;
    }
  }

  Vec3 normal() const { return Vec3(a, b, c); }

  double distance(const Vec3 &p) const {
    return a * p.x + b * p.y + c * p.z + d;
  }
};

// Tipos de objeto
enum ObjectType { SPHERE, POLYHEDRON, QUADRIC, CSG };
enum CSGOperation { CSG_UNION, CSG_DIFFERENCE };

struct Object {
  ObjectType type;
  int pigmentIdx;
  int finishIdx;

  // Esfera
  Vec3 center;
  double radius;

  // Poliedro
  std::vector<Plane> faces;

  // Quádricas
  // Equação: Ax^2 + By^2 + Cz^2 + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0
  double A, B, C; // x^2, y^2, z^2
  double D, E, F; // xy, xz, yz
  double G, H, I; // x, y, z
  double J;       // constante

  // CSG
  std::vector<Object> csgChildren;
  std::vector<CSGOperation> csgOperations;

  Object()
      : type(SPHERE), pigmentIdx(0), finishIdx(0), radius(0), A(0), B(0), C(0),
        D(0), E(0), F(0), G(0), H(0), I(0), J(0) {}
};

// Luz
struct Light {
  Vec3 position;
  Vec3 color;
  Vec3 attenuation; // constant, linear, quadratic

  Light(const Vec3 &pos, const Vec3 &col, const Vec3 &atten)
      : position(pos), color(col), attenuation(atten) {}
};

// Estrutura da cena
struct Scene {
  Vec3 eye;
  Vec3 lookAt;
  Vec3 up;
  double fovy;

  std::vector<Light> lights;
  std::vector<Pigment> pigments;
  std::vector<Finish> finishes;
  std::vector<Object> objects;

  Scene() : eye(0, 0, 0), lookAt(0, 0, -1), up(0, 1, 0), fovy(40) {}
};

// Informação de onde o raio atingiu um objeto
struct HitInfo {
  bool hit;
  double t;
  Vec3 point;
  Vec3 normal;
  int objectIdx;

  HitInfo() : hit(false), t(1e10), objectIdx(-1) {}
};

#endif
