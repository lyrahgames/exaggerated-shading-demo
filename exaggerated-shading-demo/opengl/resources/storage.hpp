#pragma once
#include "buffer_base.hpp"

namespace demo::opengl {

struct storage_base : buffer_base {
  using base = buffer_base;

  ///
  ///
  static auto create(size_type size = 0, const void* data = nullptr) noexcept
      -> storage_base {
    auto id = storage_base{base::create()};
    if (size > 0) glNamedBufferStorage(id.native_handle(), size, data, {});
    return id;
  }

  ///
  ///
  template <typename type>
  static auto create(size_type size, const type* data) noexcept
      -> storage_base {
    return create(size * sizeof(type), static_cast<const void*>(data));
  }

  ///
  ///
  static auto create(std::ranges::contiguous_range auto&& range) noexcept
      -> storage_base {
    return create(std::ranges::size(range), std::ranges::data(range));
  }
};

///
///
STRICT_FINAL_USING(storage, unique_resource<storage_base>);

///
///
STRICT_FINAL_USING(storage_view, resource_view<storage>);

}  // namespace demo::opengl
