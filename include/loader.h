#ifndef LOADER_H
#define LOADER_H

#include "structures.h"
#include <fstream>
#include <iostream>
#include <sstream>

// Carrega textura PPM
bool loadPPM(const std::string &filename, Pigment &pigment) {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Erro: Não foi possível abrir o arquivo de textura "
              << filename << std::endl;
    return false;
  }

  std::string magic;
  file >> magic;

  if (magic != "P3" && magic != "P6") {
    std::cerr << "Erro: Formato PPM não suportado " << magic << std::endl;
    return false;
  }

  // Pula comentários
  char c;
  file.get(c);
  while (file && (c == '#' || c == '\n')) {
    if (c == '#') {
      while (file.get(c) && c != '\n')
        ;
    }
    if (!file.get(c))
      break;
  }
  if (file)
    file.unget();

  int maxval;
  file >> pigment.textureWidth >> pigment.textureHeight >> maxval;

  pigment.textureData.resize(pigment.textureWidth * pigment.textureHeight);

  if (magic == "P3") {
    // ASCII
    for (int i = 0; i < pigment.textureWidth * pigment.textureHeight; i++) {
      int r, g, b;
      file >> r >> g >> b;
      pigment.textureData[i] =
          Vec3(r / (double)maxval, g / (double)maxval, b / (double)maxval);
    }
  } else {
    // Formato binário
    file.get(); // Pula newline
    for (int i = 0; i < pigment.textureWidth * pigment.textureHeight; i++) {
      unsigned char rgb[3];
      file.read((char *)rgb, 3);
      pigment.textureData[i] =
          Vec3(rgb[0] / (double)maxval, rgb[1] / (double)maxval,
               rgb[2] / (double)maxval);
    }
  }

  file.close();
  return true;
}

// Helper para ler objetos recursivamente (por exemplo, o CSG)
bool parseObject(std::ifstream &file, Object &obj) {
  file >> obj.pigmentIdx >> obj.finishIdx;

  std::string objType;
  file >> objType;

  if (objType == "sphere") {
    obj.type = SPHERE;
    file >> obj.center.x >> obj.center.y >> obj.center.z >> obj.radius;
  } else if (objType == "polyhedron") {
    obj.type = POLYHEDRON;
    int numFaces;
    file >> numFaces;
    for (int j = 0; j < numFaces; j++) {
      double a, b, c, d;
      file >> a >> b >> c >> d;
      obj.faces.push_back(Plane(a, b, c, d));
    }
  } else if (objType == "quadric") {
    obj.type = QUADRIC;
    file >> obj.A >> obj.B >> obj.C;
    file >> obj.D >> obj.E >> obj.F;
    file >> obj.G >> obj.H >> obj.I >> obj.J;
  } else if (objType == "csg") {
    obj.type = CSG;
    int numChildren;
    file >> numChildren;
    for (int j = 0; j < numChildren; j++) {
      std::string opStr;
      file >> opStr;
      if (opStr == "+")
        obj.csgOperations.push_back(CSG_UNION);
      else if (opStr == "-")
        obj.csgOperations.push_back(CSG_DIFFERENCE);

      Object child;
      parseObject(file, child);
      obj.csgChildren.push_back(child);
    }
  }
  return true;
}

// Carrega cena do arquivo
bool loadScene(const std::string &filename, Scene &scene) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Erro: Não foi possível abrir o arquivo de cena " << filename
              << std::endl;
    return false;
  }

  // 1 - Configuração da câmera
  file >> scene.eye.x >> scene.eye.y >> scene.eye.z;
  file >> scene.lookAt.x >> scene.lookAt.y >> scene.lookAt.z;
  file >> scene.up.x >> scene.up.y >> scene.up.z;
  file >> scene.fovy;

  // 2 - Luzes
  int numLights;
  file >> numLights;
  for (int i = 0; i < numLights; i++) {
    Vec3 pos, color, atten;
    file >> pos.x >> pos.y >> pos.z;
    file >> color.x >> color.y >> color.z;
    file >> atten.x >> atten.y >> atten.z;
    scene.lights.push_back(Light(pos, color, atten));
  }

  // 3 - Pigmentos
  int numPigments;
  file >> numPigments;
  for (int i = 0; i < numPigments; i++) {
    Pigment pig;
    std::string type;
    file >> type;

    if (type == "solid") {
      pig.type = SOLID;
      file >> pig.color1.x >> pig.color1.y >> pig.color1.z;
    } else if (type == "checker") {
      pig.type = CHECKER;
      file >> pig.color1.x >> pig.color1.y >> pig.color1.z;
      file >> pig.color2.x >> pig.color2.y >> pig.color2.z;
      file >> pig.scale;
    } else if (type == "texmap") {
      pig.type = TEXMAP;
      file >> pig.texturePath;
      file >> pig.p0[0] >> pig.p0[1] >> pig.p0[2] >> pig.p0[3];
      file >> pig.p1[0] >> pig.p1[1] >> pig.p1[2] >> pig.p1[3];

      // Carrega textura
      if (!loadPPM(pig.texturePath, pig)) {
        std::cerr << "Warning: Could not load texture " << pig.texturePath
                  << std::endl;
      }
    }

    scene.pigments.push_back(pig);
  }

  // 4 - Acabamentos
  int numFinishes;
  file >> numFinishes;
  for (int i = 0; i < numFinishes; i++) {
    Finish finish;
    file >> finish.ka >> finish.kd >> finish.ks >> finish.alpha;
    file >> finish.kr >> finish.kt >> finish.ior;
    scene.finishes.push_back(finish);
  }

  // 5 - Objetos
  int numObjects;
  file >> numObjects;
  for (int i = 0; i < numObjects; i++) {
    Object obj;
    parseObject(file, obj);
    scene.objects.push_back(obj);
  }

  file.close();
  return true;
}

#endif
