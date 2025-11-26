#pragma once
#include "buffer.hpp"
#include "defaults.hpp"

namespace demo::opengl {

// To-Do
//
// void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer);
// void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index);
// void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index);
// void glGetVertexArrayIndexed64iv(GLuint vaobj,
//                                  GLuint index,
//                                  GLenum pname,
//                                  GLint64* param);
// void glGetVertexArrayIndexediv(GLuint vaobj,
//                                GLuint index,
//                                GLenum pname,
//                                GLint* param);
// void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint* param);
// generic vertex attributes
// void glVertexArrayAttribBinding(GLuint vaobj,
//                                 GLuint attribindex,
//                                 GLuint bindingindex);
// void glVertexArrayAttribFormat(GLuint vaobj,
//                                GLuint attribindex,
//                                GLint size,
//                                GLenum type,
//                                GLboolean normalized,
//                                GLuint relativeoffset);
// void glVertexArrayAttribIFormat(GLuint vaobj,
//                                 GLuint attribindex,
//                                 GLint size,
//                                 GLenum type,
//                                 GLuint relativeoffset);
// void glVertexArrayAttribLFormat(GLuint vaobj,
//                                 GLuint attribindex,
//                                 GLint size,
//                                 GLenum type,
//                                 GLuint relativeoffset);
// void glVertexArrayBindingDivisor(GLuint vaobj,
//                                  GLuint bindingindex,
//                                  GLuint divisor);

/// Resource base that identifies an OpenGL vertex array object.
/// Provides factory functions for destruction and default construction
/// as well as basic API calls specific to vertex array objects.
///
struct vertex_array_identifier : identifier {
  using base = identifier;
  using base::base;  // Allow construction from `native_handle_type`.

  /// Factory for Default Constructor
  ///
  static auto create() -> vertex_array_identifier {
    native_handle_type handle;
    glCreateVertexArrays(1, &handle);
    return vertex_array_identifier{handle};
  }

  /// Factory for Destruction
  ///
  static void destroy(vertex_array_identifier& resource) noexcept {
    auto handle = resource.native_handle();
    glDeleteVertexArrays(1, &handle);  // `0` is silently ignored.
  }

  /// Checks whether the handle represents a valid OpenGL vertex array object.
  ///
  bool valid() const noexcept {
    return glIsVertexArray(native_handle()) == GL_TRUE;
  }

  /// Bind the vertex array object represented by the handle.
  ///
  void bind() const noexcept { glBindVertexArray(native_handle()); }

  /// Set the element array buffer binding of
  /// the corresponding vertex array object.
  ///
  void set_elements(buffer_identifier buffer) const noexcept {
    glVertexArrayElementBuffer(native_handle(), buffer.native_handle());
  }
  // void set_elements(buffer_view_like auto buffer) const noexcept {
  //   glVertexArrayElementBuffer(native_handle(), buffer.native_handle());
  // }
  // void set_elements(owning_buffer_like auto const& buffer) const noexcept {
  //   set_elements(resource_view_cast(buffer));
  // }
};

///
///
STRICT_FINAL_USING(vertex_array, unique_resource<vertex_array_identifier>);

///
///
STRICT_FINAL_USING(vertex_array_view, resource_view<vertex_array>);

}  // namespace demo::opengl
