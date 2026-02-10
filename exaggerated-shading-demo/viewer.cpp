#include "viewer.hpp"
//
#include <glbinding/glbinding.h>
//
#include "aabb.hpp"
//
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

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
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(10.0f);
  glLineWidth(0.5f);
  glClearColor(0.8f, 0.8f, 0.8f, 1.0f);

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
  // scene metric?
  const auto [m, r] = bounding_sphere(scene);
  const auto e = min_edge_length(scene);
  std::println("scene min edge length = {}", e);
  std::println("scene radius = {}", r);
  metric.center = m;
  metric.radius = r;
  metric.min_length = e;

  world.move_to(m);
  cam.fit(world, r);
  render_cam.fit(world, r);
  undo_cam = cam;
  //
  current.world.move_to(m);
  current.camera.fit(current.world, r);
  modifiers.clear();
  init = current;

  primitives.clear();
  for (size_t mid = 0; auto& mesh : scene.meshes) {
    std::println("mesh: material = {}", mesh.material);
    primitives.push_back({
        mesh.material,
        mesh.vertices,
        mesh.faces,
        scene.skeleton.weights[mid].offsets,
        scene.skeleton.weights[mid].data,
    });
    ++mid;
  }

  textures.clear();
  textures.resize(scene.textures.size());
  for (size_t i = 1; i < scene.textures.size(); ++i) {
    auto const& tex = textures[i];
    auto const& path = scene.textures[i];

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char* pixels =
        stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (!pixels) {
      std::println("ERROR: Failed to load image from {}", path.string());
      continue;
    }

    std::println("texture: {}x{}x{} {}", width, height, channels,
                 path.string());

    int levels =
        1 + static_cast<int>(std::floor(std::log2(std::max(width, height))));
    tex.alloc(levels, GL_RGBA8, width, height);
    tex.write(0,                          //
              0, 0, width, height,        //
              GL_RGBA, GL_UNSIGNED_BYTE,  //
              pixels);
    tex.generate_mipmap();

    stbi_image_free(pixels);

    tex.set_min_filter_to_linear();
    tex.set_mag_filter_to_linear();
    tex.repeat();
  }
  std::println("textures loaded = {}", textures.size());

  bone_transforms = scene.skeleton.global_transforms(scene.animations[0], 0.0);
  materials = scene.materials | std::views::transform([](auto const& mat) {
                return material{
                    // .ambient = vec4(mat.ambient, 1.0),
                    // .diffuse = vec4(mat.diffuse, 1.0),
                    // .specular = vec4(mat.specular, 1.0),
                    .albedo_map = mat.albedo_map,
                    .orm_map = mat.orm_map,
                };
              }) |
              std::ranges::to<std::vector>();

  // normals_buffer.assign(scene.smoothed_normals);
  // vertices.assign(scene.vertices);
  // elements.assign(scene.faces);
  // normals.assign(scene.smoothed_normals);
  // vertices = opengl::const_vector{scene.vertices};
  // elements = opengl::const_vector{scene.faces};
  // normals = opengl::const_vector{scene.smoothed_normals};

  // vertices = scene.vertices;
  // elements = scene.faces;
  // normals = scene.smoothed_normals;

  // vertices = scene.meshes[0].vertices;
  // elements = scene.meshes[0].faces;

  // breakpoint(opengl::vector_span{opengl::legacy_vector{vertices}});
  // auto nv = opengl::vector_span{normals, };
  // breakpoint(opengl::vector_span{normals,  //
  //                                (scales - 1) * vertices.size(),
  //                                vertices.size()});

  // auto v = opengl::device_vector_view{scene.vertices};

  // element_buffer = std::make_unique<opengl::legacy_buffer>(scene.faces);

  //
  // opengl::device_vector dv{scene.vertices};
  // assert(dv.size() == vertices.size());
  // assert(opengl::device_vector_view{dv}.size() == vertices.size());
  // opengl::legacy_vector lv{scene.vertices};
  // assert(lv.size() == vertices.size());

  // vertex_array.format(
  //     opengl::format<scene::vertex>(vertices.buffer(),  //
  //                                   MEMBER(0, position), MEMBER(1, normal)),
  //     opengl::offset_format<vec4>(
  //         normals_buffer, (scales - 1) * sizeof(vec4) * scene.vertices.size(),
  //         ACCESS(2, x, x)));
  // vertex_array.set_element_buffer(elements.buffer());

  // normals.buffer().bind_base(GL_SHADER_STORAGE_BUFFER, 0);
  // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
  //                  normals.buffer().native_handle());

  // breakpoint(opengl::vertex_vector_format<vec4>(0));
  // breakpoint(opengl::vertex_vector_format<dvec4>(0));
  // breakpoint(opengl::vertex_vector_format<ivec4>(0));

  // static_assert(opengl::accessor_of<decltype([](auto const& x) -> auto const& {
  //                                     return x.position;
  //                                   }),
  //                                   scene::vertex>);
  // breakpoint(opengl::vertex_vector_format<scene::vertex>(
  //     std::tuple{0, [](auto const& x) -> auto const& { return x.position; }},
  //     ATTRIBUTE_MEMBER(1, normal),  //
  //     FORMAT_ATTRIBUTE_MEMBER(int, 1, normal)));

  // breakpoint(opengl::meta::vertex_buffer<1, vec4>());
  // breakpoint(opengl::meta::vertex_buffer<scene::vertex>(
  //     opengl::meta::attribute<0>(
  //         [](auto const& x) -> auto const& { return x.position; }),
  //     opengl::meta::attribute<1, double>(
  //         [](auto const& x) -> auto const& { return x.normal; })));

  // opengl::meta::primitive(
  //     elements.buffer(),                           //
  //     opengl::meta::vertex_buffer<scene::vertex>(  //
  //         opengl::meta::attribute<0>(
  //             [](auto const& x) -> auto const& { return x.position; }),  //
  //         opengl::meta::attribute<1>(
  //             [](auto const& x) -> auto const& { return x.normal; })  //
  //         ));

  // constexpr auto primitive =
  //     opengl::primitive(opengl::vertex_buffer<scene::vertex>(
  //                           opengl::attribute<0>(MEMBER_VAR(position)),
  //                           opengl::attribute<1>(MEMBER_VAR(normal))),
  //                       opengl::vertex_buffer<2, vec4>());
  // primitive.format(vertex_array.native_handle(),
  //                  elements.buffer().native_handle());
  // vertex_array.set_elements(elements.buffer());
  // vertex_array.set_elements(*element_buffer);
  // primitive.template format<0>(vertex_array.native_handle(),
  //                              vertices.buffer().native_handle());
  // primitive.template format<0>(vertex_array, opengl::vector_span{vertices});
  // primitive.template format<1>(vertex_array.native_handle(),
  //                              normals.buffer().native_handle(),
  //                              (scales - 1) * vertices.size());
  // primitive.format(
  //     vertex_array, opengl::vector_span{elements},
  //     opengl::vector_span{vertices},
  //     opengl::vector_span{normals, (scales - 1) * vertices.size()});

  // auto e = opengl::vector_view{elements};
  // auto v = opengl::vector_span{vertices};
  // auto n = opengl::vector_span{normals, (scales - 1) * vertices.size()};
  // primitive.format(vertex_array, e);
  // primitive.template format<0>(vertex_array, v);
  // primitive.template format<1>(vertex_array, n);
  // primitive.format(vertex_array, elements.buffer());
  // primitive.template format<0>(vertex_array, vertices.buffer());
  // primitive.template format<1>(vertex_array, normals.buffer(),
  //                              (scales - 1) * vertices.size());
  // primitive.format(vertex_array.native_handle(),
  //                  elements.buffer().native_handle());
  // primitive.template format<0>(vertex_array.native_handle(),  //
  //                              vertices.buffer().native_handle(), 0);
  // primitive.template format<1>(
  //     vertex_array.native_handle(),  //
  //     normals_buffer.native_handle(),
  //     (scales - 1) * sizeof(vec4) * scene.vertices.size());
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
      // cam.zoom(0.1 * scrolled->delta);
      const auto scale = 0.1 * scrolled->delta;
      // current.camera.zoom(scale);
      modifiers.push_back(
          [scale](viewer_state& state) { state.camera.zoom(scale); });
      std::invoke(modifiers.back(), current);
    } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
      if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
        if (lua_running)
          waiting = true;
        else
          done = true;
      }
      if (keyPressed->scancode == sf::Keyboard::Scancode::Enter) {
        render_cam = current.camera;
      } else if (keyPressed->scancode == sf::Keyboard::Scancode::Space) {
        if (not camera_animation) {
          if (current.camera == render_cam) {
            camera_animation = std::make_optional<camera_switch_animation>(
                std::chrono::high_resolution_clock::now(), 0.5, render_cam,
                undo_cam, &current.camera);
          } else {
            undo_cam = current.camera;
            camera_animation = std::make_optional<camera_switch_animation>(
                std::chrono::high_resolution_clock::now(), 0.5, current.camera,
                render_cam, &current.camera);
          }
        }
      } else if (keyPressed->scancode == sf::Keyboard::Scancode::Backspace) {
        if (not modifiers.empty()) {
          modifiers.pop_back();
          current = init;
          for (auto& mod : modifiers) std::invoke(mod, current);
        }
      }
    } else if (const auto* keyReleased =
                   event->getIf<sf::Event::KeyReleased>()) {
      if (keyReleased->scancode == sf::Keyboard::Scancode::Space) {
      }
    } else if (const auto* mouse_event =
                   event->getIf<sf::Event::MouseButtonPressed>()) {
      switch (mouse_event->button) {
        case sf::Mouse::Button::Left:
          trackball =
              trackball_interaction{current.camera, screen.space(mouse)};
          break;
        case sf::Mouse::Button::Right:
          trackball =
              bell_trackball_interaction{current.camera, screen.space(mouse)};
          break;
          // case sf::Mouse::Button::Middle:
          //   cam_interpolation = mouse;
          //   undo_cam = cam;
          //   break;
      }
    } else if (const auto* mouse_event =
                   event->getIf<sf::Event::MouseButtonReleased>()) {
      switch (mouse_event->button) {
        case sf::Mouse::Button::Left:
        case sf::Mouse::Button::Right:
          if (trackball)
            std::visit(
                [this](auto& action) {
                  modifiers.push_back(
                      [action](viewer_state& state) { action(state.camera); });
                  std::invoke(modifiers.back(), current);
                },
                trackball.value());
          trackball = {};
          break;
          // case sf::Mouse::Button::Middle:
          //   cam_interpolation = {};
          //   break;
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
          std::invoke(action, current.camera, screen.space(mouse));
        },
        trackball.value());

  // if (cam_interpolation) {
  //   const float delta = (mouse - cam_interpolation.value()).y;
  //   cam_interpolate = std::clamp(0.01f * delta, 0.0f, 1.0f);
  //   const auto d1 = distance(undo_cam.focus, undo_cam.translation);
  //   const auto d2 = distance(render_cam.focus, render_cam.translation);
  //   const auto d = std::lerp(d1, d2, cam_interpolate);
  //   cam.focus = mix(undo_cam.focus, render_cam.focus, cam_interpolate);
  //   cam.orientation =
  //       slerp(undo_cam.orientation, render_cam.orientation, cam_interpolate);
  //   cam.translation = cam.focus + d * cam.out();
  // }

  if (camera_animation)
    if (camera_animation->update(std::chrono::high_resolution_clock::now()))
      camera_animation = {};
}

