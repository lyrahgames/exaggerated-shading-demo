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
//
#include "basic_update_list.hpp"

namespace demo {

struct opengl_window {
  sf::Window window{};
  opengl_window(uint width, uint height);
};

struct update_manager {
  using updater = std::function<void()>;
  std::map<std::string, updater> updates{};
  constexpr void operator()() {
    for (auto& [name, update] : updates) std::invoke(update);
  }
  constexpr void set(std::string_view name, auto&& f) {
    updates[std::string{name}] = std::forward<decltype(f)>(f);
  }
  constexpr void set(auto&& f) { set("", std::forward<decltype(f)>(f)); }
};

static_assert(std::copyable<update_manager>);

struct viewer_state {
  basic_update_list<sf::Event const&> actions{};
  basic_update_list<> updates{};

  opengl::frame world{};
  opengl::perspective_camera camera{};
};

static_assert(std::copyable<viewer_state>);

class viewer : public opengl_window {
  bool done = false;
  bool waiting = false;
  int lua_level = 0;
  std::vector<std::filesystem::path> lua_live_paths{};
  sol::state lua{};

  viewer_state init{};
  viewer_state current{};
  using modifier = std::function<void(viewer_state&)>;
  std::list<modifier> modifiers{};

  opengl::viewport screen{};
  opengl::frame world{};
  opengl::perspective_camera cam{};
  opengl::perspective_camera undo_cam{};
  opengl::perspective_camera render_cam{};
  std::optional<vec2> cam_interpolation{};
  float cam_interpolate = 0.0f;

  struct camera_switch_animation {
    using time_point =
        std::chrono::time_point<std::chrono::high_resolution_clock>;
    time_point start;
    float duration;
    opengl::perspective_camera first;
    opengl::perspective_camera last;
    opengl::perspective_camera* camera;

    constexpr bool update(time_point time) noexcept {
      const auto s = std::clamp(
          std::chrono::duration<float>(time - start).count() / duration,  //
          0.0f, 1.0f);
      // branches needed for numerical stability and equality preservation
      if (s >= 1.0f) {
        *camera = last;
        return true;
      }
      if (s <= 0.0f) {
        *camera = first;
        return false;
      }
      const auto t = 3 * s * s - 2 * s * s * s;
      const auto d1 = distance(first.focus, first.translation);
      const auto d2 = distance(last.focus, last.translation);
      const auto d = std::lerp(d1, d2, t);
      camera->focus = mix(first.focus, last.focus, t);
      camera->orientation = slerp(first.orientation, last.orientation, t);
      camera->translation = camera->focus + d * camera->out();
      return false;
    }
  };
  std::optional<camera_switch_animation> camera_animation{};

  struct camera_zoom_animation {
    using clock = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<clock>;

    float scale;
    opengl::perspective_camera first;
    opengl::perspective_camera* camera;
    float duration = 1.0f;
    time_point start = clock::now();

    constexpr bool update(time_point time = clock::now()) noexcept {
      const auto s = std::clamp(
          0.0f, 1.0f,
          std::chrono::duration<float>(time - start).count() / duration);
      const auto t = 3 * s * s - 2 * s * s * s;
      *camera = first;
      camera->zoom(t * scale);
      return s >= 1.0f;
    }
  };
  std::optional<camera_zoom_animation> camera_zoom{};

  struct scene_metric {
    vec3 center{};
    real radius{};
    real min_length{};
  };
  scene_metric metric{};

  // interaction
  struct trackball_interaction {
    vec2 from;
    vec2 to;
    opengl::frame init;
    constexpr trackball_interaction(opengl::camera_base const& camera,
                                    vec2 pos) noexcept
        : init{camera}, from{pos} {}
    constexpr void operator()(opengl::camera_base& camera, vec2 pos) noexcept {
      to = pos;
      camera.trackball(init, 2 * (to - from));
    }

