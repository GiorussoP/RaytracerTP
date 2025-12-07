#ifndef SHADING_H
#define SHADING_H

#include "intersect.h"
#include "pigment.h"
#include "structures.h"
#include <algorithm>
#include <cstdlib> // Para a função rand()

const int MAX_DEPTH = 5; // Profundidade máxima de recursão

Vec3 traceRay(const Ray &ray, const Scene &scene, int depth);

// Calcula o raio refletido
Ray reflect(const Ray &ray, const Vec3 &point, const Vec3 &normal) {
  Vec3 reflectDir = ray.direction - normal * 2.0 * ray.direction.dot(normal);
  return Ray(point + normal * 0.001, reflectDir);
}

// Calcula o raio refratado usando a lei de Snell
bool refract(const Ray &ray, const Vec3 &normal, double ior, Vec3 &refracted) {
  Vec3 I = ray.direction.normalize();
  Vec3 N = normal.normalize();
  double cosi = I.dot(N);

  double etai = 1.0; // Ar
  double etat = ior; // Material
  Vec3 n = N;

  // Entrando ou saindo?
  if (cosi < 0) {
    // Entrando
    cosi = -cosi;
  } else {
    // Saindo
    std::swap(etai, etat);
    n = N * -1.0;
  }

  double eta = etai / etat;
  double k = 1.0 - eta * eta * (1.0 - cosi * cosi);

  // Reflexão total interna
  if (k < 0) {
    return false;
  }

  refracted = (I * eta + n * (eta * cosi - sqrt(k))).normalize();
  return true;
}

// Calcula a cor de um ponto usando o modelo de iluminação Phong
Vec3 shade(const HitInfo &hit, const Scene &scene, const Ray &ray, int depth) {
  const Object &obj = scene.objects[hit.objectIdx];
  const Pigment &pigment = scene.pigments[obj.pigmentIdx];
  const Finish &finish = scene.finishes[obj.finishIdx];

  // Obtém a cor base do pigmento
  Vec3 baseColor = getPigmentColor(pigment, hit.point);

  // Componente ambiente (primeira luz fornece a cor ambiente)
  Vec3 color = baseColor * scene.lights[0].color * finish.ka;

  // Itera por todas as luzes para componentes difusa e especular
  for (size_t i = 1; i < scene.lights.size(); i++) {
    const Light &light = scene.lights[i];

    Vec3 lightDir = (light.position - hit.point).normalize();
    double lightDist = (light.position - hit.point).length();

    // Raio sombra com offset baseado no ângulo
    double bias = 0.001;
    double cosAngle = fabs(hit.normal.dot(lightDir));

    // Aumenta bias para ângulos rasantes
    if (cosAngle < 0.1)
      bias = 0.01;

    Vec3 shadowOrigin = hit.point + hit.normal * bias;

    // Sombras suaves via amostragem de área de luz
    double lightRadius = 0.5;

    // Amostra aleatória única
    double r1 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    double r2 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    double r3 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    Vec3 offset(r1, r2, r3);

    Vec3 samplePos = light.position + offset * lightRadius;
    Vec3 shadowLightDir = (samplePos - shadowOrigin).normalize();
    double shadowLightDist = (samplePos - shadowOrigin).length();

    Ray shadowRay(shadowOrigin, shadowLightDir);
    HitInfo shadowHit = findClosestHit(shadowRay, scene);

    bool inShadow = shadowHit.hit && shadowHit.t < shadowLightDist - 1e-4;

    if (!inShadow) {
      // Não está em sombra, logo, recebe luz difusa e especular

      // Atenuação da luz
      double denominator = light.attenuation.x +
                           light.attenuation.y * lightDist +
                           light.attenuation.z * lightDist * lightDist;

      double attenuation = 1.0 / denominator;

      // Componente difusa
      double diff = std::max(0.0, hit.normal.dot(lightDir));
      Vec3 diffuse = baseColor * light.color * finish.kd * diff * attenuation;

      // Componente especular
      Vec3 viewDir = (ray.origin - hit.point).normalize();
      Vec3 halfVec = (lightDir + viewDir).normalize();
      double spec = pow(std::max(0.0, hit.normal.dot(halfVec)), finish.alpha);
      Vec3 specular = light.color * finish.ks * spec * attenuation;
      color = color + diffuse + specular;
    }
  }

  // Reflexão
  if (finish.kr > 0 && depth < MAX_DEPTH) {
    Ray reflectedRay = reflect(ray, hit.point, hit.normal);

    // Reflexão glossy (Ray Tracing Distribuído)
    // Perturbação baseada na rugosidade (inverso do expoente especular)
    double roughness = (finish.alpha > 1e-3) ? (1.0 / finish.alpha) : 1.0;

    double r1 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    double r2 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    double r3 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
    Vec3 jitter(r1, r2, r3);

    Vec3 perturbedDir =
        (reflectedRay.direction + jitter * roughness).normalize();

    // Garante que o raio refletido não entre no objeto
    if (perturbedDir.dot(hit.normal) < 0) {
      perturbedDir = reflectedRay.direction;
    }

    reflectedRay.direction = perturbedDir;

    Vec3 reflectedColor = traceRay(reflectedRay, scene, depth + 1);
    color = color + reflectedColor * finish.kr;
  }

  // Refração/Transmissão
  if (finish.kt > 0 && depth < MAX_DEPTH) {
    Vec3 refractedDir;
    if (refract(ray, hit.normal, finish.ior, refractedDir)) {
      // Offset na direção da refração
      Vec3 offset = refractedDir * 0.001;
      Ray refractedRay(hit.point + offset, refractedDir);

      // Refração glossy (Ray Tracing Distribuído)
      double roughness = (finish.alpha > 1e-3) ? (5.0 / finish.alpha) : 1.0;
      double r1 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
      double r2 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
      double r3 = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
      Vec3 jitter(r1, r2, r3);

      refractedRay.direction =
          (refractedRay.direction + jitter * roughness).normalize();

      Vec3 refractedColor = traceRay(refractedRay, scene, depth + 1);
      color = color + refractedColor * finish.kt;
    }
  }

  return color.clamp();
}

// Traça um raio na cena
Vec3 traceRay(const Ray &ray, const Scene &scene, int depth) {
  if (depth > MAX_DEPTH) {
    return Vec3(0, 0, 0);
  }

  HitInfo hit = findClosestHit(ray, scene);

  if (hit.hit) {
    return shade(hit, scene, ray, depth);
  }

  // Cor de fundo - preto, não encontrou nada
  return Vec3(0, 0, 0);
}

#endif
