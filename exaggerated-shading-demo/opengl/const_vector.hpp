#pragma once
#include "resources/storage.hpp"

namespace demo::opengl {

template <typename type>
struct const_vector {
  using value_type = type;

  const_vector() = default;

  explicit const_vector(range_of<value_type> auto&& range)
      : _size{static_cast<size_type>(std::ranges::size(range))},
        _data{std::forward<decltype(range)>(range)} {}

  auto size() const noexcept -> size_type { return _size; }
  auto buffer() const noexcept -> storage_view { return _data; }

 private:
  size_type _size{};
  storage _data{};
};

template <typename type>
const_vector(type&&) -> const_vector<std::ranges::range_value_t<type>>;

}  // namespace demo::opengl
