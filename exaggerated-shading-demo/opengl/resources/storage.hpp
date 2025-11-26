#pragma once
#include "buffer.hpp"

namespace demo::opengl {

struct storage_identifier : buffer_identifier {
  using base = buffer_identifier;

  ///
  ///
  static auto create(size_type size = 0, const void* data = nullptr) noexcept
      -> storage_identifier {
    auto id = storage_identifier{base::create()};
    if (size > 0) glNamedBufferStorage(id.native_handle(), size, data, {});
    return id;
  }

  ///
  ///
  template <typename type>
  static auto create(size_type size, const type* data) noexcept
      -> storage_identifier {
    return create(size * sizeof(type), static_cast<const void*>(data));
  }

  ///
  ///
  static auto create(std::ranges::contiguous_range auto&& range) noexcept
      -> storage_identifier {
    return create(std::ranges::size(range), std::ranges::data(range));
  }
};

///
///
STRICT_FINAL_USING(storage, unique_resource<storage_identifier>);

///
///
STRICT_FINAL_USING(storage_view, resource_view<storage>);

///
///
using storage_span = basic_buffer_span<storage_identifier>;

static_assert(std::same_as<storage::identifier, storage_identifier>);

}  // namespace demo::opengl
