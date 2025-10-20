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
                 /*.stencilBits = */ 0,
                 /*.antialiasingLevel = */ 2,
                 /*.majorVersion = */ 4,
                 /*.minorVersion = */ 6,
                 /*.attributeFlags = */
                 sf::ContextSettings::Core /*| sf::ContextSettings::Debug*/,
                 /*.sRgbCapable = */ false}) {
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
  glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

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

  czstring vertex_shader_src = R""(
#version 460 core

uniform mat4 projection;
uniform mat4 view;

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;

out vec3 normal;

int main() {
  gl_Position = projection * view * vec4(p, 1.0);
  normal = vec3(transpose(inverse(view)) * vec4(n, 0.0));
}
)"";
  czstring fragment_shader_src = R""(
#version 460 core

in vec3 normal;

layout (location = 0) out vec4 frag_color;

int main() {
  float s = abs(normalize(normal).z);
  float light = 0.2 + 1.0 * pow(s, 1000) + 0.75 * pow(s, 0.2);
  frag_color = vec4(vec3(light), 1.0);
}
)"";
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
      else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) done = true;
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
  glUseProgram(shader);
  glDrawArrays(GL_TRIANGLES, 0, scene.vertices.size());
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

  println("radius = {}", radius);

  view_should_update = false;
}

void viewer::load_stl_surface(const filesystem::path& path) {
  scene = scene_from(stl_surface{path});
  fit_view_to_surface();

  println("#vertices = {}", scene.vertices.size());

  glNamedBufferData(vertex_buffer,
                    scene.vertices.size() * sizeof(scene::vertex),
                    scene.vertices.data(), GL_STATIC_DRAW);
}

void viewer::fit_view_to_surface() {
  const auto box = aabb_from(scene);
  origin = box.origin();
  bounding_radius = box.radius();

  println("bounding radius = {}", bounding_radius);

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
