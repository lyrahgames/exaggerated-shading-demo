#pragma once
#include "program.hpp"

namespace demo::opengl {

struct source {
  std::filesystem::path path{};

  auto string() const {
    const auto src = string_from_file(path);
    // std::println("{}", src);
    return src;
  }
};

struct shader_build_rule {
  GLenum shader_type;
  std::vector<source> sources{};

  auto build() const {
    shader obj{shader_type};
    obj.set_sources(sources |
                    std::ranges::views::transform(
                        [](auto const& src) { return src.string(); }));
    std::println("{}", obj.source());
    obj.compile();
    assert(obj.compiled());
    return obj;
  }
};

struct program_build_rule {
  std::vector<shader_build_rule> shaders{};

  auto build() const {
    program exe{};
    auto obj =
        shaders |
        std::views::transform([](auto const& rule) { return rule.build(); }) |
        std::ranges::to<std::vector<shader>>();
    exe.link_range(obj);
    return exe;
  }
};

}  // namespace demo::opengl
