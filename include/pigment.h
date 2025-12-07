#ifndef PIGMENT_H
#define PIGMENT_H

#include "structures.h"
#include <cmath>

// Obtem cor do pigmento em um ponto
Vec3 getPigmentColor(const Pigment &pigment, const Vec3 &point) {
  if (pigment.type == SOLID) {
    return pigment.color1;
  } else if (pigment.type == CHECKER) {
    // Padrão xadrez em 3D
    int xi = (int)floor(point.x / pigment.scale);
    int yi = (int)floor(point.y / pigment.scale);
    int zi = (int)floor(point.z / pigment.scale);

    bool even = ((xi + yi + zi) % 2) == 0;
    return even ? pigment.color1 : pigment.color2;
  } else if (pigment.type == TEXMAP) {
    // Mapeamento de textura usando as coordenadas homogêneas
    double px = point.x;
    double py = point.y;
    double pz = point.z;
    double pw = 1.0;

    // Calcula as coordenadas de textura usando combinação linear
    double s = pigment.p0[0] * px + pigment.p0[1] * py + pigment.p0[2] * pz +
               pigment.p0[3] * pw;

    double r = pigment.p1[0] * px + pigment.p1[1] * py + pigment.p1[2] * pz +
               pigment.p1[3] * pw;

    // Mantendo apenas parte fracionária das coordenadas de textura
    s = s - floor(s);
    r = r - floor(r);

    // Busca a cor na textura
    if (pigment.textureWidth > 0 && pigment.textureHeight > 0 &&
        pigment.textureData.size() > 0) {
      int u = (int)(s * pigment.textureWidth) % pigment.textureWidth;
      int v = (int)(r * pigment.textureHeight) % pigment.textureHeight;

      if (u < 0)
        u += pigment.textureWidth;
      if (v < 0)
        v += pigment.textureHeight;

      int idx = v * pigment.textureWidth + u;
      if (idx >= 0 && idx < (int)pigment.textureData.size()) {
        return pigment.textureData[idx];
      }
    }

    return pigment.color1;
  }

  return Vec3(1, 1, 1);
}

#endif
