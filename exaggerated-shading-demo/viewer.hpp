#pragma once
#include <SFML/Graphics.hpp>
//
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
//
#include "camera.hpp"
#include "defaults.hpp"
#include "fdm.hpp"
#include "scene.hpp"

namespace demo {

struct opengl_window {
  sf::Window window{};
  opengl_window(uint width, uint height);
};

class viewer : public opengl_window {
  bool done = false;
  std::vector<std::filesystem::path> paths{};
  sol::state lua{};

  sf::Vector2i mouse_pos{};
  std::optional<sf::Vector2i> trackball_start{};

  // Needs to be refactored:
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
  struct camera backup_cam{};

  opengl::viewport screen{};
  // opengl::coordinate_system world{};
  opengl::perspective_camera pcam{};
  opengl::orthographic_camera ocam{};
  opengl::free_spherical_observer camui{};

  float bounding_radius = 1.0f;
  bool view_should_update = true;

  struct scene scene{};
  size_t scales = 10;
  uint32 scale = 0;

  opengl::vertex_array vertex_array{};
  // opengl::buffer vertex_buffer{};
  // opengl::buffer element_buffer{};
  opengl::buffer normals_buffer{};
  // opengl::program shader{};

  opengl::vector<scene::vertex> vertices{};
  opengl::vector<scene::face> elements{};

  // opengl::program_build_rule program_rule{{
  // opengl::program_target shader{{{
  //     {GL_VERTEX_SHADER, {{"exaggerated-shading-demo/vs.glsl"}}},
  //     {GL_FRAGMENT_SHADER, {{"exaggerated-shading-demo/fs.glsl"}}},
  // }}};

  opengl::build_system build{};
  std::shared_ptr<opengl::program_target> shader =
      build.target("default",
                   {opengl::vs("exaggerated-shading-demo/vs.glsl"),
                    opengl::fs("exaggerated-shading-demo/fs.glsl")});

 public:
  viewer(uint width = 500, uint height = 500);

  void run();

  void add_path(std::filesystem::path const& path);

  void load_scene(std::filesystem::path const& path);
  void fit_view_to_surface();

  void turn(const vec2& angle);
  void shift(const vec2& pixels);
  void zoom(float scale);

  void trackball(vec2 x, vec2 y);

 protected:
  void init_lua();
  void listen(const fdm::address& domain);
  void watch();

  void render();
  void on_resize(int width, int height);
  void update_view();

  void build_shader();
  void update_shader();
};

}  // namespace demo
