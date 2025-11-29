#pragma once
#include "resources/buffer.hpp"
#include "resources/legacy_buffer.hpp"
#include "resources/storage.hpp"

namespace demo::opengl {

///
///
template <typename type, typename buffer>
concept strict_buffer_compatible_to = std::derived_from<type, buffer>;

///
///
template <typename type>
concept strict_buffer_like =
    strict_buffer_compatible_to<type, buffer_identifier>;

///
///
template <typename type>
concept strict_owning_buffer_like =
    strict_buffer_like<type> && strict_owning_resource_like<type>;

///
///
template <typename type>
concept buffer_like = strict_buffer_like<std::decay_t<type>>;

///
///
template <typename type>
concept owning_buffer_like = buffer_like<type> && owning_resource_like<type>;

// ///
// ///
// template <typename type, typename buffer>
// concept owning_buffer_compatible_to =
//     buffer_compatible_to<type, buffer> && owning_resource_like<type>;

// ///
// ///
// template <typename type>
// concept borrowed_buffer_like =
//     buffer_like<type> && (not owning_resource_like<type>);

/// Must be an unqualified type.
///
template <typename type>
concept vector_value_like =
    std::semiregular<type> /*&& std::is_trivially_copyable<type>*/;

template <typename type>
using buffer_reference_type = decltype(std::declval<type const&>().buffer());

template <typename type>
using vector_value_type = typename std::decay_t<type>::value_type;

///
///
template <typename type>
concept strict_buffer_reference_like =
    strict_buffer_like<type> && (not owning_resource_like<type>);

///
///
template <typename type>
concept strict_vector_like =
    vector_value_like<typename type::value_type> &&
    strict_buffer_reference_like<buffer_reference_type<type>> &&
    strict_buffer_compatible_to<typename type::buffer_type,
                                buffer_reference_type<type>> &&
    requires(type const& value) {
      { value.size() } -> std::convertible_to<size_type>;
    };

///
///
template <typename type>
concept vector_like = strict_vector_like<std::decay_t<type>>;

///
///
template <typename type, typename value_type>
concept vector_of =
    vector_like<type> && std::same_as<vector_value_type<type>, value_type>;

// ///
// ///
// template <typename type>
// concept borrowed_vector_like =
//     vector_like<type> && borrowed_buffer_like<typename type::buffer_type>;

// ///
// ///
// template <typename type>
// concept owning_vector_like =
//     vector_like<type> && owning_buffer_like<typename type::buffer_type>;

template <vector_value_like v, strict_buffer_like b>
struct basic_vector {
  using value_type = v;
  using buffer_type = b;
  static constexpr bool owning = strict_owning_resource_like<buffer_type>;
  static constexpr bool borrowing = not owning;

  constexpr basic_vector() noexcept = default;

  /// Construction from Contiguous Ranges
  ///
  constexpr basic_vector(contiguous_range_of<value_type> auto&& range) noexcept
    requires owning
      : _size{static_cast<size_type>(std::ranges::size(range))},
        _buffer{std::forward<decltype(range)>(range)} {}

  constexpr basic_vector(vector_of<value_type> auto&& data) noexcept
    requires borrowing && strict_buffer_compatible_to<
                              buffer_reference_type<decltype(data)>,
                              buffer_type>
      : _size{data.size()},
        _buffer{std::forward<decltype(data)>(data).buffer()} {}

  // constexpr basic_vector(buffer_compatible_to<buffer_type> auto data,
  //                        size_type bytes) noexcept
  //   requires borrowing
  //     : _size{bytes / sizeof(value_type)}, _buffer{data} {}

  // constexpr basic_vector(
  //     owning_buffer_compatible_to<buffer_type> auto const& data,
  //     size_type bytes) noexcept
  //   requires borrowing
  //     : _size{bytes / sizeof(value_type)}, _buffer{data} {}

  // constexpr basic_vector(buffer_compatible_to<buffer_type> auto data) noexcept
  //   requires borrowing
  //     : basic_vector{data, data.size()} {}

  // constexpr basic_vector(
  //     owning_buffer_compatible_to<buffer_type> auto const& data) noexcept
  //   requires borrowing
  //     : basic_vector{data, data.size()} {}

  // constexpr basic_vector(
  //     owning_buffer_compatible_to<buffer_type> auto const& data) noexcept
  //   requires borrowing
  //     : basic_vector{data, data.size()} {}

  // template <std::derived_from<buffer_type> from_buffer>
  //   requires(not owning) && (not owning_resource_like<from_buffer>)
  // constexpr basic_vector(
  //     basic_vector<value_type, from_buffer> from_vector) noexcept
  //     : _size{from_vector.size()}, _buffer{from_vector.buffer()} {}