    constexpr void operator()(opengl::camera_base& camera) const noexcept {
      camera.trackball(init, 2 * (to - from));
    }
  };
  struct bell_trackball_interaction {
    vec2 from;
    vec2 to;
    opengl::frame init;
    constexpr bell_trackball_interaction(opengl::camera_base const& camera,
                                         vec2 pos) noexcept
        : init{camera}, from{pos} {}
    constexpr void operator()(opengl::camera_base& camera, vec2 pos) noexcept {
      to = pos;
      camera.bell_trackball(init, from, to);
    }

    constexpr void operator()(opengl::camera_base& camera) const noexcept {
      camera.bell_trackball(init, from, to);
    }
  };
  std::optional<std::variant<trackball_interaction, bell_trackball_interaction>>
      trackball{};

  // size_t scales = 10;
  // uint32 scale = 0;

  struct primitive {
    static constexpr auto primitive_format =
        opengl::primitive(opengl::vertex_buffer<scene::vertex>(
            opengl::attribute<0>(MEMBER_VAR(position)),
            opengl::attribute<1>(MEMBER_VAR(normal)),
            opengl::attribute<2>(MEMBER_VAR(texuv))));

    primitive(scene::material_index mat,
              auto&& vs,
              auto&& fs,
              auto&& wo,
              auto&& wd)
        : material{mat},
          vertices{std::forward<decltype(vs)>(vs)},
          elements{std::forward<decltype(fs)>(fs)},
          bone_weight_offsets{std::forward<decltype(wo)>(wo)},
          bone_weight_data{std::forward<decltype(wd)>(wd)} {
      primitive_format.format(vertex_array.native_handle(),
                              elements.buffer().native_handle());
      primitive_format.template format<0>(vertex_array,
                                          opengl::vector_span{vertices});
    }

    opengl::vertex_array
        vertex_array{};  // is basically view to a mesh primitive
    // --- Should be separated in the future
    // primitive format
    scene::material_index material{};
    opengl::device_vector<scene::vertex> vertices{};  // could also be a span
    opengl::device_vector<scene::face> elements{};    // could also be a span

    opengl::device_vector<scene::size_type> bone_weight_offsets{};
    opengl::device_vector<scene::skeleton::weight_data::entry>
        bone_weight_data{};

