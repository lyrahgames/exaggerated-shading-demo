#pragma once
#include "buffer.hpp"

namespace demo::opengl {

template <typename type>
class immutable_vector {
  struct buffer _data{};
  size_type _size{};

 public:
  using value_type = type;

  immutable_vector() = default;

  explicit immutable_vector(range_of<value_type> auto&& range)
      : _size{static_cast<size_type>(std::ranges::size(range))} {
    _data.immutable_assign(std::forward<decltype(range)>(range));
  }

  auto size() const noexcept -> size_type { return _size; }
  auto buffer() const noexcept -> buffer_view { return _data; }
};

template <typename type>
immutable_vector(type&&) -> immutable_vector<std::ranges::range_value_t<type>>;

}  // namespace demo::opengl
