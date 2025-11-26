#pragma once
#include "defaults.hpp"

namespace demo::opengl {

/// Resource base that identifies an OpenGL buffer object.
/// Provides factory functions for destruction and default construction
/// as well as basic API calls specific to buffer objects.
///
struct buffer_identifier : identifier {
  using base = identifier;
  using base::base;  // Allow construction from `native_handle_type`.

  /// Factory for Default Constructor
  ///
  static auto create() noexcept -> buffer_identifier {
    native_handle_type handle;
    glCreateBuffers(1, &handle);
    return buffer_identifier{handle};
  }

  /// Factory for Destruction
  ///
  static void destroy(buffer_identifier& resource) noexcept {
    auto handle = resource.native_handle();
    glDeleteBuffers(1, &handle);  // `0` is silently ignored.
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

// ///
// ///
// template <typename type>
// concept buffer_like = similar_to<type, buffer_identifier>;

// ///
// ///
// template <typename type>
// concept buffer_view_like = buffer_like<type> && resource_view_like<type>;

// ///
// ///
// template <typename type>
// concept owning_buffer_like =
//     buffer_like<type> && (not resource_view_like<type>);

}  // namespace demo::opengl
