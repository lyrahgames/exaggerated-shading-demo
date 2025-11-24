#pragma once
#include "vector.hpp"
#include "vertex_array.hpp"

namespace demo::opengl {

template <typename T>
struct attribute_traits {};

template <>
struct attribute_traits<float> {
  static constexpr GLint size = 1;
  static constexpr GLenum type = GL_FLOAT;
};

template <>
struct attribute_traits<vec2> {
  static constexpr GLint size = 2;
  static constexpr GLenum type = GL_FLOAT;
};

template <>
struct attribute_traits<vec3> {
  static constexpr GLint size = 3;
  static constexpr GLenum type = GL_FLOAT;
};

template <>
struct attribute_traits<vec4> {
  static constexpr GLint size = 4;
  static constexpr GLenum type = GL_FLOAT;
};

template <>
struct attribute_traits<double> {
  static constexpr GLint size = 1;
  static constexpr GLenum type = GL_DOUBLE;
};

template <>
struct attribute_traits<dvec2> {
  static constexpr GLint size = 2;
  static constexpr GLenum type = GL_DOUBLE;
};

template <>
struct attribute_traits<dvec4> {
  static constexpr GLint size = 4;
  static constexpr GLenum type = GL_DOUBLE;
};

template <>
struct attribute_traits<ivec2> {
  static constexpr GLint size = 2;
  static constexpr GLenum type = GL_INT;
};

template <>
struct attribute_traits<ivec4> {
  static constexpr GLint size = 4;
  static constexpr GLenum type = GL_INT;
};

struct f32_attribute_format {
  GLuint location;
  GLsizei size;
  GLenum type;
  GLboolean normalized = GL_FALSE;
  GLintptr offset;

  void operator()(vertex_array_view va, GLuint binding) const noexcept {
    assert(location < GL_MAX_VERTEX_ATTRIBS);
    glEnableVertexArrayAttrib(va.native_handle(), location);
    glVertexArrayAttribBinding(va.native_handle(), location, binding);
    glVertexArrayAttribFormat(va.native_handle(), location, size, type,
                              normalized, offset);
  }
};

template <typename type>
constexpr auto f32_attribute(GLuint location,
                             GLintptr offset = 0,
                             GLboolean normalized = GL_FALSE) noexcept
    -> f32_attribute_format {
  return f32_attribute_format{
      .location = location,
      .size = attribute_traits<type>::size,
      .type = attribute_traits<type>::type,
      .normalized = normalized,
      .offset = offset,
  };
}

struct f64_attribute_format {
  GLuint location;
  GLsizei size;
  GLintptr offset;

  void operator()(vertex_array_view va, GLuint binding) const noexcept {
    assert(location < GL_MAX_VERTEX_ATTRIBS);
    glEnableVertexArrayAttrib(va.native_handle(), location);
    glVertexArrayAttribBinding(va.native_handle(), location, binding);
    glVertexArrayAttribLFormat(va.native_handle(), location,  //
                               size, GL_DOUBLE, offset);
  }
};

template <typename type>
  requires(attribute_traits<type>::type == GL_DOUBLE)
constexpr auto f64_attribute(GLuint location, GLintptr offset = 0) noexcept
    -> f64_attribute_format {
  return f64_attribute_format{
      .location = location,
      .size = attribute_traits<type>::size,
      .offset = offset,
  };
}

struct int_attribute_format {
  GLuint location;
  GLsizei size;
  GLenum type;
  GLintptr offset;

