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

  texture.alloc(1, GL_RGBA8, texture_size.x, texture_size.y);
  texture.set_min_filter_to_nearest();
  texture.set_mag_filter_to_nearest();
  texture.clamp_to_edges();
  renderbuffer.alloc(texture_size.x, texture_size.y);
  fbo.attach(texture);
  fbo.attach(renderbuffer);
  if (glCheckNamedFramebufferStatus(fbo.native_handle(), GL_FRAMEBUFFER) !=
      GL_FRAMEBUFFER_COMPLETE)
    std::println("ERROR: Incomplete Framebuffer");
}

void viewer::show(struct scene const& scene) {
  const auto [m, r] = bounding_sphere(scene);
  world.move_to(m);
  cam.fit(world, r);

  normals_buffer.assign(scene.smoothed_normals);
  vertices.assign(scene.vertices);
  elements.assign(scene.faces);

  vertex_array.format(
      opengl::format<scene::vertex>(vertices.buffer(),  //
                                    MEMBER(0, position), MEMBER(1, normal)),
      opengl::offset_format<vec4>(
          normals_buffer, (scales - 1) * sizeof(vec4) * scene.vertices.size(),
          ACCESS(2, x, x)));
}

void viewer::run() {
  while (not done) {
    watch();
    update();
  }
}

void viewer::update() {
  process_events();
  build.update();
  render();
  window.display();
}

void viewer::eval_lua(std::string_view str) {
  lua_running = true;
  const auto result = lua.safe_script(str, sol::script_pass_on_error);
  lua_running = false;
  if (not result.valid())
    std::println("ERROR:\n{}\n", sol::error{result}.what());
  waiting = false;
}

void viewer::eval_lua_file(std::filesystem::path const& path) {
  scoped_chdir _{path.parent_path()};
  lua_running = true;
  const auto result = lua.safe_script_file(path, sol::script_pass_on_error);
  lua_running = false;
  if (not result.valid())
    std::println("ERROR:\n{}\n", sol::error{result}.what());
  waiting = false;
}

void viewer::watch() {
  for (auto const& path : lua_live_paths) {
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

void viewer::listen(fdm::address const& domain) {
  auto msg = fdm::recv(domain);
  if (not msg) return;
  println("\n{}:\n---\n{}---", proximate(domain).string(), msg.value());
  scoped_chdir _{domain.parent_path()};
  // const auto result =
  //     lua.safe_script(std::move(msg).value(), sol::script_pass_on_error);
  // if (not result.valid()) println("ERROR:\n{}\n", sol::error{result}.what());
  eval_lua(std::move(msg).value());
}

void viewer::process_events() {
  const auto mouse_tmp = sf::Mouse::getPosition(window);
  const auto mouse = vec2(mouse_tmp.x, mouse_tmp.y);

  while (const auto event = window.pollEvent()) {
    if (event->is<sf::Event::Closed>())
      done = true;
    else if (const auto* resized = event->getIf<sf::Event::Resized>())
      on_resize(resized->size.x, resized->size.y);
    else if (const auto* scrolled =
                 event->getIf<sf::Event::MouseWheelScrolled>()) {
      cam.zoom(0.1 * scrolled->delta);
    } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
      if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
        if (lua_running)
          waiting = true;
        else
          done = true;
      }
      if (keyPressed->scancode == sf::Keyboard::Scancode::Enter) {
      } else if (keyPressed->scancode == sf::Keyboard::Scancode::Space) {
      }
    } else if (const auto* keyReleased =
                   event->getIf<sf::Event::KeyReleased>()) {
      if (keyReleased->scancode == sf::Keyboard::Scancode::Space) {
      }
    } else if (const auto* mouse_event =
                   event->getIf<sf::Event::MouseButtonPressed>()) {
      switch (mouse_event->button) {
        case sf::Mouse::Button::Left:
          trackball = trackball_interaction{cam, screen.space(mouse)};
          break;
        case sf::Mouse::Button::Right:
          trackball = bell_trackball_interaction{cam, screen.space(mouse)};
          break;
      }
    } else if (const auto* mouse_event =
                   event->getIf<sf::Event::MouseButtonReleased>()) {
      switch (mouse_event->button) {
        case sf::Mouse::Button::Left:
          trackball = {};
          break;
        case sf::Mouse::Button::Right:
          trackball = {};
          break;
      }
    }
  }

  // if (window.hasFocus()) {
  //   if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
  //     if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) {
  //     } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
  //       cam.zoom(0.01 * mouse_move.y);
  //     } else {
  //     }
  //   }
  // }

  // if (trackball) trackball.value()(cam, screen.space(mouse));
  if (trackball)
    std::visit(
        [this, mouse](auto& action) {
          std::invoke(action, cam, screen.space(mouse));
        },
        trackball.value());
}

void viewer::render() {
  // glBindFramebuffer(GL_FRAMEBUFFER, 0);

  use(screen);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // shader->try_set("projection", camera.projection_matrix());
  // shader->try_set("view", camera.view_matrix());
  // pcam.basis = camui.transform();
  // shader->try_set("projection", projection(ocam, screen));
  // shader->try_set("view", camui.transform());
  shader->try_set("projection", cam.projection(screen));
  shader->try_set("view", cam.view());
  shader->try_set("scales", (uint32)scales);
  shader->try_set("count", GLuint(vertices.size()));
  shader->try_set("scale", scale);
  shader->shader.use();
  vertex_array.bind();
  glDrawElements(GL_TRIANGLES, 3 * elements.size(), GL_UNSIGNED_INT, 0);

  use(opengl::viewport{{0, 0}, texture_size});

  fbo.bind();
  const auto mouse = vec2(sf::Mouse::getPosition(window).x,
                          screen.size.y - sf::Mouse::getPosition(window).y);
  const auto pick = glm::pickMatrix(mouse, vec2{20, 20}, ivec4(screen));
  shader->try_set("projection", pick * cam.projection(screen));
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDrawElements(GL_TRIANGLES, 3 * elements.size(), GL_UNSIGNED_INT, 0);

  //
  opengl::default_framebuffer.bind();
  texture_shader->shader.use();
  texture.bind_to_unit(0);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void viewer::on_resize(int width, int height) {
  glViewport(0, 0, width, height);
  screen.size = {width, height};
}

}  // namespace demo
