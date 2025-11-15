#include "viewer.hpp"
//
#include <glbinding/glbinding.h>
//
#include "aabb.hpp"

namespace demo {

opengl_window::opengl_window(uint width, uint height)
    : window(sf::VideoMode({width, height}),
             "Exaggerated Shading Demo",
             sf::Style::Default,
             sf::State::Windowed,
             sf::ContextSettings{
                 /*.depthBits = */ 24,
                 /*.stencilBits = */ 8,
                 /*.antialiasingLevel = */ 4,
                 /*.majorVersion = */ 4,
                 /*.minorVersion = */ 6,
                 /*.attributeFlags = */
                 sf::ContextSettings::Core /*| sf::ContextSettings::Debug*/,
                 /*.sRgbCapable = */ false}) {
  // window.setActive(true);
  window.setVerticalSyncEnabled(true);
  window.setKeyRepeatEnabled(false);

  glbinding::initialize(sf::Context::getFunction);
}

viewer::viewer(uint width, uint height) : opengl_window{width, height} {
  lua.open_libraries(sol::lib::base);
  lua["quit"] = [this]() { done = true; };
  lua["set_clear_color"] = [this](float r, float g, float b) {
    glClearColor(r, g, b, 1.0f);
  };
  lua["load_scene"] = [this](std::string_view path) { load_scene(path); };

  auto assign_sources = [](opengl::shader_build_rule& obj,
                           sol::variadic_args args) {
    if ((args.size() == 1) && (not args[0].is<std::string_view>())) {
      sol::table const& t = args.get<sol::table>(0);
      for (auto [k, v] : t)
        obj.sources.push_back(opengl::source{v.as<std::string_view>()});
      return;
    }
    for (auto const& x : args)
      obj.sources.push_back(opengl::source(x.as<std::string_view>()));
    return;
  };

  lua["vs"] = [assign_sources](sol::variadic_args args) {
    opengl::shader_build_rule obj{GL_VERTEX_SHADER};
    assign_sources(obj, args);
    return obj;
  };
  lua["fs"] = [assign_sources](sol::variadic_args args) {
    opengl::shader_build_rule obj{GL_FRAGMENT_SHADER};
    assign_sources(obj, args);
    return obj;
  };
  lua.new_usertype<opengl::shader_build_rule>(
      "shader_build_rule",  //
      "print", &opengl::shader_build_rule::print);

  lua["exe"] = [](sol::variadic_args args) {
    opengl::program_build_rule exe{};
    if ((args.size() == 1) && (not args[0].is<opengl::shader_build_rule>())) {
      sol::table const& t = args.get<sol::table>(0);
      for (auto [k, v] : t)
        exe.shaders.push_back(v.as<opengl::shader_build_rule>());
      return exe;
    }
    for (auto const& x : args)
      exe.shaders.push_back(x.as<opengl::shader_build_rule>());
    return exe;
  };
  lua.new_usertype<opengl::program_build_rule>(
      "program_build_rule",  //
      "print", &opengl::program_build_rule::print);

  lua.new_usertype<opengl::program_target>(
      "build",  //
      "target",
      sol::factories(
          [this](std::string const& name) { return build.target(name); },
          [this](std::string const& name,
                 opengl::program_build_rule const& rule) {
            return build.target(name, rule);
          }),  //
      "rule", [](opengl::program_target& self, sol::table const& data) {
        opengl::program_build_rule rule{};
        for (auto& [key, value] : data)
          rule.shaders.push_back(value.as<opengl::shader_build_rule>());
        self.set_rule(std::move(rule));

        // self.set_rule(opengl::program_build_rule(
        //     std::from_range, data | std::views::values |
        //                          std::views::transform([](auto const& x) {
        //                            return x.as<opengl::shader_build_rule>();
        //                          })));
      });

  glEnable(GL_DEPTH_TEST);
  // glEnable(GL_MULTISAMPLE);
  // glEnable(GL_POINT_SMOOTH);
  // glEnable(GL_POINT_SPRITE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(10.0f);
  glLineWidth(0.5f);
  glClearColor(0.8f, 0.8f, 0.8f, 1.0f);

  // vertex_array.format(
  //     opengl::mapping<scene::vertex>(vertex_buffer,  //
  //                                    MEMBER(0, position), MEMBER(1, normal)),
  //     opengl::mapping<scene::color>(color_buffer, 3));

  // vertex_array.set_vertex_buffer(
  //     0, opengl::format<scene::vertex>(vertex_buffer,        //
  //                                      MEMBER(0, position),  //
  //                                      MEMBER(1, normal)));

  // vertex_array.format(
  //     opengl::format<scene::vertex>(vertex_buffer, MEMBER(0, position),
  //                                   MEMBER(1, normal)),
  //     opengl::buffer_format{.buffer = normals_buffer,
  //                           .offset = (scales - 1) * sizeof(vec4),
  //                           .stride = sizeof(vec4),
  //                           .attributes = {opengl::attr<vec4>(2, 0)}});

  // vertex_array.set_element_buffer(element_buffer);
  vertex_array.set_element_buffer(elements.buffer());

  normals_buffer.bind_base(GL_SHADER_STORAGE_BUFFER, 0);

  //   czstring vertex_shader_src = (const char[]){
  // #embed "vs.glsl" suffix(, )
  //       0,
  //   };
  //   czstring fragment_shader_src = (const char[]){
  // #embed "fs.glsl" suffix(, )
  //       0,
  //   };
  //   const auto status = shader.build(opengl::vs(vertex_shader_src),
  //                                    opengl::fs(fragment_shader_src));
  //   status.print();
  // if (not status.success) done = true;

  // shader = program_rule.build();
  // assert(shader.valid());

  // shader.use();

  // build_shader();
  shader->build();
}

void viewer::run() {
  while (not done) {
    while (const auto event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>())
        done = true;
      else if (const auto* resized = event->getIf<sf::Event::Resized>())
        on_resize(resized->size.x, resized->size.y);
      else if (const auto* scrolled =
                   event->getIf<sf::Event::MouseWheelScrolled>()) {
        zoom(0.1 * scrolled->delta);
      } else if (const auto* keyPressed =
                     event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) done = true;
        if (keyPressed->scancode == sf::Keyboard::Scancode::Enter) {
          scale = (scale + 1) % scales;
        } else if (keyPressed->scancode == sf::Keyboard::Scancode::Space) {
          // build_shader();
          // shader->build();
          // build.target("default")->build();
          // build.target("default", std::move(shader2->rule));
        }
      }
    }