    void draw(opengl::program_view shader) {
      // shader.use();
      shader.try_set("material_index", uint(material));
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                       bone_weight_offsets.buffer().native_handle());
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1,
                       bone_weight_data.buffer().native_handle());
      vertex_array.bind();
      glDrawElements(GL_TRIANGLES, 3 * elements.size(), GL_UNSIGNED_INT, 0);
    }
  };

  std::vector<primitive> primitives{};
  opengl::device_vector<mat4> bone_transforms{};

  struct material {
    // vec4 ambient;
    // vec4 diffuse;
    // vec4 specular;
    uint albedo_map;
    uint orm_map;
  };
  opengl::device_vector<material> materials{};

  std::vector<opengl::texture2> textures{};

  // opengl::vertex_array vertex_array{};
  // opengl::device_vector<scene::vertex> vertices{};
  // opengl::device_vector<scene::face> elements{};
  // opengl::device_vector<vec4> normals{};
  // std::unique_ptr<opengl::legacy_buffer> element_buffer{};

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

  std::shared_ptr<opengl::program_target> animation_shader =
      build.target("animation",
                   {
                       opengl::vs("exaggerated-shading-demo/animation.vs.glsl"),
                       opengl::gs("exaggerated-shading-demo/animation.gs.glsl"),
                       opengl::fs("exaggerated-shading-demo/animation.fs.glsl"),
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

  std::shared_ptr<opengl::program_target> frame_points_shader = build.target(
      "frame.points",
      {
          opengl::vs("exaggerated-shading-demo/frame.points.vs.glsl"),
          opengl::fs("exaggerated-shading-demo/frame.points.fs.glsl"),
      });
  std::shared_ptr<opengl::program_target> frame_axes_shader = build.target(
      "frame.axes",
      {
          opengl::vs("exaggerated-shading-demo/frame.axes.vs.glsl"),
          opengl::gs("exaggerated-shading-demo/frame.axes.gs.glsl"),
          opengl::fs("exaggerated-shading-demo/frame.axes.fs.glsl"),
      });

 public:
  viewer(uint width = 500, uint height = 500);

  void eval_lua(std::string_view str);
  void eval_lua_file(std::filesystem::path const& path);

  void show(struct scene const& scene);
  void run();

  void quit() noexcept { done = true; }
  void leave() noexcept {
    if (lua_level > 0)
      waiting = true;
    else
      quit();
  }

  void undo() {
    if (modifiers.empty()) return;
    modifiers.pop_back();
    current = init;
    for (auto& mod : modifiers) std::invoke(mod, current);
  }
  void redo() { std::println("Redo is not yet supported"); }

  template <std::invocable<sf::Event::KeyPressed> signal>
  struct keysig : signal {};
  struct keymod {
    bool alt{};
    bool control{};
    bool shift{};
    bool system{};
    friend constexpr auto operator+(keymod x, keymod y) noexcept {
      return keymod{
          x.alt || y.alt,
          x.control || y.control,
          x.shift || y.shift,
          x.system || y.system,
      };
    }
    friend constexpr auto operator<=>(keymod, keymod) noexcept = default;
    friend constexpr bool operator==(sf::Event::KeyPressed key,
                                     keymod mod) noexcept {
      return (key.alt == mod.alt) && (key.control == mod.control) &&
             (key.shift == mod.shift) && (key.system == mod.system);
    }

    template <std::invocable<sf::Event::KeyPressed> signal>
    friend constexpr auto operator+(keymod mod, keysig<signal> sig) noexcept {
      return [mod, sig = static_cast<signal>(sig)](sf::Event::KeyPressed key) {
        return sig(key) && (key == mod);
      };
    }
  };
  static constexpr auto alt = keymod{true, false, false, false};
  static constexpr auto ctrl = keymod{false, true, false, false};
  static constexpr auto shift = keymod{false, false, true, false};
  static constexpr auto system = keymod{false, false, false, true};

  static constexpr auto esc = keysig{[](sf::Event::KeyPressed key) {
    return key.scancode == sf::Keyboard::Scancode::Escape;
  }};
  static constexpr auto enter = keysig{[](sf::Event::KeyPressed key) {
    return key.scancode == sf::Keyboard::Scancode::Enter;
  }};
  static constexpr auto space = keysig{[](sf::Event::KeyPressed key) {
    return key.scancode == sf::Keyboard::Scancode::Space;
  }};
  static constexpr auto backspace = keysig{[](sf::Event::KeyPressed key) {
    return key.scancode == sf::Keyboard::Scancode::Backspace;
  }};
  static constexpr auto z = keysig{[](sf::Event::KeyPressed key) {
    return key.code == sf::Keyboard::Key::Z;
  }};

  template <std::invocable<sf::Event::KeyPressed> signal>
  static constexpr auto keypress(keysig<signal> sig) noexcept {
    return [sig](sf::Event const& event) {
      if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        return sig(*key) && (*key == keymod{});
      return false;
    };
  }

  static constexpr auto keypress(
      std::invocable<sf::Event::KeyPressed> auto sig) noexcept {
    return [sig](sf::Event const& event) {
      if (const auto* key = event.getIf<sf::Event::KeyPressed>())
        return sig(*key);
      return false;
    };
  }

  static constexpr auto action(std::invocable<sf::Event const&> auto activation,
                               std::invocable auto execution) {
    return [activation, execution](sf::Event const& event) {
      if (activation(event)) execution();
    };
  }

  static constexpr auto keybinding(
      std::invocable<sf::Event::KeyPressed> auto sig,
      std::invocable auto exe) noexcept {
    return action(keypress(sig), exe);
  }

  // bool esc_action(sf::Event const& event) noexcept {
  //   if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
  //     if (key->scancode == sf::Keyboard::Scancode::Escape) {
  //       return not(key->alt && key->control && key->shift && key->system);
  //     }
  //   }
  //   return false;
  // }

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
