#pragma once
#include "defaults.hpp"

namespace demo::opengl {

struct shader;
struct shader_view;
struct shader_base;

///
///
template <typename type>
concept shader_like = similar_to<type, shader_base>;

///
///
constexpr auto shader_type_string(GLenum shader_type) -> czstring {
  switch (shader_type) {
    case GL_VERTEX_SHADER:
      return "vertex";
      break;

    case GL_TESS_CONTROL_SHADER:
      return "tessellation control";
      break;

    case GL_TESS_EVALUATION_SHADER:
      return "tessellation evaluation";
      break;

    case GL_GEOMETRY_SHADER:
      return "geometry";
      break;

    case GL_FRAGMENT_SHADER:
      return "fragment";
      break;

    case GL_COMPUTE_SHADER:
      return "compute";
      break;

    default:
      return "unknown";
  }
}

///
///
struct shader_base : identifier {
  using base = identifier;
  using base::base;  // Allow construction from `native_handle_type`.

  using view_type = shader_view;
  // static auto view() -> shader_view;

  static auto create(GLenum shader_type) noexcept -> shader_base {
    return shader_base{glCreateShader(shader_type)};
  }

  /// The destructor does not throw and marks the OpenGL shader handle for deletion.
  ///
  static void destroy(shader_base& resource) noexcept {
    // A value of 0 for 'handle' will be silently ignored.
    glDeleteShader(resource.native_handle());
  }

  /// Checks whether the shader object represents a valid OpenGL shader object.
  ///
  bool valid() const noexcept { return glIsShader(native_handle()) == GL_TRUE; }

  auto type() const noexcept -> GLenum {
    GLint t;
    glGetShaderiv(native_handle(), GL_SHADER_TYPE, &t);
    return static_cast<GLenum>(t);
  }

  auto type_string() const noexcept { return shader_type_string(type()); }

  /// Checks whether the shader object is valid and has been successfully compiled.
  ///
  bool compiled() const noexcept {
    // 'glGetShaderiv' can generate the errors 'GL_INVALID_VALUE' or
    // 'GL_INVALID_OPERATION' if the provided shader handle is invalid.
    // If an error occurs, the value of 'status' remains unchanged.
    // Note that newly created shader objects are not compiled by default.
    // Therefore, 'status' can only be 'GL_TRUE' if the shader handle is valid
    // and the shader has been successfully compiled.
    auto status = static_cast<GLint>(GL_FALSE);
    glGetShaderiv(native_handle(), GL_COMPILE_STATUS, &status);
    return static_cast<GLboolean>(status) == GL_TRUE;
  }

  /// This function returns the info log for the specified shader object.
  /// The info log for a shader object is modified when the shader is compiled.
  ///
  auto info_log() const -> std::string {
    // Retrieve the size in bytes of the info log.
    GLint size;
    glGetShaderiv(native_handle(), GL_INFO_LOG_LENGTH, &size);
    // Use the size to load and return the content of the info log.
    std::string info{};
    info.resize(size);
    glGetShaderInfoLog(native_handle(), info.size(), nullptr, info.data());
    return info;
  }

  // Helper Types and Functions for Source Code Strings
  //
  using GLstring = const GLchar*;
  static auto data(czstring str) noexcept -> GLstring { return str; }
  static auto size(czstring str) noexcept -> GLint { return -1; }
  static auto data(std::string_view str) noexcept -> GLstring {
    return str.data();
  }
  static auto size(std::string_view str) noexcept -> GLint {
    return str.size();
  }

  /// This function sets the source code in the shader object
  /// to the strings provided as variadic parameters, which can
  /// include any combination of standard string types.
  /// Any previously stored source code in the shader object is completely replaced.
  /// The source code strings are not scanned or parsed at this stage;
  /// they are simply copied into the shader object.
  /// This allows the application to safely free its own copy of
  /// the source code strings immediately after the function returns.
  ///
  void set_source(auto&&... str) const noexcept {
    // When 'glShaderSource' is called, OpenGL copies
    // the provided shader source code strings.
    // This allows the application to safely free its own copy of the
    // source code strings immediately after the function returns.
    constexpr GLsizei count = sizeof...(str);
    const std::array<GLint, count> lengths{size(str)...};
    const std::array<GLstring, count> strings{data(str)...};
    glShaderSource(native_handle(), count, strings.data(), lengths.data());
  }