  // template <std::derived_from<buffer_type> from_buffer>
  //   requires(not owning) && owning_resource_like<from_buffer>
  // constexpr basic_vector(
  //     basic_vector<value_type, from_buffer> const& from_vector) noexcept
  //     : _size{from_vector.size()}, _buffer{from_vector.buffer()} {}

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

  constexpr auto view() const noexcept {
    return basic_vector<value_type, decltype(buffer())>{
        buffer(), size() * sizeof(value_type)};
  }

 private:
  size_type _size;
  buffer_type _buffer;
};

/// Deduction Guide for Contiguous Ranges
///
template <std::ranges::contiguous_range range, strict_owning_buffer_like buffer>
basic_vector(range&&)
    -> basic_vector<std::ranges::range_value_t<range>, buffer>;

/// Deduction Guide for Vector Views and Conversions
///
template <vector_like vector, strict_buffer_reference_like buffer>
basic_vector(vector&&) -> basic_vector<vector_value_type<vector>, buffer>;

///
///
template <typename value_type>
using device_vector = basic_vector<value_type, storage>;

///
///
template <typename value_type>
using device_vector_view = basic_vector<value_type, storage_identifier>;

///
///
template <typename value_type>
using legacy_vector = basic_vector<value_type, legacy_buffer>;

///
///
template <typename value_type>
using legacy_vector_view = basic_vector<value_type, legacy_buffer_identifier>;

// static_assert(std::same_as<device_vector<int>::buffer_type, storage>);
// static_assert(
//     std::same_as<device_vector_view<int>::buffer_type, storage_identifier>);
// static_assert(not device_vector_view<int>::owning);
// static_assert(device_vector<int>::owning);
// static_assert(
//     std::constructible_from<device_vector<int>, std::vector<int> const&>);
// static_assert(not std::constructible_from<device_vector_view<int>,
//                                           std::vector<int> const&>);
// static_assert(std::constructible_from<device_vector_view<int>,
//                                       device_vector<int> const&>);
// static_assert(
//     std::constructible_from<device_vector_view<int>, device_vector_view<int>>);

///
///
// template <typename v, buffer_span_like s>
// struct vector_span {
//   using value_type = v;
//   using buffer_span_type = s;
//   using buffer_type = typename buffer_span_type::buffer_type;

//   template <std::derived_from<buffer_type> from_buffer>
//     requires(not owning_resource_like<from_buffer>)
//   constexpr vector_span(basic_vector<value_type, from_buffer> data,
//                         offset_type offset,
//                         size_type size) noexcept
//       : _buffer_span{data.buffer(),
//                      static_cast<offset_type>(offset * sizeof(value_type)),
//                      static_cast<size_type>(size * sizeof(value_type))} {}

//   template <std::derived_from<buffer_type> from_buffer>
//     requires owning_resource_like<from_buffer>
//   constexpr vector_span(basic_vector<value_type, from_buffer> const& data,
//                         offset_type offset,
//                         size_type size) noexcept
//       : _buffer_span{data.buffer(),
//                      static_cast<offset_type>(offset * sizeof(value_type)),
//                      static_cast<size_type>(size * sizeof(value_type))} {}

//   template <std::convertible_to<buffer_span_type> from_buffer_span>
//   constexpr vector_span(vector_span<value_type, from_buffer_span> data) noexcept
//       : _buffer_span{data.buffer_span()} {}

//   explicit constexpr vector_span(buffer_span_type span) noexcept
//       : _buffer_span{span} {}

//   constexpr auto buffer_span() const noexcept -> buffer_span_type {
//     return _buffer_span;
//   }
//   constexpr auto buffer() const noexcept -> buffer_type {
//     return _buffer_span.buffer();
//   }
//   constexpr auto offset() const noexcept -> offset_type {
//     return _buffer_span.offset() / sizeof(value_type);
//   }
//   constexpr auto size() const noexcept -> size_type {
//     return _buffer_span.size() / sizeof(value_type);
//   }

//  private:
//   buffer_span_type _buffer_span;
// };

// template <typename value_type, typename buffer_type>
//   requires(not owning_resource_like<buffer_type>)
// vector_span(basic_vector<value_type, buffer_type>, offset_type, size_type)
//     -> vector_span<value_type, basic_buffer_span<buffer_type>>;

// template <typename value_type, typename buffer_type>
//   requires owning_resource_like<buffer_type>
// vector_span(basic_vector<value_type, buffer_type> const&,
//             offset_type,
//             size_type)
//     -> vector_span<value_type,
//                    basic_buffer_span<typename buffer_type::identifier>>;

// template <vector_like v>
// struct vector_span {
//   using vector_type = v;
//   using data_type = decltype(std::declval<vector_type>().view());
//   using value_type = typename vector_type::value_type;
//   using buffer_type = typename vector_type::buffer_type;

//   using buffer_type =

//   template <std::convertible_to<data_type> from_vector>
//     requires from_vector::owning
//   constexpr vector_span(from_vector const& data,
//                          offset_type offset,
//                          size_type size) noexcept
//       : _offset{offset}, _size{size}, _data{data} {}

//   template <std::convertible_to<data_type> from_vector>
//     requires(not from_vector::owning)
//   constexpr vector_span(from_vector data,
//                          offset_type offset,
//                          size_type size) noexcept
//       : _offset{offset}, _size{size}, _data{data} {}

//   template <std::convertible_to<data_type> from_vector>
//     requires from_vector::owning
//   constexpr vector_span(from_vector const& data,
//                          offset_type offset = 0) noexcept
//       : _offset{offset}, _size{data.size() - offset}, _data{data} {}

//   template <std::convertible_to<data_type> from_vector>
//     requires(not from_vector::owning)
//   constexpr vector_span(from_vector data, offset_type offset = 0) noexcept
//       : _offset{offset}, _size{data.size() - offset}, _data{data} {}

//   template <std::convertible_to<data_type> from_vector>
//   constexpr vector_span(vector_span<from_vector> data) noexcept
//       : _offset{data.offset()}, _size{data.size()}, _data{data} {}

//   constexpr auto offset() const noexcept -> offset_type { return _offset; }
//   constexpr auto size() const noexcept -> size_type { return _size; }
//   constexpr auto buffer() const noexcept -> buffer_type {
//     return _data.buffer();
//   }

//  private:
//   offset_type _offset{};
//   size_type _size{};
//   data_type _data{};
// };

// template <typename vector_type>
// vector_span(vector_type const&) -> vector_span<vector_type>;

template <typename type>
concept strict_vector_span_like =
    strict_vector_like<type> &&
    std::same_as<typename type::buffer_type, buffer_reference_type<type>> &&
    requires(type const& value) {
      { value.offset() } -> std::convertible_to<offset_type>;
    };

template <typename type>
concept vector_span_like = strict_vector_span_like<std::decay_t<type>>;

template <typename type, typename value_type>
concept vector_span_of =
    vector_span_like<type> && std::same_as<vector_value_type<type>, value_type>;

template <vector_value_like v, strict_buffer_reference_like b>
struct vector_span {
  using value_type = v;
  using buffer_type = b;

