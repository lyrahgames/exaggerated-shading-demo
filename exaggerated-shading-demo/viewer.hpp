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
  bool waiting = false;
  bool lua_running = false;
  std::vector<std::filesystem::path> lua_live_paths{};
  sol::state lua{};

  opengl::viewport screen{};
  opengl::frame world{};
  opengl::perspective_camera cam{};

  // interaction
  struct trackball_interaction {
    vec2 from;
    opengl::frame init;
    constexpr trackball_interaction(opengl::camera_base const& camera,
                                    vec2 pos) noexcept
        : init{camera}, from{pos} {}
    constexpr void operator()(opengl::camera_base& camera, vec2 to) noexcept {
      camera.trackball(init, 2 * (to - from));
    }
  };
  struct bell_trackball_interaction {
    vec2 from;
    opengl::frame init;
    constexpr bell_trackball_interaction(opengl::camera_base const& camera,
                                         vec2 pos) noexcept
        : init{camera}, from{pos} {}
    constexpr void operator()(opengl::camera_base& camera, vec2 to) noexcept {
      camera.bell_trackball(init, from, to);
    }
  };
  std::optional<std::variant<trackball_interaction, bell_trackball_interaction>>
      trackball{};

  size_t scales = 10;
  uint32 scale = 0;

  opengl::vertex_array vertex_array{};
  opengl::const_vector<scene::vertex> vertices{};
  opengl::const_vector<scene::face> elements{};
  opengl::const_vector<vec4> normals{};

  // opengl::program_build_rule program_rule{{
  // opengl::program_target shader{{{
  //     {GL_VERTEX_SHADER, {{"exaggerated-shading-demo/vs.glsl"}}},
  //     {GL_FRAGMENT_SHADER, {{"exaggerated-shading-demo/fs.glsl"}}},
  // }}};

  opengl::build_system build{};
  std::shared_ptr<opengl::program_target> shader =
      build.target("default",
                   {
                       opengl::vs("exaggerated-shading-demo/vs.glsl"),
                       opengl::fs("exaggerated-shading-demo/fs.glsl"),
                   });

  opengl::framebuffer fbo{};
  opengl::texture2 texture{};
  opengl::renderbuffer renderbuffer{};
  vec2 texture_size{128, 128};
  std::shared_ptr<opengl::program_target> texture_shader =
      build.target("texture",
                   {
                       opengl::vs("exaggerated-shading-demo/texture.vs.glsl"),
                       opengl::fs("exaggerated-shading-demo/texture.fs.glsl"),
                   });

 public:
  viewer(uint width = 500, uint height = 500);

  void eval_lua(std::string_view str);
  void eval_lua_file(std::filesystem::path const& path);

  void show(struct scene const& scene);
  void run();

 protected:
  void init_lua();
  void watch();
  void listen(const fdm::address& domain);

  void update();
  void process_events();
  void render();
  void on_resize(int width, int height);
};

}  // namespace demo