  void operator()(vertex_array_view va, GLuint binding) const noexcept {
    assert(location < GL_MAX_VERTEX_ATTRIBS);
    glEnableVertexArrayAttrib(va.native_handle(), location);
    glVertexArrayAttribBinding(va.native_handle(), location, binding);
    glVertexArrayAttribIFormat(va.native_handle(), location, size, type,
                               offset);
  }
};

template <typename type>
constexpr auto int_attribute(GLuint location, GLintptr offset = 0) noexcept
    -> int_attribute_format {
  return int_attribute_format{
      .location = location,
      .size = attribute_traits<type>::size,
      .type = attribute_traits<type>::type,
      .offset = offset,
  };
}

constexpr auto is_f32(GLenum type) noexcept {
  switch (type) {
    case GL_FLOAT:
    case GL_HALF_FLOAT:
    case GL_FIXED:
      return true;
    default:
      return false;
  }
}

constexpr auto is_f64(GLenum type) noexcept {
  return type == GL_DOUBLE;
}

constexpr auto is_int(GLenum type) noexcept {
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

template <typename type>
  requires(is_f64(attribute_traits<type>::type))
constexpr auto attribute(GLuint location, GLintptr offset = 0) noexcept {
  return f64_attribute<type>(location, offset);
}

template <typename type>
  requires(is_f32(attribute_traits<type>::type))
constexpr auto attribute(GLuint location, GLintptr offset = 0) noexcept {
  return f32_attribute<type>(location, offset);
}

template <typename type>
  requires(is_int(attribute_traits<type>::type))
constexpr auto attribute(GLuint location, GLintptr offset = 0) noexcept {
  return int_attribute<type>(location, offset);
}

template <typename type>
concept accessible = std::is_lvalue_reference_v<type> &&
                     std::is_const_v<std::remove_reference_t<type>>;

template <typename type, typename value_type>
concept accessor_of = requires(type access, value_type const& value) {
  { access(value) } -> accessible;
};

template <typename value_type>
inline auto f32_attribute_projection(
    GLuint location,
    accessor_of<value_type> auto access) noexcept {
  using attribute_type =
      std::decay_t<decltype(access(std::declval<value_type const&>()))>;
  return f32_attribute<attribute_type>(location,
                                       access_offset<value_type>(access));
}

template <typename value_type>
inline auto f64_attribute_projection(
    GLuint location,
    accessor_of<value_type> auto access) noexcept {
  using attribute_type =
      std::decay_t<decltype(access(std::declval<value_type const&>()))>;
  return f64_attribute<attribute_type>(location,
                                       access_offset<value_type>(access));
}

template <typename value_type>
inline auto int_attribute_projection(
    GLuint location,
    accessor_of<value_type> auto access) noexcept {
  using attribute_type =
      std::decay_t<decltype(access(std::declval<value_type const&>()))>;
  return int_attribute<attribute_type>(location,
                                       access_offset<value_type>(access));
}

template <typename value_type>
inline auto attribute_projection(f32,
                                 GLuint location,
                                 accessor_of<value_type> auto access) noexcept {
  return f32_attribute_projection<value_type>(location, access);
}

template <typename value_type>
inline auto attribute_projection(f64,
                                 GLuint location,
                                 accessor_of<value_type> auto access) noexcept {
  return f64_attribute_projection<value_type>(location, access);
}

template <typename value_type>
inline auto attribute_projection(int,
                                 GLuint location,
                                 accessor_of<value_type> auto access) noexcept {
  return int_attribute_projection<value_type>(location, access);
}

template <typename value_type>
inline auto attribute_projection(GLuint location,
                                 accessor_of<value_type> auto access) noexcept {
  using attribute_type =
      std::decay_t<decltype(access(std::declval<value_type const&>()))>;
  return attribute<attribute_type>(location, access_offset<value_type>(access));
}

template <typename type>
concept attribute_like = one_of<type,
                                f32_attribute_format,
                                f64_attribute_format,
                                int_attribute_format> ||
                         (std::tuple_size<type>::value > 1);

template <typename value_type>
constexpr auto attribute_projection(f32_attribute_format x) noexcept {
  return x;
}
template <typename value_type>
constexpr auto attribute_projection(f64_attribute_format x) noexcept {
  return x;
}
template <typename value_type>
constexpr auto attribute_projection(int_attribute_format x) noexcept {
  return x;
}
template <typename value_type>
inline auto attribute_projection(auto arg) noexcept
  requires(std::tuple_size<decltype(arg)>::value == 2)
{
  const auto [location, access] = arg;
  return attribute_projection<value_type>(location, access);
}
template <typename value_type>
inline auto attribute_projection(auto arg) noexcept
  requires(std::tuple_size<decltype(arg)>::value == 3)
{
  const auto [format, location, access] = arg;
  return attribute_projection<value_type>(format, location, access);
}

#define ATTRIBUTE_MEMBER(LOCATION, NAME)                          \
  std::tuple {                                                    \
    LOCATION, [](auto const& x) -> auto const& { return x.NAME; } \
  }

#define ATTRIBUTE_ACCESS(LOCATION, VAR, EXPR)                     \
  std::tuple {                                                    \
    LOCATION, [](auto const& VAR) -> auto const& { return EXPR; } \
  }

#define FORMAT_ATTRIBUTE_MEMBER(FORMAT, LOCATION, NAME)                     \
  std::tuple {                                                              \
    FORMAT{}, LOCATION, [](auto const& x) -> auto const& { return x.NAME; } \
  }

#define FORMAT_ATTRIBUTE_ACCESS(FORMAT, LOCATION, VAR, EXPR)                \
  std::tuple {                                                              \
    FORMAT{}, LOCATION, [](auto const& VAR) -> auto const& { return EXPR; } \
  }

template <typename... types>
struct static_vertex_buffer_format {
  // GLuint binding;
  GLuint buffer = 0;
  GLintptr offset = 0;
  GLsizei stride = 0;
  // size_type size;
  std::tuple<types...> attributes{};

  constexpr static_vertex_buffer_format(GLsizei stride, types... attr) noexcept
      : stride{stride}, attributes{attr...} {}

  template <std::size_t... indices>
  void operator()(
      vertex_array_view va,
      GLuint binding,
      std::index_sequence<indices...> _ =
          std::make_index_sequence<sizeof...(types)>{}) const noexcept {
    assert(binding < GL_MAX_VERTEX_ATTRIB_BINDINGS);
    glVertexArrayVertexBuffer(va.native_handle(), binding,  //
                              buffer, offset, stride);
    (std::invoke(std::get<indices>(attributes), va, binding), ...);
  }
};

template <typename type, typename... types>
struct static_vertex_vector_format : static_vertex_buffer_format<types...> {
  using base = static_vertex_buffer_format<types...>;
  using value_type = type;

  constexpr static_vertex_vector_format(types... attr) noexcept
      : base{sizeof(value_type), attr...} {}

  constexpr void assign(vector_view<value_type> v, size_t first = 0) noexcept {
    base::buffer = v.buffer().native_handle();
    base::offset = first * sizeof(value_type);
  }
};

template <typename value_type>
constexpr auto vertex_vector_format(GLuint location) noexcept {
  auto attr = attribute<value_type>(location);
  return static_vertex_vector_format<value_type, decltype(attr)>{attr};
}

template <typename value_type>
constexpr auto vertex_vector_format(
    attribute_like auto... attributes) noexcept {
  return static_vertex_vector_format<
      value_type, decltype(attribute_projection<value_type>(attributes))...>{
      attribute_projection<value_type>(attributes)...};
}

template <typename... types>
struct static_primitive_format {
  std::tuple<types...> vertex_buffers;
  GLenum topology;
  GLint first;
  GLsizei count;

  template <GLuint... indices>
  void operator()(vertex_array_view va,
                  std::integer_sequence<GLuint, indices...> _ =
                      std::make_integer_sequence<GLuint, sizeof...(types)>{})
      const noexcept {
    (std::invoke(std::get<indices>(vertex_buffers), va, indices), ...);
  }
};

template <typename... types>
struct static_primitive_elements_format : static_primitive_format<types...> {
  using base = static_primitive_format<types...>;

  GLuint element_buffer;
  GLenum element_index_type;
  GLsizei element_count;
  GLintptr element_offset;

  void operator()(vertex_array_view va) const noexcept {
    base::operator()(va);
    glVertexArrayElementBuffer(va.native_handle(), element_buffer);
  }
};

template <typename value_type, accessor_of<value_type> auto access>
using access_type =
    std::decay_t<decltype(access(std::declval<value_type const&>()))>;

namespace meta {

template <GLuint loc,
          typename projection,
          one_of<void, int, float, double> precision = void>
struct static_attribute_format : projection {
  using base = projection;

  static constexpr auto location() noexcept { return loc; }

  // constexpr auto project(auto const& x) const noexcept -> decltype(auto) {
  //   return base::operator()(x);
  // }

  template <typename value_type>
  using attribute_type = std::decay_t<decltype(base::operator()(
      std::declval<value_type const&>()))>;

  template <typename value_type>
  static constexpr auto size(auto attribute) noexcept {
    return attribute_traits<attribute_type<value_type>>::size;
  }

  template <typename value_type>
  static constexpr auto type() noexcept {
    return attribute_traits<attribute_type<value_type>>::type;
  }

  template <typename value_type>
  auto offset(value_type const& value = {}) noexcept {
    const auto base = reinterpret_cast<std::uintptr_t>(&value);
    const auto proj =
        reinterpret_cast<std::uintptr_t>(&base::operator()(value));
    return proj - base;
  }

  template <typename value_type>
  void format(vertex_array_view va, GLuint binding) const noexcept {
    glEnableVertexArrayAttrib(va.native_handle(), location());
    glVertexArrayAttribBinding(va.native_handle(), location(), binding);
    if constexpr (std::same_as<precision, int>)
      glVertexArrayAttribIFormat(va.native_handle(),  //
                                 location(), size<value_type>(),
                                 type<value_type>(), offset<value_type>());
    else if constexpr (std::same_as<precision, double>)
      glVertexArrayAttribLFormat(va.native_handle(),  //
                                 location(), size<value_type>(),
                                 type<value_type>(), offset<value_type>());
    else
      glVertexArrayAttribFormat(va.native_handle(),  //
                                location(), size<value_type>(),
                                type<value_type>(), GL_FALSE,
                                offset<value_type>());
  }
};

template <GLuint location, one_of<void, int, float, double> precision = void>
constexpr auto attribute(auto access) noexcept {
  return static_attribute_format<location, decltype(access), precision>{access};
}

template <typename value_type, typename... attributes>
struct static_vertex_buffer_format : attributes... {
  static constexpr auto stride() noexcept -> GLsizei {
    return sizeof(value_type);
  }

  void format(vertex_array_view va, GLuint binding) const noexcept {
    glVertexArrayVertexBuffer(va.native_handle(), binding,  //
                              buffer, offset, stride());
    (attributes::template format<value_type>(va, binding), ...);
  }

  GLuint buffer = 0;
  GLintptr offset = 0;
};

template <typename value_type>
constexpr auto vertex_buffer(auto... attributes) noexcept {
  return static_vertex_buffer_format<value_type, decltype(attributes)...>{
      attributes...};
}

template <GLuint location, typename value_type>
constexpr auto vertex_buffer() noexcept {
  auto identity =
      attribute<location>([](auto const& x) -> auto const& { return x; });
  return static_vertex_buffer_format<value_type, decltype(identity)>{identity};
}

template <typename... vertex_buffers>
struct static_array_primitive_format : vertex_buffers... {
  template <GLuint... locations>
  void format(
      vertex_array_view va,
      std::integer_sequence<GLuint, locations...> _ =
          std::make_integer_sequence<GLuint, sizeof...(vertex_buffers)>{})
      const noexcept {
    (vertex_buffers...[locations] ::format(va, locations), ...);
  }
};

template <typename... vertex_buffers>
struct static_element_primitive_format
    : static_array_primitive_format<vertex_buffers...> {
  using base = static_array_primitive_format<vertex_buffers...>;

  constexpr static_element_primitive_format(buffer_view elements,
                                            vertex_buffers... args) noexcept
      : base{args...}, element_buffer{elements.native_handle()} {}

  void format(vertex_array_view va) const noexcept {
    base::format(va);
    glVertexArrayElementBuffer(va.native_handle(), element_buffer);
  }

  GLuint element_buffer;
};

constexpr auto primitve(buffer_view element_buffer,
                        auto... vertex_buffers) noexcept {
  return static_element_primitive_format<decltype(vertex_buffers)...>{
      element_buffer, vertex_buffers...};
}

}  // namespace meta

// rendering boils mostly down to:
// glDrawArrays
// glDrawElements
// *Instanced
// maybe: *Indirect
// glMultiDrawArraysIndirect -> GPU-driven list of draw commands

// so far I do not see any benefits using *Range or *BaseVertex variants

// typedef struct {
//   uint count;
//   uint instanceCount;
//   uint first;
//   uint baseInstance;
// } DrawArraysIndirectCommand;

// typedef struct {
//   uint count;
//   uint instanceCount;
//   uint firstIndex;
//   int baseVertex;
//   uint baseInstance;
// } DrawElementsIndirectCommand;

// glMultiDrawArraysIndirect(GLenum mode,
//                           const void* indirect,
//                           GLsizei drawcount,
//                           GLsizei stride);

struct array_render_format {};

struct element_render_format {};

inline void draw(GLenum mode, GLint first, GLsizei count) noexcept {
  glDrawArrays(mode, first, count);
}

inline void draw(GLenum mode,
                 GLsizei count,
                 GLenum type,
                 const GLvoid* indices) noexcept {
  glDrawElements(mode, count, type, indices);
}

}  // namespace demo::opengl
