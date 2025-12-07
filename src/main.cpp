#include "intersect.h"
#include "loader.h"
#include "shading.h"
#include "structures.h"
#include "vec3.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <vector>

// Tamanho da janela (pode ser sobrescrito por linha de comando)
int WIDTH = 800;
int HEIGHT = 600;
int SAMPLES = 16; // Número de amostras por pixel (Distributed Ray Tracing)

// Parâmetros da câmera para Depth of Field
double APERTURE = 0.0;    // Raio da abertura da lente (0 = sem DOF)
double FOCUS_DIST = 10.0; // Distância focal

// Cena a ser renderizada
Scene scene;
std::vector<unsigned char> frameBuffer;

// Configuração da câmera
void setupCamera(const Scene &scene, Vec3 &u, Vec3 &v, Vec3 &w,
                 double &aspectRatio) {
  w = (scene.eye - scene.lookAt).normalize();
  u = scene.up.cross(w).normalize();
  v = scene.up.normalize();
  aspectRatio = (double)(WIDTH) / (double)(HEIGHT);
}

// Renderização da cena
void renderScene() {
  srand(time(NULL)); // Inicializa semente aleatória
  Vec3 u, v, w;
  double aspectRatio;
  setupCamera(scene, u, v, w, aspectRatio);

  double fovyRad = scene.fovy * M_PI / 180.0;
  double viewportHeight = 2.0 * tan(fovyRad / 2.0);
  double viewportWidth = viewportHeight * aspectRatio;

  frameBuffer.resize(WIDTH * HEIGHT * 3);

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      Vec3 pixelColor(0, 0, 0);

      // Superamostragem
      for (int s = 0; s < SAMPLES; s++) {
        // Jittering - deslocamento aleatório dentro do pixel
        double jitterX = (double)rand() / (double)RAND_MAX;
        double jitterY = (double)rand() / (double)RAND_MAX;

        // Calcula coordenadas normalizadas do dispositivo com jitter
        double ndcX = (2.0 * (x + jitterX) / WIDTH) - 1.0;
        double ndcY = 1.0 - (2.0 * (y + jitterY) / HEIGHT);

        // Calcula direção do raio (sem DOF)
        Vec3 rayDir = u * (ndcX * viewportWidth / 2.0) +
                      v * (ndcY * viewportHeight / 2.0) - w;
        rayDir = rayDir.normalize();

        // DoF - Amostra ponto aleatório no disco da abertura
        Vec3 rayOrigin = scene.eye;
        if (APERTURE > 0.0) {
          // Amostragem aleatória em disco unitário
          double dx, dy;
          do {
            dx = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            dy = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
          } while (dx * dx + dy * dy > 1.0);

          // Offset da origem do raio na abertura
          Vec3 offset = u * (dx * APERTURE) + v * (dy * APERTURE);
          rayOrigin = scene.eye + offset;

          // Ponto de foco na distância focal
          Vec3 focusPoint = scene.eye + rayDir * FOCUS_DIST;

          // Nova direção do raio da origem offset para o ponto de foco
          rayDir = (focusPoint - rayOrigin).normalize();
        }

        Ray ray(rayOrigin, rayDir);
        pixelColor = pixelColor + traceRay(ray, scene, 0);
      }

      // Média das amostras
      pixelColor = pixelColor / (double)SAMPLES;

      // Armazena a cor no buffer de quadros
      int idx = (y * WIDTH + x) * 3;
      frameBuffer[idx + 0] = (unsigned char)(pixelColor.x * 255);
      frameBuffer[idx + 1] = (unsigned char)(pixelColor.y * 255);
      frameBuffer[idx + 2] = (unsigned char)(pixelColor.z * 255);
    }
  }
}