  constexpr vector_span(buffer_like auto&& buffer,
                        offset_type offset,
                        size_type size) noexcept
      : _offset{static_cast<offset_type>(offset / sizeof(value_type))},
        _size{static_cast<size_type>(size / sizeof(value_type))},
        _buffer{std::forward<decltype(buffer)>(buffer)} {}

  explicit constexpr vector_span(buffer_like auto&& buffer,
                                 offset_type offset = 0) noexcept
      : vector_span{std::forward<decltype(buffer)>(buffer),  //
                    offset, buffer.size() - offset} {}

  constexpr vector_span(vector_of<value_type> auto&& vector,
                        offset_type offset,
                        size_type size) noexcept
      : _offset{offset}, _size{size}, _buffer{vector.buffer()} {}

  constexpr vector_span(vector_of<value_type> auto&& vector,
                        offset_type offset = 0) noexcept
      : vector_span{std::forward<decltype(vector)>(vector),  //
                    offset, vector.size() - offset} {}

  /// Construction from Compatible Vector Span Types
  ///
  template <strict_buffer_reference_like from_buffer>
  constexpr vector_span(vector_span<value_type, from_buffer> span) noexcept
      : _offset{span.offset()}, _size{span.size()}, _buffer{span.buffer()} {}

  /// Access to the Span's Offset
  ///
  constexpr auto offset() const noexcept -> offset_type { return _offset; }

  /// Access to the Span's Count of Elements
  ///
  constexpr auto size() const noexcept -> size_type { return _size; }

  /// Access to the Span's Buffer used to store its elements.
  ///
  constexpr auto buffer() const noexcept -> buffer_type { return _buffer; }

 private:
  offset_type _offset;
  size_type _size;
  buffer_type _buffer;
};

/// Deduction Guides for Vectors
///
template <vector_like vector>
vector_span(vector&&)
    -> vector_span<vector_value_type<vector>, buffer_reference_type<vector>>;
template <vector_like vector>
vector_span(vector&&, offset_type)
    -> vector_span<vector_value_type<vector>, buffer_reference_type<vector>>;
template <vector_like vector>
vector_span(vector&&, offset_type, size_type)
    -> vector_span<vector_value_type<vector>, buffer_reference_type<vector>>;

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
