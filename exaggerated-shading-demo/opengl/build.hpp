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

  struct build_status {
    std::vector<shader> shaders{};
    std::string linkage{};
    std::string validation{};
    bool success{};

    void print() const {
      std::println("Program Build Status: {}", success ? "Success" : "Failed");
      for (shader_view obj : shaders) obj.debug_print();
      if (not linkage.empty()) std::println("Linkage:\n{}", linkage);
      if (not validation.empty()) std::println("Validation:\n{}", validation);
      std::println();
    }
  };

  auto build() const {
    program exe{};
    build_status status{};
    status.shaders.assign_range(
        shaders |
        std::views::transform([](auto&& rule) { return rule.build(); }));
    if (std::ranges::all_of(status.shaders,
                            [](shader_view obj) { return obj.compiled(); })) {
      exe.link_range(status.shaders);
      status.linkage = exe.info_log();
      if (exe.linked()) {
        exe.validate();
        status.validation = exe.info_log();
        if (exe.validated()) status.success = true;
      }
    }
    return std::tuple{std::move(exe), std::move(status)};
  }
};

}  // namespace demo::opengl
