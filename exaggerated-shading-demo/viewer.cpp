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

void viewer::add_path(std::filesystem::path const& path) {
  paths.push_back(path);
  std::println("Paths being watched:");
  for (const auto& path : paths) println("  {}", path.string());
}

void viewer::load_scene(const filesystem::path& path) {
  scene = scene_from(path);
  scene.generate_edges();
  scene.smooth_normals(scales);

  const auto [m, r] = bounding_sphere(scene);
  world.move_to(m);
  cam.fit(world, r);

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

void viewer::assign(struct scene const& scene) {
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
      } else if (const auto* keyPressed =
                     event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) done = true;
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

    watch();
    // if (program_rule.check()) build_shader();
    build.update();
    update_shader();

    render();
    window.display();
  }
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

void viewer::listen(fdm::address const& domain) {
  auto msg = fdm::recv(domain);
  if (not msg) return;
  println("\n{}:\n---\n{}---", proximate(domain).string(), msg.value());
  scoped_chdir _{domain.parent_path()};
  const auto result =
      lua.safe_script(std::move(msg).value(), sol::script_pass_on_error);
  if (not result.valid()) println("ERROR:\n{}\n", sol::error{result}.what());
}

void viewer::render() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  vertex_array.bind();
  glDrawElements(GL_TRIANGLES, 3 * elements.size(), GL_UNSIGNED_INT, 0);
}

void viewer::on_resize(int width, int height) {
  glViewport(0, 0, width, height);
  screen.size = {width, height};
}

// void viewer::trackball(vec2 x, vec2 y) {
//   if (length(x - y) < 1e-3f) return;
//   auto z_projection = [](vec2 p) {
//     const auto d = length(p);
//     constexpr auto r = 0.8f;
//     constexpr auto b = r / std::sqrt(2.0f);
//     return (d < b) ? std::sqrt(r * r - d * d) : (r * r / 2.0f / d);
//   };
//   const auto p = normalize(vec3(x, z_projection(x)));
//   const auto q = normalize(vec3(y, z_projection(y)));
//   const auto pxq = cross(p, q);
//   const auto sin_phi = length(pxq);
//   const auto cos_phi = dot(p, q);
//   const auto phi = std::acos(cos_phi);
//   const auto axis = pxq / sin_phi;
//   const auto rotation =
//       rotate(mat4(1.0f), 2.0f * phi,
//              glm::mat3(transpose(backup_cam.view_matrix())) * axis);

//   camera = backup_cam.rotate(rotation);

//   // shader->try_set("model", rotate(mat4(1.0f), phi, axis));
// }

void viewer::update_shader() {
  // shader->try_set("projection", camera.projection_matrix());
  // shader->try_set("view", camera.view_matrix());

  // pcam.basis = camui.transform();

  // shader->try_set("projection", projection(ocam, screen));
  // shader->try_set("view", camui.transform());

  shader->try_set("projection", cam.projection(screen));
  shader->try_set("view", cam.view());

  shader->try_set("scales", (uint32)scales);
  shader->try_set("count", (uint32)scene.vertices.size());
  shader->try_set("scale", scale);
  shader->shader.use();
}

}  // namespace demo
