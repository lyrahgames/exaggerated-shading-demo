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
  glbinding::initialize(sf::Context::getFunction);
  window.setVerticalSyncEnabled(true);
  window.setKeyRepeatEnabled(false);
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

  //
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
  //
  glCreateBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(scene::vertex),
                        (void*)offsetof(scene::vertex, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(scene::vertex),
                        (void*)offsetof(scene::vertex, normal));
  //
  glCreateBuffers(1, &element_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
  //
  glCreateBuffers(1, &normals_buffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, normals_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, normals_buffer);

  czstring vertex_shader_src = (const char[]){
#embed "vs.glsl" suffix(, )
      0,
  };
  czstring fragment_shader_src = (const char[]){
#embed "fs.glsl" suffix(, )
      0,
  };
  GLint src_size = -1;
  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_src, &src_size);
  glCompileShader(vertex_shader);
  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_src, &src_size);
  glCompileShader(fragment_shader);
  shader = glCreateProgram();
  glAttachShader(shader, vertex_shader);
  glAttachShader(shader, fragment_shader);
  glLinkProgram(shader);
  glDetachShader(shader, fragment_shader);
  glDetachShader(shader, vertex_shader);
  glDeleteShader(fragment_shader);
  glDeleteShader(vertex_shader);
}

viewer::~viewer() {
  glDeleteProgram(shader);
  glDeleteBuffers(1, &element_buffer);
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteVertexArrays(1, &vertex_array);
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
          glProgramUniform1ui(shader, glGetUniformLocation(shader, "scale"),
                              scale);
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

  glBindVertexArray(vertex_array);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
  glUseProgram(shader);
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

  glProgramUniformMatrix4fv(shader, glGetUniformLocation(shader, "projection"),
                            1, GL_FALSE, value_ptr(camera.projection_matrix()));
  glProgramUniformMatrix4fv(shader, glGetUniformLocation(shader, "view"), 1,
                            GL_FALSE, value_ptr(camera.view_matrix()));

  // const auto light = -camera.front() - camera.up() + camera.right();
  // glProgramUniform3f(shader, glGetUniformLocation(shader, "light"),  //
  //                    light.x, light.y, light.z);

  view_should_update = false;
}

void viewer::load_scene(const filesystem::path& path) {
  scene = scene_from(path);
  scene.generate_edges();
  scene.smooth_normals(scales);

  fit_view_to_surface();

  glNamedBufferData(vertex_buffer,
                    scene.vertices.size() * sizeof(scene::vertex),
                    scene.vertices.data(), GL_STATIC_DRAW);
  glNamedBufferData(element_buffer, scene.faces.size() * sizeof(scene::face),
                    scene.faces.data(), GL_STATIC_DRAW);
  glNamedBufferData(
      normals_buffer,
      scene.smoothed_normals.size() * sizeof(scene.smoothed_normals[0]),
      scene.smoothed_normals.data(), GL_STATIC_DRAW);
  glProgramUniform1ui(shader, glGetUniformLocation(shader, "scales"), scales);
  glProgramUniform1ui(shader, glGetUniformLocation(shader, "count"),
                      scene.vertices.size());
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
