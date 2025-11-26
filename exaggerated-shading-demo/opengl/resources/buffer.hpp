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

///
///
template <typename type>
concept buffer_identifier_like = std::derived_from<type, buffer_identifier> &&
                                 (not owning_resource_like<type>);

///
///
template <typename type>
concept buffer_span_like =
    buffer_identifier_like<typename type::buffer_type> && requires(type value) {
      { value.buffer() } -> std::derived_from<buffer_identifier>;
      { value.offset() } -> std::convertible_to<offset_type>;
      { value.size() } -> std::convertible_to<size_type>;
    };

///
///
template <buffer_identifier_like type>
struct basic_buffer_span {
  using buffer_type = type;

  constexpr basic_buffer_span(buffer_type data,
                              offset_type offset,
                              size_type size) noexcept
      : _buffer{data}, _offset{offset}, _size{size} {}

  constexpr basic_buffer_span(
      buffer_type data,
      std::pair<offset_type, offset_type> range) noexcept
      : _buffer{data},
        _offset{range.first},
        _size{range.second - range.first} {}

  constexpr basic_buffer_span(buffer_type data, offset_type first = 0) noexcept
      : basic_buffer_span{data, {first, data.size()}} {}

  template <buffer_identifier_like cast_buffer>
    requires std::derived_from<buffer_type, cast_buffer>
  constexpr operator basic_buffer_span<cast_buffer>() noexcept {
    return {_buffer, _offset, _size};
  }

  constexpr auto offset() const noexcept -> offset_type { return _offset; }
  constexpr auto size() const noexcept -> size_type { return _size; }
  constexpr auto buffer() const noexcept -> buffer_type { return _buffer; }

 private:
  buffer_type _buffer;
  offset_type _offset;
  size_type _size;
};

///
///
using buffer_span = basic_buffer_span<buffer_identifier>;

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
