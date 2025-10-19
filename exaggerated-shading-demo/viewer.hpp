#pragma once
#include <SFML/Graphics.hpp>
//
#include "camera.hpp"
#include "defaults.hpp"
#include "scene.hpp"

namespace demo {

struct opengl_window {
  sf::RenderWindow window{};
  opengl_window(uint width, uint height);
};

class viewer : public opengl_window {
  bool done = false;

  // World Origin
  vec3 origin;
  // Basis Vectors of Right-Handed Coordinate System
  vec3 up{0, 1, 0};
  vec3 right{1, 0, 0};
  vec3 front{0, 0, 1};
  // Spherical/Horizontal Coordinates of Camera
  float radius = 10;
  float altitude = 0;
  float azimuth = 0;
  // Perspective camera
  struct camera camera{};

  struct scene scene{};

  GLuint vertex_array{};
  GLuint vertex_buffer{};
  GLuint shader{};

 public:
  viewer(uint width = 500, uint height = 500);
  virtual ~viewer();

  void run();

  void load_stl_surface(const filesystem::path& path);

 protected:
  void render();
};

}  // namespace demo