// Salva a imagem em um arquivo PPM
bool savePPM(const std::string &filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Erro: Não foi possível criar o arquivo de saída " << filename
              << std::endl;
    return false;
  }

  file << "P3\n";
  file << "# Imagem raytracing\n";
  file << WIDTH << " " << HEIGHT << "\n";
  file << "255\n";

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      int idx = (y * WIDTH + x) * 3;
      file << (int)frameBuffer[idx + 0] << " " << (int)frameBuffer[idx + 1]
           << " " << (int)frameBuffer[idx + 2] << " ";
    }
    file << "\n";
  }

  file.close();
  return true;
}

// Função principal
int main(int argc, char **argv) {
  // Ler argumentos da linha de comando
  if (argc < 3) {
    std::cerr << "Uso: " << argv[0]
              << " <input_scene.in> <output_image.ppm> [width] [height] "
                 "[aperture] [focus_dist]"
              << std::endl;
    std::cerr << "  input_scene.in  - Arquivo de cena de entrada" << std::endl;
    std::cerr << "  output_image.ppm - Arquivo de imagem PPM de saída"
              << std::endl;
    std::cerr << "  width           - Largura da imagem (opcional, padrão: 800)"
              << std::endl;
    std::cerr << "  height          - Altura da imagem (opcional, padrão: 600)"
              << std::endl;
    std::cerr << "  aperture        - Abertura da lente (opcional, padrão: 0.0)"
              << std::endl;
    std::cerr << "  focus_dist      - Distância focal (opcional, padrão: 10.0)"
              << std::endl;
    return 1;
  }

  std::string inputFile = argv[1];
  std::string outputFile = argv[2];

  // Ler largura e altura opcionais
  if (argc >= 4) {
    WIDTH = std::atoi(argv[3]);
    if (WIDTH <= 0) {
      std::cerr << "Erro: Valor inválido para largura" << std::endl;
      return 1;
    }
  }

  if (argc >= 5) {
    HEIGHT = std::atoi(argv[4]);
    if (HEIGHT <= 0) {
      std::cerr << "Erro: Valor inválido para altura" << std::endl;
      return 1;
    }
  }

  if (argc >= 6) {
    APERTURE = std::atof(argv[5]);
    if (APERTURE < 0) {
      std::cerr << "Erro: Valor inválido para abertura" << std::endl;
      return 1;
    }
  }

  if (argc >= 7) {
    FOCUS_DIST = std::atof(argv[6]);
    if (FOCUS_DIST <= 0) {
      std::cerr << "Erro: Valor inválido para distância focal" << std::endl;
      return 1;
    }
  }

  std::cout << "=== Ray Tracer - TP2 ===" << std::endl;
  std::cout << "Arquivo de entrada: " << inputFile << std::endl;
  std::cout << "Arquivo de saída: " << outputFile << std::endl;
  std::cout << "Resolução: " << WIDTH << "x" << HEIGHT << std::endl;
  std::cout << "Abertura: " << APERTURE << std::endl;
  std::cout << "Distância focal: " << FOCUS_DIST << std::endl;
  std::cout << std::endl;

  std::cout << "Carregando cena de " << inputFile << "..." << std::endl;
  if (!loadScene(inputFile, scene)) {
    std::cerr << "Falha ao carregar a cena!" << std::endl;
    return 1;
  }

  std::cout << "Cena carregada com sucesso!" << std::endl;
  std::cout << "  Luzes: " << scene.lights.size() << std::endl;
  std::cout << "  Pigmentos: " << scene.pigments.size() << std::endl;
  std::cout << "  Acabamentos: " << scene.finishes.size() << std::endl;
  std::cout << "  Objetos: " << scene.objects.size() << std::endl;
  std::cout << std::endl;

  std::cout << "Renderizando cena..." << std::endl;
  renderScene();
  std::cout << "Salvando imagem em " << outputFile << "..." << std::endl;
  if (!savePPM(outputFile)) {
    std::cerr << "Falha ao salvar a imagem!" << std::endl;
    return 1;
  }
  std::cout << "Imagem salva." << std::endl;
  std::cout << std::endl;

  return 0;
}
