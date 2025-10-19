#include "viewer.hpp"
//
#include <glbinding/glbinding.h>

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

out vec3 position;
out vec3 normal;

int main() {
  position = vec3(view * vec4(p, 1.0));
  normal = vec3(transpose(inverse(view)) * vec4(n, 0.0));
}
)"";
  czstring fragment_shader_src = R""(
#version 460 core

in vec3 position;
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
      else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) done = true;
      }
    }

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

void viewer::load_stl_surface(const filesystem::path& path) {
  scene = scene_from(stl_surface{path});

  glNamedBufferData(vertex_buffer,
                    scene.vertices.size() * sizeof(scene::vertex),
                    scene.vertices.data(), GL_STATIC_DRAW);
}

}  // namespace demo
