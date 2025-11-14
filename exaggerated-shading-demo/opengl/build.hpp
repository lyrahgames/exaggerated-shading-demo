#pragma once
#include "program.hpp"

namespace demo::opengl {

struct source {
  std::filesystem::path path{};
  mutable std::filesystem::file_time_type time{};

  auto string() const {
    time = last_write_time(path);
    return string_from_file(path);
  }

  bool check() const { return time < last_write_time(path); }
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

auto vs(auto&&... src) -> shader_build_rule {
  return shader_build_rule{
      GL_VERTEX_SHADER,
      std::vector<source>{source{std::forward<decltype(src)>(src)}...}};
}

auto tcs(auto&&... src) -> shader_build_rule {
  return shader_build_rule{
      GL_TESS_CONTROL_SHADER,
      std::vector<source>{source{std::forward<decltype(src)>(src)}...}};
}

auto tes(auto&&... src) -> shader_build_rule {
  return shader_build_rule{
      GL_TESS_EVALUATION_SHADER,
      std::vector<source>{source{std::forward<decltype(src)>(src)}...}};
}

auto gs(auto&&... src) -> shader_build_rule {
  return shader_build_rule{
      GL_GEOMETRY_SHADER,
      std::vector<source>{source{std::forward<decltype(src)>(src)}...}};
}

auto fs(auto&&... src) -> shader_build_rule {
  return shader_build_rule{
      GL_FRAGMENT_SHADER,
      std::vector<source>{source{std::forward<decltype(src)>(src)}...}};
}

auto cs(auto&&... src) -> shader_build_rule {
  return shader_build_rule{
      GL_COMPUTE_SHADER,
      std::vector<source>{source{std::forward<decltype(src)>(src)}...}};
}

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

  bool check() const {
    for (auto&& s : shaders)
      for (auto&& src : s.sources)
        if (src.check()) return true;
    return false;
  }
};

// template <typename type>
// struct basic_program_target : type {
//   using base = type;
// };

struct program_target {
  program_build_rule rule{};
  program shader{};

  operator program_view() const noexcept { return shader; }

  void build() {
    auto [exe, status] = rule.build();
    status.print();
    if (not status.success) return;
    shader = std::move(exe);
  }

  bool update() {
    if (not rule.check()) return false;
    build();
    return true;
  }

  void try_set(czstring name, auto&& value) {
    shader.try_set(name, std::forward<decltype(value)>(value));
  }
};

struct build_system {
  std::map<std::string, std::shared_ptr<program_target>> targets{};

  [[nodiscard]] auto target(auto&& name) -> std::shared_ptr<program_target> {
    auto it = targets.find(name);
    if (it != targets.end()) return it->second;
    return targets[std::forward<decltype(name)>(name)] =
               std::make_shared<program_target>();
  }

  [[nodiscard]] auto target(auto&& name, program_build_rule&& rule)
      -> std::shared_ptr<program_target> {
    auto ptr = target(std::forward<decltype(name)>(name));
    ptr->rule = std::move(rule);
    ptr->build();
    return ptr;
  }

  void update() {
    for (auto& [name, t] : targets) {
      if (t->update()) std::println("Program '{}' updated.", name);
    }
  }
};

}  // namespace demo::opengl
