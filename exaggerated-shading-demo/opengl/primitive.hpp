#pragma once
#include "draw.hpp"
#include "vector.hpp"
#include "vertex_array.hpp"

namespace demo::opengl {

template <typename type>
concept attribute_type_format_like = requires(type value) {
  { value.size() } -> std::convertible_to<GLint>;
  { value.type() } -> std::convertible_to<GLenum>;
  { value.offset() } -> std::convertible_to<GLuint>;
};

template <typename type>
concept attribute_base_format_like =
    attribute_type_format_like<type> && requires(type value) {
      { value.location() } -> std::convertible_to<GLuint>;
    };

template <typename type>
concept f32_attribute_format_like =
    attribute_base_format_like<type> && requires(type value) {
      { value.normalized() } -> std::convertible_to<GLboolean>;
    };

template <typename type>
concept f64_attribute_format_like =
    attribute_base_format_like<type> && (type::type() == GL_DOUBLE);

template <typename type>
concept int_attribute_format_like = attribute_base_format_like<type> &&
                                    std::same_as<typename type::precision, int>;

template <typename type>
concept attribute_format_like = f32_attribute_format_like<type> ||  //
                                f64_attribute_format_like<type> ||  //
                                int_attribute_format_like<type>;

template <typename type>
concept projected = std::is_lvalue_reference_v<type> &&
                    std::is_const_v<std::remove_reference_t<type>>;

template <typename type, typename value_type>
concept projection_of = requires(type projection, value_type const& value) {
  { projection(value) } -> projected;
};

template <typename value_type, projection_of<value_type> projection>
using projected_type =
    std::decay_t<std::invoke_result_t<projection, value_type const&>>;

template <typename value_type>
inline auto projection_offset(projection_of<value_type> auto projection,
                              value_type const& value = {}) noexcept
    -> std::uintptr_t {
  const auto base = reinterpret_cast<std::uintptr_t>(&value);
  const auto proj = reinterpret_cast<std::uintptr_t>(&projection(value));
  return proj - base;
}

template <GLuint l, GLint s, GLenum t, typename offset_functor>
struct basic_attribute_format : offset_functor {
  static consteval auto location() noexcept { return l; }
  static consteval auto size() noexcept { return s; }
  static consteval auto type() noexcept { return t; }
  constexpr auto offset() const noexcept {
    return offset_functor::operator()();
  }
};

template <GLuint l,
          GLint s,
          GLenum t,
          typename offset_functor,
          GLboolean n = GL_FALSE>
struct basic_f32_attribute_format
    : basic_attribute_format<l, s, t, offset_functor> {
  using base = basic_attribute_format<l, s, t, offset_functor>;
  using base::location;
  using base::offset;
  using base::size;
  using base::type;
  static consteval auto normalized() noexcept { return n; }
  void format(GLuint vaobj, GLuint binding) const noexcept {
    glEnableVertexArrayAttrib(vaobj, location());
    glVertexArrayAttribBinding(vaobj, location(), binding);
    glVertexArrayAttribFormat(vaobj, location(),  //
                              size(), type(), normalized(), offset());
  }
};

template <GLuint l, GLint s, GLenum t, typename offset_functor>
  requires(t == GL_DOUBLE)
struct basic_f64_attribute_format
    : basic_attribute_format<l, s, t, offset_functor> {
  using base = basic_attribute_format<l, s, t, offset_functor>;
  using base::location;
  using base::offset;
  using base::size;
  using base::type;
  void format(GLuint vaobj, GLuint binding) const noexcept {
    glEnableVertexArrayAttrib(vaobj, location());
    glVertexArrayAttribBinding(vaobj, location(), binding);
    glVertexArrayAttribLFormat(vaobj, location(), size(), type(), offset());
  }
};

template <GLuint l, GLint s, GLenum t, typename offset_functor>
struct basic_int_attribute_format
    : basic_attribute_format<l, s, t, offset_functor> {
  using base = basic_attribute_format<l, s, t, offset_functor>;
  using base::location;
  using base::offset;
  using base::size;
  using base::type;
  using precision = int;
  void format(GLuint vaobj, GLuint binding) const noexcept {
    glEnableVertexArrayAttrib(vaobj, location());
    glVertexArrayAttribBinding(vaobj, location(), binding);
    glVertexArrayAttribIFormat(vaobj, location(), size(), type(), offset());
  }
};

constexpr auto defaults_to_f32(GLenum type) noexcept {
  switch (type) {
    case GL_FLOAT:
    case GL_HALF_FLOAT:
    case GL_FIXED:
      return true;
    default:
      return false;
  }
}

constexpr auto defaults_to_f64(GLenum type) noexcept {
  return type == GL_DOUBLE;
}

constexpr auto defaults_to_int(GLenum type) noexcept {
  switch (type) {
    case GL_BYTE:
    case GL_SHORT:
    case GL_INT:
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_INT:
      return true;
    default:
      return false;
  }
}

template <typename value_type, GLuint location>
constexpr auto attribute_projection() noexcept {
  using traits = attribute_traits<value_type>;
  auto offset = [] { return GLuint(0); };
  auto attribute = basic_attribute_format<location, traits::size, traits::type,
                                          decltype(offset)>{offset};
  if constexpr (defaults_to_f32(traits::type))
    return basic_f32_attribute_format{attribute};
  if constexpr (defaults_to_f64(traits::type))
    return basic_f64_attribute_format{attribute};
  if constexpr (defaults_to_int(traits::type))
    return basic_int_attribute_format{attribute};
}

template <typename value_type, GLuint location>
constexpr auto attribute_projection(
    projection_of<value_type> auto projection) noexcept {
  using attribute_type = projected_type<value_type, decltype(projection)>;
  using traits = attribute_traits<attribute_type>;
  auto offset = [projection] {
    return projection_offset<value_type>(projection);
  };
  auto attribute = basic_attribute_format<location, traits::size, traits::type,
                                          decltype(offset)>{offset};
  if constexpr (defaults_to_f32(traits::type))
    return basic_f32_attribute_format{attribute};
  if constexpr (defaults_to_f64(traits::type))
    return basic_f64_attribute_format{attribute};
  if constexpr (defaults_to_int(traits::type))
    return basic_int_attribute_format{attribute};
}

template <typename type, attribute_format_like... attributes>
struct basic_vertex_buffer_format : attributes... {
  using value_type = type;
  using assign_type = vector_span<value_type>;
  static consteval auto stride() noexcept { return sizeof(value_type); }
  void format(GLuint vaobj,
              GLuint binding,
              GLuint buffer,
              GLintptr offset = 0) const noexcept {
    glVertexArrayVertexBuffer(vaobj, binding,  //
                              buffer, offset, stride());
    (attributes::format(vaobj, binding), ...);
  }
  void format(vertex_array_view va,
              GLuint binding,
              buffer_view buffer,
              GLintptr offset = 0) const noexcept {
    format(va.native_handle(), binding, buffer.native_handle(), offset);
  }
  void format(vertex_array_view va,
              GLuint binding,
              vector_span<value_type> vertices) const noexcept {
    format(va, binding, vertices.buffer(), vertices.byte_offset());
  }
};

template <GLuint l, typename projection>
struct attribute_specification : projection {
  static consteval auto location() noexcept { return l; }
};

template <GLuint location>
constexpr auto attribute(auto projection) noexcept {
  return attribute_specification<location, decltype(projection)>{projection};
}

#define MEMBER_VAR(NAME) [](auto const& x) -> auto const& { return x.NAME; }

template <typename value_type,
          GLuint location,
          projection_of<value_type> projection>
constexpr auto attribute_projection(
    attribute_specification<location, projection> spec) noexcept {
  return attribute_projection<value_type, location>(
      static_cast<projection>(spec));
}

template <GLuint location, typename value_type>
constexpr auto vertex_buffer() noexcept {
  auto attribute = attribute_projection<value_type, location>();
  return basic_vertex_buffer_format<value_type, decltype(attribute)>{attribute};
}

template <typename value_type>
constexpr auto vertex_buffer(auto... attributes) noexcept {
  return basic_vertex_buffer_format<
      value_type, decltype(attribute_projection<value_type>(attributes))...>{
      attribute_projection<value_type>(attributes)...};
}

template <typename... vertex_buffers>
struct basic_primitive_format : vertex_buffers... {
  template <std::size_t binding>
  void format(GLuint vaobj, GLuint buffer, GLintptr offset = 0) const noexcept {
    vertex_buffers...[binding] ::format(vaobj, binding, buffer, offset);
  }

