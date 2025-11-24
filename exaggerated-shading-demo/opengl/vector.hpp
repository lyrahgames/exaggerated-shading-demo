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
  using size_type = GLuint;
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

template <typename type>
vector_view(type&&) -> vector_view<typename std::decay_t<type>::value_type>;

template <typename type>
struct vector_span {
  using value_type = type;
  using size_type = GLuint;

  constexpr vector_span(vector_view<type> v,
                        size_type _first,
                        size_type _last) noexcept
      : data{v}, first{_first}, last{_last} {
    // assert(first <= last);
    assert(last <= data.size());
  }

  constexpr vector_span(vector_view<type> v, size_type offset = 0) noexcept
      : vector_span{v, offset, v.size()} {}

  constexpr auto size() const noexcept { return last - first; }
  constexpr auto offset() const noexcept { return first; }
  constexpr auto byte_offset() const noexcept {
    return offset() * sizeof(value_type);
  }
  constexpr auto buffer() const noexcept { return data.buffer(); }

 protected:
  vector_view<type> data;
  size_type first;
  size_type last;
};

template <typename type>
vector_span(type&&, GLuint, GLuint)
    -> vector_span<typename std::decay_t<type>::value_type>;
template <typename type>
vector_span(type&&, GLuint)
    -> vector_span<typename std::decay_t<type>::value_type>;
template <typename type>
vector_span(type&&) -> vector_span<typename std::decay_t<type>::value_type>;

}  // namespace demo::opengl
