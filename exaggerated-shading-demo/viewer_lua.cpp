#include "viewer.hpp"

namespace demo {

void viewer::init_lua() {
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
}

}  // namespace demo
