#pragma once
#include "resources/buffer.hpp"
#include "resources/legacy_buffer.hpp"
#include "resources/storage.hpp"

namespace demo::opengl {

template <typename v, std::derived_from<buffer_identifier> b>
struct basic_vector {
  using value_type = v;
  using buffer_type = b;
  static constexpr bool owning = owning_resource_like<buffer_type>;
  // using identifier =
  //     std::conditional_t<owning, typename buffer_type::identifier, buffer_type>;

  constexpr basic_vector() noexcept = default;

  constexpr basic_vector(contiguous_range_of<value_type> auto&& range) noexcept
    requires owning
      : _size{static_cast<size_type>(std::ranges::size(range))},
        _buffer{std::forward<decltype(range)>(range)} {}

  template <std::derived_from<buffer_type> from_buffer>
    requires(not owning) && (not owning_resource_like<from_buffer>)
  constexpr basic_vector(
      basic_vector<value_type, from_buffer> from_vector) noexcept
      : _size{from_vector.size()}, _buffer{from_vector.buffer()} {}

  template <std::derived_from<buffer_type> from_buffer>
    requires(not owning) && owning_resource_like<from_buffer>
  constexpr basic_vector(
      basic_vector<value_type, from_buffer> const& from_vector) noexcept
      : _size{from_vector.size()}, _buffer{from_vector.buffer()} {}

  constexpr auto size() const noexcept -> size_type { return _size; }
  constexpr auto buffer() const noexcept -> buffer_type
    requires(not owning)
  {
    return _buffer;
  }

  constexpr auto buffer() const noexcept
    requires owning
  {
    return typename buffer_type::identifier{_buffer};
  }

 private:
  size_type _size;
  buffer_type _buffer;
};

template <std::ranges::contiguous_range range, typename buffer_type>
  requires owning_resource_like<buffer_type>
basic_vector(range&&)
    -> basic_vector<std::ranges::range_value_t<range>, buffer_type>;

template <typename value_type,
          typename buffer_type,
          std::derived_from<buffer_type> from_buffer>
  requires(not owning_resource_like<buffer_type>)
basic_vector(basic_vector<value_type, from_buffer> const&)
    -> basic_vector<value_type, buffer_type>;

template <typename value_type>
using device_vector = basic_vector<value_type, storage>;
template <typename value_type>
using device_vector_view = basic_vector<value_type, storage_identifier>;

template <typename value_type>
using legacy_vector = basic_vector<value_type, legacy_buffer>;
template <typename value_type>
using legacy_vector_view = basic_vector<value_type, legacy_buffer_identifier>;

static_assert(std::same_as<device_vector<int>::buffer_type, storage>);
static_assert(
    std::same_as<device_vector_view<int>::buffer_type, storage_identifier>);
static_assert(not device_vector_view<int>::owning);
static_assert(device_vector<int>::owning);
static_assert(
    std::constructible_from<device_vector<int>, std::vector<int> const&>);
static_assert(not std::constructible_from<device_vector_view<int>,
                                          std::vector<int> const&>);
static_assert(std::constructible_from<device_vector_view<int>,
                                      device_vector<int> const&>);
static_assert(
    std::constructible_from<device_vector_view<int>, device_vector_view<int>>);

///
///
template <typename v, buffer_span_like s>
struct vector_span {
  using value_type = v;
  using buffer_span_type = s;
  using buffer_type = typename buffer_span_type::buffer_type;

  template <std::derived_from<buffer_type> from_buffer>
    requires(not owning_resource_like<from_buffer>)
  constexpr vector_span(basic_vector<value_type, from_buffer> data,
                        offset_type offset,
                        size_type size) noexcept
      : _buffer_span{data.buffer(),
                     static_cast<offset_type>(offset * sizeof(value_type)),
                     static_cast<size_type>(size * sizeof(value_type))} {}

  template <std::derived_from<buffer_type> from_buffer>
    requires owning_resource_like<from_buffer>
  constexpr vector_span(basic_vector<value_type, from_buffer> const& data,
                        offset_type offset,
                        size_type size) noexcept
      : _buffer_span{data.buffer(),
                     static_cast<offset_type>(offset * sizeof(value_type)),
                     static_cast<size_type>(size * sizeof(value_type))} {}