    watch();
    // if (program_rule.check()) build_shader();
    build.update();
    update_shader();

    // Get new mouse position and compute movement in space.
    const auto new_mouse_pos = sf::Mouse::getPosition(window);
    const auto mouse_move = new_mouse_pos - mouse_pos;
    mouse_pos = new_mouse_pos;

    if (window.hasFocus()) {
      if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
          shift({mouse_move.x, mouse_move.y});
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
          zoom(0.01 * mouse_move.y);
        else
          turn({-0.01 * mouse_move.x, 0.01 * mouse_move.y});
      }
    }

    if (view_should_update) update_view();

    render();
    window.display();
  }
}

void viewer::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  vertex_array.bind();
  glDrawElements(GL_TRIANGLES, 3 * scene.faces.size(), GL_UNSIGNED_INT, 0);
}

void viewer::on_resize(int width, int height) {
  glViewport(0, 0, width, height);
  camera.set_screen_resolution(width, height);
  view_should_update = true;
}

void viewer::update_view() {
  // Compute camera position by using spherical coordinates.
  // This transformation is a variation of the standard
  // called horizontal coordinates often used in astronomy.
  //
  auto p = cos(altitude) * sin(azimuth) * right +  //
           cos(altitude) * cos(azimuth) * front +  //
           sin(altitude) * up;
  p *= radius;
  p += origin;
  camera.move(p).look_at(origin, up);

  // camera.set_near_and_far(std::max(1e-3f * radius, radius - bounding_radius),
  //                         radius + bounding_radius);

  camera.set_near_and_far(
      std::max(1e-3f * bounding_radius, radius - 10.0f * bounding_radius),
      radius + 10.0f * bounding_radius);

  view_should_update = false;
}

