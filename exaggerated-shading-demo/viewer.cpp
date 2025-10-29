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
  glEnable(GL_DEPTH_TEST);
  // glEnable(GL_MULTISAMPLE);
  // glEnable(GL_POINT_SMOOTH);
  // glEnable(GL_POINT_SPRITE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(10.0f);
  glLineWidth(0.5f);
  glClearColor(0.8f, 0.8f, 0.8f, 1.0f);

  vertex_array.enable_attribute(0);
  vertex_array.set_attribute(0, opengl::attr<vec3>,
                             offsetof(scene::vertex, position));
  vertex_array.set_attribute_binding(0, 0);

  vertex_array.enable_attribute(1);
  vertex_array.set_attribute(1, opengl::attr<vec3>,
                             offsetof(scene::vertex, normal));
  vertex_array.set_attribute_binding(1, 0);

  vertex_array.set_vertex_buffer(0, vertex_buffer, 0, sizeof(scene::vertex));
  vertex_array.set_element_buffer(element_buffer);

  normals_buffer.bind_base(GL_SHADER_STORAGE_BUFFER, 0);

  czstring vertex_shader_src = (const char[]){
#embed "vs.glsl" suffix(, )
      0,
  };
  czstring fragment_shader_src = (const char[]){
#embed "fs.glsl" suffix(, )
      0,
  };
  const auto status = shader.build(opengl::vs(vertex_shader_src),
                                   opengl::fs(fragment_shader_src));
  status.print();
  if (not status.success) done = true;
  shader.use();
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
          shader.set("scale", scale);
        }
      }
    }

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

  shader.set("projection", camera.projection_matrix());
  shader.set("view", camera.view_matrix());

  view_should_update = false;
}

void viewer::load_scene(const filesystem::path& path) {
  scene = scene_from(path);
  scene.generate_edges();
  scene.smooth_normals(scales);

  fit_view_to_surface();

  vertex_buffer.assign(scene.vertices);
  element_buffer.assign(scene.faces);
  normals_buffer.assign(scene.smoothed_normals);

  shader.set("scales", (uint32)scales);
  shader.set("count", (uint32)scene.vertices.size());
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

}  // namespace demo
