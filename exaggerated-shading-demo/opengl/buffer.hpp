#pragma once
#include "defaults.hpp"

namespace demo::opengl {

///
///
struct buffer_base : object {
  /// Base Type and Constructors
  ///
  using base = object;
  using base::base;

  /// The default constructor obtains a valid OpenGL buffer handle.
  /// If acquiring the handle fails, it throws a 'resource_acquisition_error'.
  ///
  static auto create() -> buffer_base {
    // With 'glCreateBuffers', create buffer objects
    // are already bound to an unspecified target.
    native_handle_type handle;
    glCreateBuffers(1, &handle);
    return buffer_base{handle};
  }

  /// After a buffer object is deleted, it has no contents,
  /// and its name is free for reuse (for example by glGenBuffers).
  /// If a buffer object that is currently bound is deleted, the
  /// binding reverts to 0 (the absence of any buffer object).
  ///
  static void destroy(buffer_base& resource) noexcept {
    // Silently ignores zero and names that do
    // not correspond to existing buffer objects.
    glDeleteBuffers(1, &resource.handle);
  }

  ///
  ///
  bool valid() const noexcept { return glIsBuffer(handle) == GL_TRUE; }

  ///
  ///
  auto size() const noexcept -> size_t {
    GLint s;
    glGetNamedBufferParameteriv(handle, GL_BUFFER_SIZE, &s);
    return s;
  }
};

///
///
STRICT_FINAL_USING(buffer, unique<buffer_base>);

///
///
STRICT_FINAL_USING(buffer_view, view<buffer>);

}  // namespace demo::opengl