  ///
  ///
  void set_sources(stable_string_range auto&& str) const {
    const GLsizei count = std::ranges::size(str);
    std::vector<GLint> lengths(count);
    std::vector<GLstring> strings(count);
    for (size_t i = 0; auto const& s : str) {
      lengths[i] = s.size();
      strings[i] = s.data();
      ++i;
    }
    glShaderSource(native_handle(), count, strings.data(), lengths.data());
  }
  //
  void set_sources(string_range auto&& str) const {
    set_sources(std::forward<decltype(str)>(str) |
                std::ranges::to<std::vector<std::string>>());
  }

  /// This function returns the concatenated source string
  /// for the specified shader object.
  ///
  auto source() const -> std::string {
    // Retrieve the size in bytes of the source string.
    GLint size;
    glGetShaderiv(native_handle(), GL_SHADER_SOURCE_LENGTH, &size);
    // Use the size to load and return the content of the source string.
    std::string src{};
    src.resize(size);
    glGetShaderSource(native_handle(), src.size(), nullptr, src.data());
    return src;
  }

  ///
  ///
  void compile() const noexcept { glCompileShader(native_handle()); }

  ///
  ///
  void compile(auto&&... src) const noexcept {
    set_source(std::forward<decltype(src)>(src)...);
    compile();
  }

  // debug status?
  // - source with lines
  // - log with lines, source location, severity, message
  // unreadable messages could be stored as `note`
  void debug_print() const noexcept {
    const auto str = info_log();
    if (compiled() && str.empty()) return;

    std::println("{} shader compile status: {}", type_string(),
                 compiled() ? "success" : "failed");

    if (str.empty()) return;

    const auto lines = line_offsets(str);

    const auto src = source();
    const auto src_lines = line_offsets(src);

    std::println("---");

    for (size_t i = 1; i < lines.size(); ++i) {
      const auto line = str.substr(lines[i - 1], lines[i] - lines[i - 1]);
      if (line.empty()) continue;
      const auto n = matched_line_number(line);
      std::print("{}", line);
      if (n > 0) {
        const auto first = std::max(n - 2, 1);
        const auto last = std::min(n + 2, int(src_lines.size()));
        for (size_t i = first; i < n; ++i) {
          std::print("  {:>3} |  {}", i,
                     std::string_view{src}.substr(
                         src_lines[i - 1], src_lines[i] - src_lines[i - 1]));
        }
        std::print("->{:>3} |  {}", n,
                   std::string_view{src}.substr(
                       src_lines[n - 1], src_lines[n] - src_lines[n - 1]));
        for (size_t i = n + 1; i < last; ++i) {
          std::print("  {:>3} |  {}", i,
                     std::string_view{src}.substr(
                         src_lines[i - 1], src_lines[i] - src_lines[i - 1]));
        }
      }
    }
    std::println("\n---");
    std::println();
  }
};

///
///
struct shader final : unique_resource<shader_base> {
  using base = unique_resource<shader_base>;

  shader(GLenum shader_type, auto&&... src) : base{shader_type} {
    if constexpr (sizeof...(src))
      set_source(std::forward<decltype(src)>(src)...);
  }
};

///
///
STRICT_FINAL_USING(shader_view, resource_view<shader>);

// /// Free Factory Function for Construction of Vertex Shader
// ///
// auto vs(auto&&... src) -> shader {
//   return shader{GL_VERTEX_SHADER, std::forward<decltype(src)>(src)...};
// }

// /// Free Factory Function for Construction of Tessellation Control Shader
// ///
// auto tcs(auto&&... src) -> shader {
//   return shader{GL_TESS_CONTROL_SHADER, std::forward<decltype(src)>(src)...};
// }

// /// Free Factory Function for Construction of Tessellation Evaluation Shader
// ///
// auto tes(auto&&... src) -> shader {
//   return shader{GL_TESS_EVALUATION_SHADER, std::forward<decltype(src)>(src)...};
// }

// /// Free Factory Function for Construction of Geometry Shader
// ///
// auto gs(auto&&... src) -> shader {
//   return shader{GL_GEOMETRY_SHADER, std::forward<decltype(src)>(src)...};
// }

// /// Free Factory Function for Construction of Fragment Shader
// ///
// auto fs(auto&&... src) -> shader {
//   return shader{GL_FRAGMENT_SHADER, std::forward<decltype(src)>(src)...};
// }

// /// Free Factory Function for Construction of Compute Shader
// ///
// auto cs(auto&&... src) -> shader {
//   return shader{GL_COMPUTE_SHADER, std::forward<decltype(src)>(src)...};
// }

}  // namespace demo::opengl
