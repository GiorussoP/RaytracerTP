#ifndef INTERSECT_H
#define INTERSECT_H

#include "structures.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

// Checa se o raio intersecta a esfera
bool intersectSphere(const Ray &ray, const Object &sphere, HitInfo &hit) {
  Vec3 oc = ray.origin - sphere.center;
  double a = ray.direction.dot(ray.direction);
  double b = 2.0 * oc.dot(ray.direction);
  double c = oc.dot(oc) - sphere.radius * sphere.radius;
  double discriminant = b * b - 4 * a * c;

  if (discriminant < 0)
    return false;

  double t1 = (-b - sqrt(discriminant)) / (2.0 * a);
  double t2 = (-b + sqrt(discriminant)) / (2.0 * a);

  double t = t1;
  if (t < 0.001)
    t = t2;
  if (t < 0.001)
    return false;

  hit.hit = true;
  hit.t = t;
  hit.point = ray.at(t);
  hit.normal = (hit.point - sphere.center).normalize();

  return true;
}

// Checa se o raio intersecta o poliedro
bool intersectPolyhedron(const Ray &ray, const Object &poly, HitInfo &hit) {
  double tNear = -std::numeric_limits<double>::infinity();
  double tFar = std::numeric_limits<double>::infinity();
  Vec3 nearNormal(0, 0, 0), farNormal(0, 0, 0);

  for (const Plane &plane : poly.faces) {
    Vec3 n = plane.normal();
    double denom = n.dot(ray.direction);
    double dist = -plane.distance(ray.origin) / denom;

    if (fabs(denom) < 1e-10) {
      // Raio paralelo ao plano
      if (plane.distance(ray.origin) > 0)
        return false;
      continue;
    }

    if (denom < 0) {
      // Entrando no semi-espaço
      if (dist > tNear) {
        tNear = dist;
        nearNormal = n;
      }
    } else {
      // Saindo do semi-espaço
      if (dist < tFar) {
        tFar = dist;
        farNormal = n * -1.0;
      }
    }

    if (tNear > tFar)
      return false;
  }

  if (tNear < 0.001) {
    tNear = tFar;
    nearNormal = farNormal;
  }

  if (tNear < 0.001 || tNear > 1e10)
    return false;

  hit.hit = true;
  hit.t = tNear;
  hit.point = ray.at(tNear);
  hit.normal = nearNormal.normalize();

  return true;
}

// Checa se o raio intersecta uma superfície quádrica
bool intersectQuadric(const Ray &ray, const Object &quad, HitInfo &hit) {
  // Raio: P(t) = O + tD
  // Quádrica: Ax^2 + By^2 + Cz^2 + Dxy + Exz + Fyz + Gx + Hy + Iz + J = 0

  // Substituindo P(t) na equação, obtemos: at^2 + bt + c = 0

  Vec3 o = ray.origin;
  Vec3 d = ray.direction;

  // Coeficiente de t^2
  double aq = quad.A * d.x * d.x + quad.B * d.y * d.y + quad.C * d.z * d.z +
              quad.D * d.x * d.y + quad.E * d.x * d.z + quad.F * d.y * d.z;

  // Coeficiente de t
  double bq = 2.0 * quad.A * o.x * d.x + 2.0 * quad.B * o.y * d.y +
              2.0 * quad.C * o.z * d.z + quad.D * (o.x * d.y + o.y * d.x) +
              quad.E * (o.x * d.z + o.z * d.x) +
              quad.F * (o.y * d.z + o.z * d.y) + quad.G * d.x + quad.H * d.y +
              quad.I * d.z;

  // Termo constante
  double cq = quad.A * o.x * o.x + quad.B * o.y * o.y + quad.C * o.z * o.z +
              quad.D * o.x * o.y + quad.E * o.x * o.z + quad.F * o.y * o.z +
              quad.G * o.x + quad.H * o.y + quad.I * o.z + quad.J;

  // Resolve equação quadrática
  double discriminant = bq * bq - 4.0 * aq * cq;

  if (discriminant < 0)
    return false;

  double sqrt_disc = sqrt(discriminant);
  double t1 = (-bq - sqrt_disc) / (2.0 * aq);
  double t2 = (-bq + sqrt_disc) / (2.0 * aq);

  double t = t1;
  if (t < 0.001)
    t = t2;
  if (t < 0.001)
    return false;

  hit.hit = true;
  hit.t = t;
  hit.point = ray.at(t);

  // Calcula normal usando o gradiente da superfície
  Vec3 p = hit.point;
  hit.normal = Vec3(2.0 * quad.A * p.x + quad.D * p.y + quad.E * p.z + quad.G,
                    2.0 * quad.B * p.y + quad.D * p.x + quad.F * p.z + quad.H,
                    2.0 * quad.C * p.z + quad.E * p.x + quad.F * p.y + quad.I)
                   .normalize();

  return true;
}

