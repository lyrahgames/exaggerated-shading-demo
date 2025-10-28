#pragma once
#include <array>
#include <filesystem>
#include <print>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
//
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
//
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
//
#include <glm/ext.hpp>
//
#include <glm/gtx/norm.hpp>
//
#include <cpptrace/cpptrace.hpp>

namespace demo::opengl {

using namespace std;
using namespace gl;
using namespace glm;

using czstring = const char*;

///
///
inline auto line_offsets(std::string_view str)
    -> std::vector<std::string_view::size_type> {
  using size_type = std::string_view::size_type;
  std::vector<size_type> offsets{0};
  for (size_type i = 0; i < str.size(); ++i) {
    if (str[i] == '\n') offsets.push_back(i + 1);
    if (str[i] == '\0') {
      offsets.push_back(i);
      return offsets;
    }
  }
  offsets.push_back(str.size());
  return offsets;
}

///
///
inline auto matched_line_number(const std::string& str) {
  std::regex pattern{R"##(:(\d+)\()##"};
  const auto first = std::sregex_iterator(str.begin(), str.end(), pattern);
  const auto last = std::sregex_iterator();
  if (not std::distance(first, last)) return -1;
  return std::stoi(std::smatch(*first)[1].str());
  // std::cmatch match;
  // if (not std::regex_search(str.begin(), str.end(), match, pattern)) return 0;
  // return std::stoi(match[1].str());
}

///
///
#define STRICT_USING(NEW, OLD) \
  struct NEW : OLD {           \
    using base = OLD;          \
    using base::base;          \
  }

///
///
#define STRICT_FINAL_USING(NEW, OLD) \
  struct NEW final : OLD {           \
    using base = OLD;                \
    using base::base;                \
  }

/// Basic Error
///
STRICT_USING(error, cpptrace::runtime_error);

///
///
STRICT_USING(resource_acquisition_error, error);

///
///
STRICT_USING(invalid_uniform_identifier, error);

///
///
template <typename type>
struct identifier {
  using native_handle_type = type;

 protected:
  native_handle_type handle{};

 public:
  constexpr identifier() noexcept = default;

  /// Allow only explicit construction from native handle.
  ///
  explicit constexpr identifier(native_handle_type value) noexcept
      : handle{value} {}

  /// Receive native handle to interact with the OpenGL API directly.
  ///
  constexpr auto native_handle() const noexcept -> native_handle_type {
    return handle;
  }

  friend constexpr auto operator<=>(identifier x,
                                    identifier y) noexcept = default;
};

///
///
STRICT_USING(object, identifier<GLuint>);

///
///
template <typename type>
struct unique : type {
  using identifier = type;
  using base = identifier;
  using base::handle;
  using base::valid;

  ///
  ///
  unique(auto&&... args)
    requires requires {
      {
        base::create(std::forward<decltype(args)>(args)...)
      } -> std::same_as<base>;
    }
      : base{base::create(std::forward<decltype(args)>(args)...)} {
    if (not valid())
      throw resource_acquisition_error{"Failed to acquire OpenGL resource."};
  }

  /// Destructor is not virtual on purpose to reduce overhead.
  /// Using it in polymorphic contexts is strongly discouraged.
  ///
  ~unique() noexcept { base::destroy(*this); }

  /// Make the type move-only:
  /// Copying is NOT allowed and moving does not throw.
  ///
  constexpr unique(const unique&) = delete;
  constexpr unique& operator=(const unique&) = delete;
  // Moving must implemented manually.
  // otherwise, `other` is not invalidated and the objects gets destroyed right away.
  constexpr unique(unique&& other) noexcept : base{other.native_handle()} {
    other.handle = 0;
  }
  constexpr unique& operator=(unique&& other) noexcept {
    using std::swap;
    swap(handle, other.handle);
    return *this;
  }
};

template <typename type>
struct view : type::identifier {
  using viewed_type = type;
  using identifier = typename viewed_type::identifier;
  using base = identifier;

  /// Views are implicitly constructible from lvalue references.
  /// However, they explicitly forbid construction from rvalues.
  ///
  constexpr view(const viewed_type& x) noexcept : base{x} {}
  constexpr view(const viewed_type&&) = delete;
};

///
///
template <typename type, typename base>
concept similar_to = std::derived_from<std::decay_t<type>, base>;

///
///
template <typename type>
using view_type = typename std::decay_t<type>::view_type;

///
///
template <typename type>
concept viewable =
    std::constructible_from<typename std::decay_t<type>::view_type, type>;

/// Used as syntactic sugar for providing easier restrictions for function overloads.
///
template <typename type>
concept unviewable = not viewable<type>;

/// An OpenGL object will decay to its respective view if it is possible.
///
template <typename type>
using decay = std::conditional_t<viewable<type>,
                                 view_type<type>,
                                 std::unwrap_ref_decay_t<type>>;

/// Using this forward will forward an OpenGL object
/// as its respective view if it is possible.
///
template <viewable type>
constexpr auto forward(std::remove_reference_t<type>& value) noexcept ->
    typename std::decay_t<type>::view_type {
  return value;
}
//
template <unviewable type>
constexpr auto forward(std::remove_reference_t<type>& value) noexcept
    -> type&& {
  return static_cast<type&&>(value);
}

}  // namespace demo::opengl
