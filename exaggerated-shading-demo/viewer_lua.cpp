#include "viewer.hpp"

namespace demo {

void viewer::init_lua() {
  lua.open_libraries(       //
      sol::lib::base,       //
      sol::lib::package,    //
      sol::lib::coroutine,  //
      sol::lib::string,     //
      sol::lib::os,         //
      sol::lib::math,       //
      sol::lib::table,      //
      sol::lib::io,         //
      sol::lib::debug,      //
      sol::lib::bit32,      //
      sol::lib::utf8);

  lua["quit"] = [this]() { done = true; };

  auto live_paths = lua["live_paths"].get_or_create<sol::table>();
  live_paths[sol::metatable_key] = lua.create_table_with("__call", [this] {
    const auto table =
        lua_live_paths |
        std::views::transform([](auto const& path) { return path.string(); }) |
        std::ranges::to<std::vector>();
    return sol::as_table(table);
  });
  live_paths["assign"] = [this](sol::table const& table) {
    lua_live_paths.clear();
    for (auto const& [k, v] : table) {
      lua_live_paths.push_back(
          weakly_canonical(std::filesystem::path(v.as<std::string_view>())));
    }
  };
  live_paths["append"] = [this](sol::table const& table) {
    for (auto const& [k, v] : table) {
      lua_live_paths.push_back(
          weakly_canonical(std::filesystem::path(v.as<std::string_view>())));
    }
  };
  live_paths["clear"] = [this] { lua_live_paths.clear(); };

  lua["print_live_paths"] = [this]() {
    for (auto const& path : lua_live_paths) std::println("{}", path.string());
  };

  lua["scene_from_file"] = [](std::string_view path) {
    return scene_from(std::filesystem::path{path});
  };

  lua["show"] = [this](struct scene const& scene) { show(scene); };

  lua["set_clear_color"] = [this](float r, float g, float b) {
    glClearColor(r, g, b, 1.0f);
  };

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