struct CSGIntersection {
  double t;
  Vec3 normal;
  int childIdx;
  bool operator<(const CSGIntersection &other) const { return t < other.t; }
};

void getAllIntersections(const Ray &ray, const Object &obj,
                         std::vector<CSGIntersection> &hits) {
  if (obj.type == SPHERE) {
    Vec3 oc = ray.origin - obj.center;
    double a = ray.direction.dot(ray.direction);
    double b = 2.0 * oc.dot(ray.direction);
    double c = oc.dot(oc) - obj.radius * obj.radius;
    double discriminant = b * b - 4 * a * c;
    if (discriminant >= 0) {
      double sqrt_disc = sqrt(discriminant);
      double t1 = (-b - sqrt_disc) / (2.0 * a);
      double t2 = (-b + sqrt_disc) / (2.0 * a);
      hits.push_back({t1, (ray.at(t1) - obj.center).normalize(), -1});
      hits.push_back({t2, (ray.at(t2) - obj.center).normalize(), -1});
    }
  } else if (obj.type == POLYHEDRON) {
    double tNear = -std::numeric_limits<double>::infinity();
    double tFar = std::numeric_limits<double>::infinity();
    Vec3 nearNormal, farNormal;
    bool hit = true;
    for (const Plane &plane : obj.faces) {
      Vec3 n = plane.normal();
      double denom = n.dot(ray.direction);
      double dist = -plane.distance(ray.origin) / denom;
      if (fabs(denom) < 1e-10) {
        if (plane.distance(ray.origin) > 0) {
          hit = false;
          break;
        }
        continue;
      }
      if (denom < 0) {
        if (dist > tNear) {
          tNear = dist;
          nearNormal = n;
        }
      } else {
        if (dist < tFar) {
          tFar = dist;
          farNormal = n;
        }
      }
    }
    if (hit && tNear <= tFar) {
      hits.push_back({tNear, nearNormal.normalize(), -1});
      hits.push_back({tFar, farNormal.normalize(), -1});
    }
  } else if (obj.type == QUADRIC) {
    Vec3 o = ray.origin;
    Vec3 d = ray.direction;
    double aq = obj.A * d.x * d.x + obj.B * d.y * d.y + obj.C * d.z * d.z +
                obj.D * d.x * d.y + obj.E * d.x * d.z + obj.F * d.y * d.z;
    double bq = 2.0 * obj.A * o.x * d.x + 2.0 * obj.B * o.y * d.y +
                2.0 * obj.C * o.z * d.z + obj.D * (o.x * d.y + o.y * d.x) +
                obj.E * (o.x * d.z + o.z * d.x) +
                obj.F * (o.y * d.z + o.z * d.y) + obj.G * d.x + obj.H * d.y +
                obj.I * d.z;
    double cq = obj.A * o.x * o.x + obj.B * o.y * o.y + obj.C * o.z * o.z +
                obj.D * o.x * o.y + obj.E * o.x * o.z + obj.F * o.y * o.z +
                obj.G * o.x + obj.H * o.y + obj.I * o.z + obj.J;
    double discriminant = bq * bq - 4.0 * aq * cq;

    if (discriminant >= 0) {
      double sqrt_disc = sqrt(discriminant);
      double t1 = (-bq - sqrt_disc) / (2.0 * aq);
      double t2 = (-bq + sqrt_disc) / (2.0 * aq);
      auto getNormal = [&](double t) {
        Vec3 p = ray.at(t);
        return Vec3(2.0 * obj.A * p.x + obj.D * p.y + obj.E * p.z + obj.G,
                    2.0 * obj.B * p.y + obj.D * p.x + obj.F * p.z + obj.H,
                    2.0 * obj.C * p.z + obj.E * p.x + obj.F * p.y + obj.I)
            .normalize();
      };
      hits.push_back({t1, getNormal(t1), -1});
      hits.push_back({t2, getNormal(t2), -1});
    }
  } else if (obj.type == CSG) {
    std::vector<CSGIntersection> allChildHits;
    for (size_t i = 0; i < obj.csgChildren.size(); ++i) {
      std::vector<CSGIntersection> currentChildHits;
      getAllIntersections(ray, obj.csgChildren[i], currentChildHits);
      for (auto &h : currentChildHits) {
        h.childIdx = (int)i;
        allChildHits.push_back(h);
      }
    }
    std::sort(allChildHits.begin(), allChildHits.end());

    std::vector<bool> inside(obj.csgChildren.size(), false);
    bool wasInside = false;
    for (const auto &hit : allChildHits) {
      inside[hit.childIdx] = !inside[hit.childIdx];
      bool inPositive = false;
      bool inNegative = false;
      for (size_t i = 0; i < obj.csgChildren.size(); ++i) {
        if (inside[i]) {
          if (obj.csgOperations[i] == CSG_UNION)
            inPositive = true;
          else if (obj.csgOperations[i] == CSG_DIFFERENCE)
            inNegative = true;
        }
      }
      bool isInside = inPositive && !inNegative;
      if (isInside != wasInside) {
        CSGIntersection newHit = hit;
        if (obj.csgOperations[hit.childIdx] == CSG_DIFFERENCE) {
          newHit.normal = hit.normal * -1.0;
        }
        hits.push_back(newHit);
        wasInside = isInside;
      }
    }
  }
}

