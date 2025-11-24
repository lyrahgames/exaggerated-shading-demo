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
  void bind(GLenum target) const noexcept {
    glBindBuffer(target, native_handle());
  }

  ///
  ///
  void bind_base(GLenum target, uint index) const noexcept {
    glBindBufferBase(target, index, native_handle());
  }

  ///
  ///
  auto size() const noexcept -> size_type {
    GLint64 s = 0;
    glGetNamedBufferParameteri64v(handle, GL_BUFFER_SIZE, &s);
    return s;
  }

  ///
  ///
  void immutable_assign(size_type size,
                        const void* data,
                        BufferStorageMask flags = {}) const noexcept {
    glNamedBufferStorage(native_handle(), size, data, flags);
  }
  void immutable_assign(size_type size,
                        const auto* data,
                        BufferStorageMask flags = {}) const noexcept {
    immutable_assign(size * sizeof(data[0]), static_cast<const void*>(data),
                     flags);
  }
  void immutable_assign(std::ranges::contiguous_range auto&& range,
                        BufferStorageMask flags = {}) const noexcept {
    immutable_assign(std::ranges::size(range), std::ranges::data(range), flags);
  }

  ///
  ///
  void assign(const void* data, size_type size) const noexcept {
    glNamedBufferData(native_handle(), size, data, GL_STATIC_DRAW);
  }

  ///
  ///
  void assign(const auto* data, size_type size) const noexcept {
    assign(static_cast<const void*>(data), size * sizeof(data[0]));
  }

  ///
  ///
  void assign(const std::ranges::contiguous_range auto& range) const noexcept {
    assign(ranges::data(range), ranges::size(range));
  }

  ///
  ///
  template <transferable type>
    requires(!std::ranges::contiguous_range<type>)  //
  void assign(const type& value) const noexcept {
    assign(&value, 1);
  }

  // void alloc(size_type size) const noexcept {
  //   assign(static_cast<const void*>(nullptr), size);
  // }

  // void write(const void* data,
  //            size_type size,
  //            size_type offset = 0) const noexcept {
  //   // assert(offset + size <= self.size());
  //   glBufferSubData(buffer_type, offset, size, data);
  // }

  // void write(const auto* data,
  //            size_type size,
  //            size_type offset = 0) const noexcept {
  //   write(static_cast<const void*>(data),  //
  //         size * sizeof(data[0]), offset);
  // }

  // void write(const std::ranges::contiguous_range auto& range,
  //            size_type offset = 0) const noexcept {
  //   write(ranges::data(range), ranges::size(range), offset);
  // }

  // void write(const transferable auto& v, size_type offset = 0) const noexcept {
  //   write(&v, 1, offset);
  // }
};

///
///
STRICT_FINAL_USING(buffer, unique<buffer_base>);

///
///
STRICT_FINAL_USING(buffer_view, view<buffer>);

}  // namespace demo::opengl