void viewer::load_scene(const filesystem::path& path) {
  scene = scene_from(path);
  scene.generate_edges();
  scene.smooth_normals(scales);

  fit_view_to_surface();

  // vertex_buffer.assign(scene.vertices);
  // element_buffer.assign(scene.faces);
  normals_buffer.assign(scene.smoothed_normals);

  vertices.assign(scene.vertices);
  assert(vertices.size() == scene.vertices.size());
  elements.assign(scene.faces);
  assert(elements.size() == scene.faces.size());

  // vertex_array.format(
  //     opengl::format<scene::vertex>(vertex_buffer, MEMBER(0, position),
  //                                   MEMBER(1, normal)),
  //     opengl::buffer_format{
  //         .buffer = normals_buffer,
  //         .offset = (scales - 1) * sizeof(vec4) * scene.vertices.size(),
  //         .stride = sizeof(vec4),
  //         .attributes = {opengl::attr<vec4>(2, 0)}});

  vertex_array.format(
      opengl::format<scene::vertex>(vertices.buffer(),  //
                                    MEMBER(0, position), MEMBER(1, normal)),
      opengl::offset_format<vec4>(
          normals_buffer, (scales - 1) * sizeof(vec4) * scene.vertices.size(),
          ACCESS(2, x, x)));
}

void viewer::fit_view_to_surface() {
  const auto box = aabb_from(scene);
  origin = box.origin();
  bounding_radius = box.radius();
  radius = bounding_radius / tan(0.5f * camera.vfov());
  camera.set_near_and_far(1e-5f * radius, 100 * radius);
  view_should_update = true;
}

void viewer::turn(const vec2& angle) {
  altitude += angle.y;
  azimuth += angle.x;
  constexpr float bound = pi / 2 - 1e-5f;
  altitude = std::clamp(altitude, -bound, bound);
  view_should_update = true;
}

void viewer::shift(const vec2& pixels) {
  const auto shift = -pixels.x * camera.right() + pixels.y * camera.up();
  const auto scale = camera.pixel_size() * radius;
  origin += scale * shift;
  view_should_update = true;
}

void viewer::zoom(float scale) {
  radius *= exp(-scale);
  view_should_update = true;
}

void viewer::listen(fdm::address const& domain) {
  auto msg = fdm::recv(domain);
  if (not msg) return;
  println("\n{}:\n---\n{}---", proximate(domain).string(), msg.value());
  scoped_chdir _{domain.parent_path()};
  const auto result =
      lua.safe_script(std::move(msg).value(), sol::script_pass_on_error);
  if (not result.valid()) println("ERROR:\n{}\n", sol::error{result}.what());
}

void viewer::watch() {
  for (const auto& path : paths) {
    if (not is_directory(path)) {
      listen(path);
      continue;
    }
    for (const auto& entry : filesystem::recursive_directory_iterator(path)) {
      if (not entry.is_regular_file()) continue;
      if (entry.path().extension() != ".lua") continue;
      listen(entry.path());
    }
  }
}

void viewer::add_path(std::filesystem::path const& path) {
  paths.push_back(path);
  std::println("Paths being watched:");
  for (const auto& path : paths) println("  {}", path.string());
}

void viewer::build_shader() {
  // auto [exe, status] = program_rule.build();
  // status.print();
  // if (not status.success) return;
  // shader = std::move(exe);
  // shader.use();
  // view_should_update = true;
}

void viewer::update_shader() {
  shader->try_set("projection", camera.projection_matrix());
  shader->try_set("view", camera.view_matrix());
  shader->try_set("scales", (uint32)scales);
  shader->try_set("count", (uint32)scene.vertices.size());
  shader->try_set("scale", scale);
  shader->shader.use();
}

}  // namespace demo
