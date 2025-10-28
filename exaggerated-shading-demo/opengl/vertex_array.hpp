#pragma once
#include "defaults.hpp"

namespace demo::opengl {

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
};

///
///
STRICT_FINAL_USING(vertex_array, unique<vertex_array_base>);

///
///
STRICT_FINAL_USING(vertex_array_view, view<vertex_array>);

}  // namespace demo::opengl
