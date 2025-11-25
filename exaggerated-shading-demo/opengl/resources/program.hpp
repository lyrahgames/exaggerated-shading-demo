#pragma once
#include "shader.hpp"

namespace demo::opengl {

struct program;
struct program_view;
struct program_base;

///
///
template <typename type>
concept program_like = similar_to<type, program_base>;

///
///
struct program_base : identifier {
  using base = identifier;
  using base::base;  // Allow construction from `native_handle_type`.

  using view_type = program_view;

  /// The default constructor obtains a valid OpenGL program handle.
  /// If acquiring the handle fails, it throws a 'resource_acquisition_error'.
  ///
  static auto create() -> program_base {
    return program_base{glCreateProgram()};
  }

  /// The destructor does not throw and marks the OpenGL program handle for deletion.
  ///
  static void destroy(program_base& resource) noexcept {
    // A value of 0 for 'handle' will be silently ignored.
    glDeleteProgram(resource.native_handle());
  }

  /// Checks whether the shader program represents a valid OpenGL shader program.
  ///
  bool valid() const noexcept {
    return glIsProgram(native_handle()) == GL_TRUE;
  }

  /// Checks whether the shader program is valid and has been successfully linked.
  ///
  bool linked() const noexcept {
    // 'glGetProgramiv' can generate the errors 'GL_INVALID_VALUE' or
    // 'GL_INVALID_OPERATION' if the provided program handle is invalid.
    // If an error occurs, the value of 'status' remains unchanged.
    // Note that newly created programs are not linked by default.
    // Therefore, 'status' can only be 'GL_TRUE' if the program handle
    // is valid and the program has been successfully linked.
    auto status = static_cast<GLint>(GL_FALSE);
    glGetProgramiv(native_handle(), GL_LINK_STATUS, &status);
    return static_cast<GLboolean>(status) == GL_TRUE;
  }

  bool validated() const noexcept {
    auto status = static_cast<GLint>(GL_FALSE);
    glGetProgramiv(native_handle(), GL_VALIDATE_STATUS, &status);
    return static_cast<GLboolean>(status) == GL_TRUE;
  }

  /// This function returns the info log for the specified shader program.
  /// The info log for a program is modified
  /// when the program is linked or validated.
  ///
  auto info_log() const -> std::string {
    // Retrieve the size in bytes of the info log.
    GLint size;
    glGetProgramiv(native_handle(), GL_INFO_LOG_LENGTH, &size);
    // Use the size to load and return the content of the info log.
    std::string info{};
    info.resize(size);
    glGetProgramInfoLog(native_handle(), info.size(), nullptr, info.data());
    return info;
  }

  void validate() const noexcept { glValidateProgram(native_handle()); }

  void link(shader_like auto&&... obj) const noexcept {
    // link cannot throw. RAII/scope utility does not need to be applied.
    (glAttachShader(native_handle(), obj.native_handle()), ...);
    glLinkProgram(native_handle());
    (glDetachShader(native_handle(), obj.native_handle()), ...);
  }

  void link_range(std::ranges::bidirectional_range auto&& obj) const noexcept {
    std::ranges::for_each(obj, [this](auto&& obj) {
      glAttachShader(native_handle(), obj.native_handle());
    });
    glLinkProgram(native_handle());
    std::ranges::for_each(obj | std::views::reverse, [this](auto&& obj) {
      glDetachShader(native_handle(), obj.native_handle());
    });
  }

  template <shader_like... shader_types>
  struct build_status {
    using tuple = std::tuple<shader_types...>;
    tuple shaders{};
    std::string linkage{};
    std::string validation{};
    bool success{};

    constexpr build_status(shader_like auto&&... args) noexcept
        : shaders{std::forward<decltype(args)>(args)...} {}

    void print() const {
      std::println("Program Build Status: {}", success ? "Success" : "Failed");
      std::apply(
          [](auto&&... obj) {
            const auto info = [](auto&& obj) { obj.debug_print(); };
            (info(obj), ...);
          },
          shaders);
      if (not linkage.empty()) std::println("Linkage:\n{}", linkage);
      if (not validation.empty()) std::println("Validation:\n{}", validation);
      std::println();
    }
  };
  template <shader_like... types>
  build_status(types&&...) -> build_status<opengl::decay<types>...>;

  auto build(shader_like auto&&... obj) const noexcept {
    // build_status status{std::tuple{fwd(std::forward<decltype(obj)>(obj))...}};
    // build_status status{fwd(std::forward<decltype(obj)>(obj))...};
    build_status status{opengl::forward<decltype(obj)>(obj)...};

    std::apply([](auto&&... obj) { (obj.compile(), ...); }, status.shaders);
    const auto compiled = std::apply(
        [](auto&&... obj) { return (... && obj.compiled()); }, status.shaders);
    if (not compiled) return status;

    std::apply([this](auto&&... obj) { link(obj...); }, status.shaders);
    status.linkage = info_log();
    if (not linked()) return status;

    validate();
    status.validation = info_log();
    if (not validated()) return status;

    status.success = true;
    return status;
  }

  void use() const noexcept { glUseProgram(native_handle()); }

 private:
  auto uniform_location(czstring name) noexcept {
    return glGetUniformLocation(native_handle(), name);
  }

  auto valid_uniform_location(czstring name) {
    const auto result = uniform_location(name);
    if (result == -1) throw invalid_uniform_identifier{name};
    return result;
  }

 public:
  void set(czstring name, auto&& value) {
    try_set(valid_uniform_location(name), std::forward<decltype(value)>(value));
  }