void viewer::render() {
  // glBindFramebuffer(GL_FRAMEBUFFER, 0);

  use(screen);

  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  opengl::current_framebuffer::clear();

  // shader->try_set("projection", camera.projection_matrix());
  // shader->try_set("view", camera.view_matrix());
  // pcam.basis = camui.transform();
  // shader->try_set("projection", projection(ocam, screen));
  // shader->try_set("view", camui.transform());

  // shader: viewport
  // shader: mouse position
  // shader: time

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2,
                   bone_transforms.buffer().native_handle());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3,
                   materials.buffer().native_handle());
  for (int i = 0; i < textures.size(); ++i) textures[i].bind_to_unit(i);

  const auto d = distance(current.camera.origin(), metric.center);
  const real near =
      std::max(d - metric.radius,
               std::max(10 * metric.min_length, metric.radius / 10000));
  const real far = d + metric.radius;
  const auto projection = current.camera.projection(screen, near, far);

  // Render the rendering camera frame.
  const auto frame_scale = 0.1f * metric.radius;
  frame_axes_shader->try_set("projection", projection);
  frame_axes_shader->try_set("view", current.camera.view());
  frame_axes_shader->try_set("frame",
                             scale(render_cam.global(), vec3(frame_scale)));
  frame_axes_shader->shader.use();
  glDrawArrays(GL_LINES, 0, 12);
  frame_points_shader->try_set("projection", projection);
  frame_points_shader->try_set("view", current.camera.view());
  frame_points_shader->try_set("frame",
                               scale(render_cam.global(), vec3(frame_scale)));
  frame_points_shader->shader.use();
  glDrawArrays(GL_POINTS, 0, 7);

  shader->try_set("projection", projection);
  shader->try_set("view", current.camera.view());
  shader->try_set("screen_size", vec2(screen.size));
  // shader->try_set("scales", (uint32)scales);
  // shader->try_set("count", GLuint(vertices.size()));
  // shader->try_set("scale", scale);
  shader->shader.use();

  GLint samplers[32];
  for (int i = 0; i < textures.size(); ++i) samplers[i] = i;
  glUniform1iv(glGetUniformLocation(shader->shader.native_handle(), "textures"),
               textures.size(), samplers);

  // vertex_array.bind();
  // glDrawElements(GL_TRIANGLES, 3 * elements.size(), GL_UNSIGNED_INT, 0);
  for (uint i = 0; auto& primitive : primitives) {
    shader->try_set("mesh_index", i);
    primitive.draw(shader->shader);
    ++i;
  }

  use(opengl::viewport{{0, 0}, texture_size});

  opengl::current_framebuffer::set(fbo);
  opengl::current_framebuffer::clear();
  const auto mouse = vec2(sf::Mouse::getPosition(window).x,
                          screen.size.y - sf::Mouse::getPosition(window).y);
  const auto mag_screen = vec2{20, 20};
  const auto pick = glm::pickMatrix(mouse, mag_screen, ivec4(screen));
  shader->try_set("projection", pick * projection);
  shader->try_set("screen_size", mag_screen);

  // glDrawElements(GL_TRIANGLES, 3 * elements.size(), GL_UNSIGNED_INT, 0);
  for (auto& primitive : primitives) primitive.draw(shader->shader);

  opengl::current_framebuffer::set(opengl::default_framebuffer);
  texture_shader->shader.use();
  texture.bind_to_unit(0);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void viewer::on_resize(int width, int height) {
  glViewport(0, 0, width, height);
  screen.size = {width, height};
}

}  // namespace demo
