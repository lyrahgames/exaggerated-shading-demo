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

  sf::Vector2i mouse_pos{};

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

  float bounding_radius = 1.0f;
  bool view_should_update = true;

  struct scene scene{};

  GLuint vertex_array{};
  GLuint vertex_buffer{};
  GLuint shader{};

 public:
  viewer(uint width = 500, uint height = 500);
  virtual ~viewer();

  void run();

  void load_stl_surface(const filesystem::path& path);
  void fit_view_to_surface();

  void turn(const vec2& angle);
  void shift(const vec2& pixels);
  void zoom(float scale);

 protected:
  void render();
  void on_resize(int width, int height);
  void update_view();
};

}  // namespace demo