  template <std::size_t binding>
  void format(vertex_array_view va,
              buffer_view buffer,
              GLintptr offset = 0) const noexcept {
    vertex_buffers...[binding] ::format(va, binding, buffer, offset);
  }

  template <std::size_t binding>
  using vertex_buffer = vertex_buffers...[binding];
  template <std::size_t binding>
  using assign_type = typename vertex_buffer<binding>::assign_type;

  template <std::size_t binding>
  void format(vertex_array_view va,
              assign_type<binding> vertices) const noexcept {
    vertex_buffers...[binding] ::format(va, binding, vertices);
  }

  void format(GLuint vaobj, GLuint element_buffer) const noexcept {
    glVertexArrayElementBuffer(vaobj, element_buffer);
  }

  void format(vertex_array_view va, buffer_view elements) const noexcept {
    format(va.native_handle(), elements.native_handle());
  }

  template <typename element_type>
  void format(vertex_array_view va,
              vector_view<element_type> elements) const noexcept {
    format(va, elements.buffer());
  }

  // template <typename element_type, GLuint... bindings>
  // void format(
  //     vertex_array_view va,
  //     vector_view<element_type> elements,
  //     vertex_buffers::assign_type... vertices,
  //     std::integer_sequence<GLuint, bindings...> _ =
  //         std::make_integer_sequence<GLuint, sizeof...(vertex_buffers)>{})
  //     const noexcept {
  //   format(va, elements);
  //   (format<bindings>(va, vertices...[bindings]), ...);
  // }
};

constexpr auto primitive(auto... vertex_buffers) noexcept {
  return basic_primitive_format<decltype(vertex_buffers)...>{vertex_buffers...};
}

}  // namespace demo::opengl