  template <std::convertible_to<buffer_span_type> from_buffer_span>
  constexpr vector_span(vector_span<value_type, from_buffer_span> data) noexcept
      : _buffer_span{data.buffer_span()} {}

  explicit constexpr vector_span(buffer_span_type span) noexcept
      : _buffer_span{span} {}

  constexpr auto buffer_span() const noexcept -> buffer_span_type {
    return _buffer_span;
  }
  constexpr auto buffer() const noexcept -> buffer_type {
    return _buffer_span.buffer();
  }
  constexpr auto offset() const noexcept -> offset_type {
    return _buffer_span.offset() / sizeof(value_type);
  }
  constexpr auto size() const noexcept -> size_type {
    return _buffer_span.size() / sizeof(value_type);
  }

 private:
  buffer_span_type _buffer_span;
};

template <typename value_type, typename buffer_type>
  requires(not owning_resource_like<buffer_type>)
vector_span(basic_vector<value_type, buffer_type>, offset_type, size_type)
    -> vector_span<value_type, basic_buffer_span<buffer_type>>;

template <typename value_type, typename buffer_type>
  requires owning_resource_like<buffer_type>
vector_span(basic_vector<value_type, buffer_type> const&,
            offset_type,
            size_type)
    -> vector_span<value_type,
                   basic_buffer_span<typename buffer_type::identifier>>;

// template <typename value_type>
// using vector_span = basic_vector_span<value_type, buffer_span>;

///
///
// template <typename type>
// struct vector_base {
//   using value_type = type;
//   using size_type = GLuint;
//   // using size_type = size_type;

//   auto size(this auto const& self) noexcept -> size_type {
//     return self.buffer().size() / sizeof(value_type);
//   }

//   void assign(this auto& self,
//               range_of<value_type> auto const& values) noexcept {
//     self.buffer().assign(values);
//   }
// };

// template <typename type>
// struct vector : vector_base<type> {
//   using base = vector_base<type>;

//   explicit operator buffer_view() const noexcept { return data; }

//   auto buffer() const noexcept -> buffer_view { return data; }

//  protected:
//   struct buffer data;
// };

// template <typename type>
// struct vector_view : vector_base<type> {
//   using base = vector_base<type>;

//   constexpr vector_view(const vector<type>& x) noexcept : data{x.buffer()} {}
//   constexpr vector_view(const vector<type>&&) = delete;

//   explicit operator buffer_view() const noexcept { return data; }

//   auto buffer() const noexcept -> buffer_view { return data; }

//  protected:
//   buffer_view data;
// };

// template <typename type>
// vector_view(type&&) -> vector_view<typename std::decay_t<type>::value_type>;

// template <typename type>
// struct vector_span {
//   using value_type = type;
//   using size_type = GLuint;

//   constexpr vector_span(vector_view<type> v,
//                         size_type _first,
//                         size_type _last) noexcept
//       : data{v}, first{_first}, last{_last} {
//     // assert(first <= last);
//     assert(last <= data.size());
//   }

//   constexpr vector_span(vector_view<type> v, size_type offset = 0) noexcept
//       : vector_span{v, offset, v.size()} {}

//   constexpr auto size() const noexcept { return last - first; }
//   constexpr auto offset() const noexcept { return first; }
//   constexpr auto byte_offset() const noexcept {
//     return offset() * sizeof(value_type);
//   }
//   constexpr auto buffer() const noexcept { return data.buffer(); }

//  protected:
//   vector_view<type> data;
//   size_type first;
//   size_type last;
// };

// template <typename type>
// vector_span(type&&, GLuint, GLuint)
//     -> vector_span<typename std::decay_t<type>::value_type>;
// template <typename type>
// vector_span(type&&, GLuint)
//     -> vector_span<typename std::decay_t<type>::value_type>;
// template <typename type>
// vector_span(type&&) -> vector_span<typename std::decay_t<type>::value_type>;

}  // namespace demo::opengl