bool intersectCSG(const Ray &ray, const Object &csg, HitInfo &hit) {
  std::vector<CSGIntersection> hits;
  getAllIntersections(ray, csg, hits);

  double closestT = std::numeric_limits<double>::infinity();
  bool found = false;

  for (const auto &h : hits) {
    if (h.t > 0.001 && h.t < closestT) {
      closestT = h.t;
      hit.t = h.t;
      hit.normal = h.normal;
      hit.point = ray.at(h.t);
      hit.hit = true;
      found = true;
    }
  }
  return found;
}

// Encontra o hit mais próximo na cena
HitInfo findClosestHit(const Ray &ray, const Scene &scene) {
  HitInfo closestHit;
  closestHit.t = std::numeric_limits<double>::infinity();

  for (size_t i = 0; i < scene.objects.size(); i++) {
    HitInfo hit;
    bool intersected = false;

    if (scene.objects[i].type == SPHERE) {
      intersected = intersectSphere(ray, scene.objects[i], hit);
    } else if (scene.objects[i].type == POLYHEDRON) {
      intersected = intersectPolyhedron(ray, scene.objects[i], hit);
    } else if (scene.objects[i].type == QUADRIC) {
      intersected = intersectQuadric(ray, scene.objects[i], hit);
    } else if (scene.objects[i].type == CSG) {
      intersected = intersectCSG(ray, scene.objects[i], hit);
    }

    if (intersected && hit.t < closestHit.t) {
      closestHit = hit;
      closestHit.objectIdx = i;
    }
  }

  return closestHit;
}

#endif
