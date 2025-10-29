#pragma once
#include "buffer.hpp"
#include "defaults.hpp"

namespace demo::opengl {

template <typename type>
concept attribute_format = std::is_empty_v<type> && requires {
  { type::size } -> std::convertible_to<GLint>;
  { type::type } -> std::convertible_to<GLenum>;
};

template <typename type>
concept float32_attribute_format =
    attribute_format<type> && (type::type == GL_FLOAT);

template <typename T>
struct attribute {};

template <>
struct attribute<float> {
  static constexpr GLint size = 1;
  static constexpr GLenum type = GL_FLOAT;
};

template <>
struct attribute<vec2> {
  static constexpr GLint size = 2;
  static constexpr GLenum type = GL_FLOAT;
};

template <>
struct attribute<vec3> {
  static constexpr GLint size = 3;
  static constexpr GLenum type = GL_FLOAT;
};

template <>
struct attribute<vec4> {
  static constexpr GLint size = 4;
  static constexpr GLenum type = GL_FLOAT;
};

template <>
struct attribute<double> {
  static constexpr GLint size = 1;
  static constexpr GLenum type = GL_DOUBLE;
};

template <>
struct attribute<dvec2> {
  static constexpr GLint size = 2;
  static constexpr GLenum type = GL_DOUBLE;
};

template <typename type>
inline constexpr auto attr = attribute<type>{};

///
///
struct vertex_array_base : object {
  using base = object;
  using base::base;

  /// The default constructor obtains a valid OpenGL vertex array handle.
  /// If acquiring the handle fails, it throws a 'resource_acquisition_error'.
  ///
  static auto create() -> vertex_array_base {
    native_handle_type handle;
    // glGenVertexArrays(1, &handle);
    // vertex_array_base result{handle};
    // // If array is a name returned by `glGenVertexArrays`, by that
    // // has not yet been bound through a call to `glBindVertexArray`,
    // // then the name is not a vertex array object
    // // and `glIsVertexArray` returns `GL_FALSE`.
    // result.bind();
    glCreateVertexArrays(1, &handle);
    return vertex_array_base{handle};
  }

  /// Once a vertex array object is deleted it has no contents
  /// and its name is again unused.
  /// If a vertex array object that is currently bound is deleted,
  /// the binding for that object reverts to zero and the default
  /// vertex array becomes current.
  ///
  static void destroy(vertex_array_base& resource) noexcept {
    // Unused names in arrays are silently ignored, as is the value zero.
    glDeleteVertexArrays(1, &resource.handle);
  }

  /// Checks whether the handle represents a valid OpenGL vertex array object.
  ///
  bool valid() const noexcept {
    return glIsVertexArray(native_handle()) == GL_TRUE;
  }

  /// Bind the vertex array object represented by the handle.
  ///
  void bind() const noexcept { glBindVertexArray(native_handle()); }

  ///
  ///
  void set_vertex_buffer(GLuint location,
                         buffer_view vertices,
                         GLintptr offset,
                         GLsizei stride) const noexcept {
    glVertexArrayVertexBuffer(native_handle(), location,  //
                              vertices.native_handle(), offset, stride);
  }

  ///
  ///
  void set_element_buffer(buffer_view elements) const noexcept {
    glVertexArrayElementBuffer(native_handle(), elements.native_handle());
  }

  template <attribute_format format>
    requires(format::type != GL_FLOAT) && (format::type != GL_DOUBLE)
  void set_float32_attribute(GLuint location,
                             format,
                             GLuint offset,
                             GLboolean normalized = GL_FALSE) const noexcept {
    glVertexArrayAttribFormat(native_handle(), location, format::size,
                              format::type, normalized, offset);
  }

  template <attribute_format format>
    requires(format::type == GL_FLOAT) || (format::type == GL_DOUBLE)
  void set_float32_attribute(GLuint location,
                             format,
                             GLuint offset) const noexcept {
    glVertexArrayAttribFormat(native_handle(), location,  //
                              format::size, format::type, GL_FALSE, offset);
  }

  template <attribute_format format>
  void set_integer_attribute(GLuint location,
                             format,
                             GLuint offset) const noexcept {
    glVertexArrayAttribIFormat(native_handle(), location,  //
                               format::size, format::type, offset);
  }

  template <attribute_format format>
    requires(format::type == GL_DOUBLE)
  void set_float64_attribute(GLuint location,
                             format,
                             GLuint offset) const noexcept {
    glVertexArrayAttribLFormat(native_handle(), location,  //
                               format::size, GL_DOUBLE, offset);
  }

  template <attribute_format format>
  void set_attribute(GLuint location, format f, GLuint offset = 0) {
    if constexpr (format::type == GL_FLOAT)
      set_float32_attribute(location, f, offset);
    else if constexpr (format::type == GL_DOUBLE)
      set_float64_attribute(location, f, offset);
    else
      set_integer_attribute(location, f, offset);
  }

  void set_attribute_binding(GLuint attr, GLuint buffer) const noexcept {
    glVertexArrayAttribBinding(native_handle(), attr, buffer);
  }

  void enable_attribute(GLuint attr) const noexcept {
    glEnableVertexArrayAttrib(native_handle(), attr);
  }
  void disable_attribute(GLuint attr) const noexcept {
    glDisableVertexArrayAttrib(native_handle(), attr);
  }
};

///
///
STRICT_FINAL_USING(vertex_array, unique<vertex_array_base>);

///
///
STRICT_FINAL_USING(vertex_array_view, view<vertex_array>);

}  // namespace demo::opengl
