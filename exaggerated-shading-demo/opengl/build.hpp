#pragma once
#include "program.hpp"

namespace demo::opengl {

struct source {
  std::filesystem::path path{};

  auto string() const { return string_from_file(path); }
};

struct shader_build_rule {
  GLenum shader_type;
  std::vector<source> sources{};

  auto build() const {
    shader obj{shader_type};
    obj.set_sources(sources | std::views::transform(
                                  [](auto&& src) { return src.string(); }));
    obj.compile();
    return obj;
  }
};

struct program_build_rule {
  std::vector<shader_build_rule> shaders{};

  auto build() const {
    program exe{};
    auto obj = shaders |
               std::views::transform([](auto&& rule) { return rule.build(); }) |
               std::ranges::to<std::vector>();
    exe.link_range(obj);
    return exe;
  }
};

}  // namespace demo::opengl
