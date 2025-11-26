#pragma once
#include "buffer.hpp"

namespace demo::opengl {

struct legacy_buffer_identifier : buffer_identifier {
  using base = buffer_identifier;

  /// Default Constructor Forwarded From Resource Base
  ///
  static auto create() noexcept -> legacy_buffer_identifier {
    return legacy_buffer_identifier{base::create()};
  }

  /// Construction from C-Style Array
  ///
  static auto create(size_type size, const auto* data) noexcept
      -> legacy_buffer_identifier {
    auto id = create();
    id.assign(size, data);
    return id;
  }

  /// Construction from Contiguous Ranges
  ///
  static auto create(std::ranges::contiguous_range auto&& range) noexcept
      -> legacy_buffer_identifier {
    auto id = create();
    id.assign(std::forward<decltype(range)>(range));
    return id;
  }

  /// Creates and initializes a buffer object's data store
  /// with the given untyped data and its size in bytes.
  ///
  void assign(size_type size, const void* data) const noexcept {
    // Expects: size >= 0
    constexpr auto usage = GL_STATIC_DRAW;
    glNamedBufferData(native_handle(), size, data, usage);
  }

  /// Creates and initializes a buffer object's data store with
  /// the given C-style array of data and its element count.
  ///
  template <typename type>
  void assign(size_type size, const type* data) const noexcept {
    assign(size * sizeof(type), static_cast<const void*>(data));
  }

  /// Creates and initializes a buffer object's data store with any given
  /// value whose type fulfills the requirements of a contiguous range.
  ///
  void assign(std::ranges::contiguous_range auto&& range) const noexcept {
    assign(std::ranges::size(range), std::ranges::data(range));
  }
};

///
///
STRICT_FINAL_USING(legacy_buffer, unique_resource<legacy_buffer_identifier>);

///
///
STRICT_FINAL_USING(legacy_buffer_view, resource_view<legacy_buffer>);

///
///
constexpr auto resource_view_cast(legacy_buffer const& data) noexcept
    -> legacy_buffer_view {
  return static_cast<legacy_buffer_view>(data);
}

}  // namespace demo::opengl
