#pragma once
#include "defaults.hpp"

namespace demo::opengl {

///
///
struct buffer_base : identifier {
  using base = identifier;
  using base::base;  // Allow construction from `native_handle_type`.

  /// The default constructor obtains a valid OpenGL buffer handle.
  ///
  static auto create() noexcept -> buffer_base {
    // With 'glCreateBuffers', created buffer objects
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
    auto handle = resource.native_handle();
    glDeleteBuffers(1, &handle);
  }

  /// Check whether the identifier currently corresponds
  /// to an initialized OpenGL buffer object.
  ///
  bool valid() const noexcept { return glIsBuffer(native_handle()) == GL_TRUE; }

  /// Retrieve the buffer's size in bytes from the driver.
  ///
  auto size() const noexcept -> size_type {
    GLint64 s;
    glGetNamedBufferParameteri64v(native_handle(), GL_BUFFER_SIZE, &s);
    return static_cast<size_type>(s);
  }

  /// Check whether the buffer is immutable.
  ///
  bool immutable() const noexcept {
    GLint data;
    glGetNamedBufferParameteriv(native_handle(), GL_BUFFER_IMMUTABLE_STORAGE,
                                &data);
    return static_cast<GLboolean>(data) == GL_TRUE;
  }

  /// Check whether the buffer is currently mapped.
  ///
  bool mapped() const noexcept {
    GLint data;
    glGetNamedBufferParameteriv(native_handle(), GL_BUFFER_MAPPED, &data);
    return static_cast<GLboolean>(data) == GL_TRUE;
  }

  ///
  ///
  // void bind(GLenum target) const noexcept {
  //   glBindBuffer(target, native_handle());
  // }

  ///
  ///
  // void bind_base(GLenum target, uint index) const noexcept {
  //   glBindBufferBase(target, index, native_handle());
  // }
};

}  // namespace demo::opengl
