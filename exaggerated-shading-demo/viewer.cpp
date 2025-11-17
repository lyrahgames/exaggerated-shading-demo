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
  init_lua();

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
  bool click = false;

  while (not done) {
    // Get new mouse position and compute movement in space.
    const auto new_mouse_pos = sf::Mouse::getPosition(window);
    // mouse_pos = new_mouse_pos;

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
          trackball_start = sf::Mouse::getPosition(window);
          backup_cam = camera;
        }
      } else if (const auto* keyReleased =
                     event->getIf<sf::Event::KeyReleased>()) {
        if (keyReleased->scancode == sf::Keyboard::Scancode::Space)
          trackball_start = {};
      } else if (const auto* mouse =
                     event->getIf<sf::Event::MouseButtonPressed>()) {
        click = true;
        mouse_pos = new_mouse_pos;
        std::println("clicking");
      } else if (const auto* mouse =
                     event->getIf<sf::Event::MouseButtonReleased>()) {
        click = false;
        camui.commit();
        // mouse_pos = new_mouse_pos;
        const auto mouse_move = new_mouse_pos - mouse_pos;
        std::println("mouse move = [{},{}]", mouse_move.x, mouse_move.y);
      }
    }
    const auto mouse_move = new_mouse_pos - mouse_pos;
    // std::println("mouse move = [{},{}]", mouse_move.x, mouse_move.y);

    if (trackball_start) {
      const auto m1 = trackball_start.value();
      const auto m2 = sf::Mouse::getPosition(window);
      const auto x = vec2(2.0f * m1.x / camera.screen_height() - 1,
                          1.0f - 2.0f * m1.y / camera.screen_height());
      const auto y = vec2(2.0f * m2.x / camera.screen_height() - 1,
                          1.0f - 2.0f * m2.y / camera.screen_height());
      trackball(x, y);
    }

    watch();
    // if (program_rule.check()) build_shader();
    build.update();
    update_shader();

    if (window.hasFocus()) {
      if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
          shift({mouse_move.x, mouse_move.y});
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
          zoom(0.01 * mouse_move.y);
          camui.zoom(0.01 * mouse_move.y);
        } else {
          turn({-0.01 * mouse_move.x, 0.01 * mouse_move.y});
          camui.turn({-0.01 * mouse_move.x, 0.01 * mouse_move.y});
        }
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
  screen.size = {width, height};
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

  const auto [m, r] = bounding_sphere(scene);

  camui.basis.set_origin(m);
  camui.radius = r;

  pcam.fit(m, r);
  ocam.fit(m, r);

  view_should_update = true;

  // auto print_vec = [](vec4 v) {
  //   std::println("{>6:.3}{>6:.3}{>6:.3}{>6:.3}", v.x, v.y, v.z, v.w);
  // };
  // auto print_mat = [](mat4 m) {
  //   std::println("{}{}{}{}", m[0], m[1], m[2], m[3]);
  // };
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

void viewer::trackball(vec2 x, vec2 y) {
  if (length(x - y) < 1e-3f) return;
  auto z_projection = [](vec2 p) {
    const auto d = length(p);
    constexpr auto r = 0.8f;
    constexpr auto b = r / std::sqrt(2.0f);
    return (d < b) ? std::sqrt(r * r - d * d) : (r * r / 2.0f / d);
  };
  const auto p = normalize(vec3(x, z_projection(x)));
  const auto q = normalize(vec3(y, z_projection(y)));
  const auto pxq = cross(p, q);
  const auto sin_phi = length(pxq);
  const auto cos_phi = dot(p, q);
  const auto phi = std::acos(cos_phi);
  const auto axis = pxq / sin_phi;
  const auto rotation =
      rotate(mat4(1.0f), 2.0f * phi,
             glm::mat3(transpose(backup_cam.view_matrix())) * axis);

  camera = backup_cam.rotate(rotation);

  // shader->try_set("model", rotate(mat4(1.0f), phi, axis));
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
  // shader->try_set("projection", camera.projection_matrix());
  // shader->try_set("view", camera.view_matrix());

  // pcam.basis = camui.transform();

  shader->try_set("projection", projection(ocam, screen));
  shader->try_set("view", camui.transform());

  shader->try_set("scales", (uint32)scales);
  shader->try_set("count", (uint32)scene.vertices.size());
  shader->try_set("scale", scale);
  shader->shader.use();
}

}  // namespace demo
