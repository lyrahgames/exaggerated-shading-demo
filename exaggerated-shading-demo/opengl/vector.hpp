#pragma once
#include "buffer.hpp"

namespace demo::opengl {

///
///
template <typename type, typename value_type>
concept range_of = std::ranges::contiguous_range<type> &&
                   std::same_as<std::ranges::range_value_t<type>, value_type>;

///
///
template <typename type>
struct vector_base {
  using value_type = type;
  // using size_type = size_type;

  auto size(this auto const& self) noexcept -> size_type {
    return self.buffer().size() / sizeof(value_type);
  }

  void assign(this auto& self,
              range_of<value_type> auto const& values) noexcept {
    self.buffer().assign(values);
  }
};

template <typename type>
struct vector : vector_base<type> {
  using base = vector_base<type>;

  explicit operator buffer_view() const noexcept { return data; }

  auto buffer() const noexcept -> buffer_view { return data; }

 protected:
  struct buffer data;
};

template <typename type>
struct vector_view : vector_base<type> {
  using base = vector_base<type>;

  constexpr vector_view(const vector<type>& x) noexcept : data{x.buffer()} {}
  constexpr vector_view(const vector<type>&&) = delete;

  explicit operator buffer_view() const noexcept { return data; }

  auto buffer() const noexcept -> buffer_view { return data; }

 protected:
  buffer_view data;
};

}  // namespace demo::opengl