  void try_set(czstring name, auto&& value) noexcept {
    try_set(uniform_location(name), std::forward<decltype(value)>(value));
  }

 private:
  void try_set(GLint location, float value) noexcept {
    glProgramUniform1f(native_handle(), location, value);
  }
  void try_set(GLint location, float x, float y) noexcept {
    glProgramUniform2f(native_handle(), location, x, y);
  }
  void try_set(GLint location, float x, float y, float z) noexcept {
    glProgramUniform3f(native_handle(), location, x, y, z);
  }
  void try_set(GLint location, float x, float y, float z, float w) noexcept {
    glProgramUniform4f(native_handle(), location, x, y, z, w);
  }

  void try_set(GLint location, GLint value) noexcept {
    glProgramUniform1i(native_handle(), location, value);
  }
  void try_set(GLint location, GLint x, GLint y) noexcept {
    glProgramUniform2i(native_handle(), location, x, y);
  }
  void try_set(GLint location, GLint x, GLint y, GLint z) noexcept {
    glProgramUniform3i(native_handle(), location, x, y, z);
  }
  void try_set(GLint location, GLint x, GLint y, GLint z, GLint w) noexcept {
    glProgramUniform4i(native_handle(), location, x, y, z, w);
  }

  void try_set(GLint location, GLuint value) noexcept {
    glProgramUniform1ui(native_handle(), location, value);
  }
  void try_set(GLint location, GLuint x, GLuint y) noexcept {
    glProgramUniform2ui(native_handle(), location, x, y);
  }
  void try_set(GLint location, GLuint x, GLuint y, GLuint z) noexcept {
    glProgramUniform3ui(native_handle(), location, x, y, z);
  }
  void try_set(GLint location,
               GLuint x,
               GLuint y,
               GLuint z,
               GLuint w) noexcept {
    glProgramUniform4ui(native_handle(), location, x, y, z, w);
  }

  void try_set(GLint location, const vec2& value) noexcept {
    try_set(location, value.x, value.y);
  }
  void try_set(GLint location, const vec3& value) noexcept {
    try_set(location, value.x, value.y, value.z);
  }
  void try_set(GLint location, const vec4& value) noexcept {
    try_set(location, value.x, value.y, value.z, value.w);
  }

  void try_set(GLint location, const ivec2& value) noexcept {
    try_set(location, value.x, value.y);
  }
  void try_set(GLint location, const ivec3& value) noexcept {
    try_set(location, value.x, value.y, value.z);
  }
  void try_set(GLint location, const ivec4& value) noexcept {
    try_set(location, value.x, value.y, value.z, value.w);
  }

  void try_set(GLint location, const uvec2& value) noexcept {
    try_set(location, value.x, value.y);
  }
  void try_set(GLint location, const uvec3& value) noexcept {
    try_set(location, value.x, value.y, value.z);
  }
  void try_set(GLint location, const uvec4& value) noexcept {
    try_set(location, value.x, value.y, value.z, value.w);
  }

  void try_set(GLint location, const mat2& value) noexcept {
    glProgramUniformMatrix2fv(native_handle(), location, 1, GL_FALSE,
                              value_ptr(value));
  }
  void try_set(GLint location, const mat3& value) noexcept {
    glProgramUniformMatrix3fv(native_handle(), location, 1, GL_FALSE,
                              value_ptr(value));
  }
  void try_set(GLint location, const mat4& value) noexcept {
    glProgramUniformMatrix4fv(native_handle(), location, 1, GL_FALSE,
                              value_ptr(value));
  }

  void try_set(GLint location, const mat2x3& value) noexcept {
    glProgramUniformMatrix2x3fv(native_handle(), location, 1, GL_FALSE,
                                value_ptr(value));
  }
  void try_set(GLint location, const mat3x2& value) noexcept {
    glProgramUniformMatrix3x2fv(native_handle(), location, 1, GL_FALSE,
                                value_ptr(value));
  }

  void try_set(GLint location, const mat2x4& value) noexcept {
    glProgramUniformMatrix2x4fv(native_handle(), location, 1, GL_FALSE,
                                value_ptr(value));
  }
  void try_set(GLint location, const mat4x2& value) noexcept {
    glProgramUniformMatrix4x2fv(native_handle(), location, 1, GL_FALSE,
                                value_ptr(value));
  }

  void try_set(GLint location, const mat3x4& value) noexcept {
    glProgramUniformMatrix3x4fv(native_handle(), location, 1, GL_FALSE,
                                value_ptr(value));
  }
  void try_set(GLint location, const mat4x3& value) noexcept {
    glProgramUniformMatrix4x3fv(native_handle(), location, 1, GL_FALSE,
                                value_ptr(value));
  }

  void try_get(GLint location, float& value) noexcept {
    glGetnUniformfv(native_handle(), location, sizeof(value), &value);
  }
  void try_get(GLint location, vec2& value) noexcept {
    glGetnUniformfv(native_handle(), location, sizeof(value), value_ptr(value));
  }
};

///
///
STRICT_FINAL_USING(program, unique_resource<program_base>);

///
///
STRICT_FINAL_USING(program_view, resource_view<program>);

/// Free Factory Function for Shader Program Building
/// Given shaders with source code, this functions compiles, links, and validates.
/// All errors and warnings potentially arising will be returned as status.
///
auto program_build(shader_like auto&&... obj) {
  program exe{};
  auto status = exe.build(std::forward<decltype(obj)>(obj)...);
  return std::tuple{std::move(exe), std::move(status)};
}

}  // namespace demo::opengl
