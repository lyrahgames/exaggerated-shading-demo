#pragma once
#include "../defaults.hpp"

namespace demo::opengl {

/// Basic Wrapper for OpenGL IDs that represent Resources
///
template <typename type>
struct basic_identifier {
  using native_handle_type = type;

  /// Default Constructor
  ///
  constexpr basic_identifier() noexcept = default;

  /// Allow only explicit construction from native handle.
  ///
  explicit constexpr basic_identifier(native_handle_type value) noexcept
      : handle{value} {}

  /// Receive native handle to interact with the OpenGL API directly.
  ///
  constexpr auto native_handle() const noexcept -> native_handle_type {
    return handle;
  }

  /// Swap for Implementation of Move Assignments
  ///
  constexpr void swap(basic_identifier& other) noexcept {
    std::swap(handle, other.handle);
  }

  /// Default Ordering Operator
  ///
  friend constexpr auto operator<=>(basic_identifier x,
                                    basic_identifier y) noexcept = default;

 protected:
  /// Handle Reset for Implementation of Move Constructions
  ///
  constexpr void reset() noexcept { handle = {}; }

 private:
  native_handle_type handle{};
};

/// All basic OpenGL objects can be identified by `GLuint`.
///
using identifier = basic_identifier<GLuint>;

/// A generic resource base/interface class makes it possible to check
/// whether the given identifier corresponds to a valid resource.
/// Also, it must provide a public static `destroy` function that
/// will be used for destruction in a RAII-wrapped resource class.
///
template <typename type>
concept resource_interface = requires(const type value, type& lvalue) {
  { type::destroy(lvalue) } -> std::same_as<void>;
  { value.valid() } -> std::convertible_to<bool>;
};

/// To implement a constructor, we require a static public factory
/// `create` function in the resource base/interface class.
/// To allow for overriding and forbid implicit constructor forwarding,
/// we require the result's type to be the same as the resource base type.
///
template <typename type, typename... types>
concept resource_construction_from = requires(types&&... args) {
  { type::create(std::forward<types>(args)...) } -> std::same_as<type>;
};

/// RAII-enabled class template to wrap any OpenGL resource base.
/// Copying is disabled - it only allows for moving operations.
///
template <resource_interface resource_base>
struct unique_resource : resource_base {
  using base = resource_base;

  /// Member type used for generating lightweight view types.
  ///
  using identifier = resource_base;

  /// The constructors of an OpenGL resource are solely determined
  /// by the factory functions of its base class.
  /// The returned identifier value is checked for validity.
  ///
  template <typename... types>
    requires resource_construction_from<base, types...>
  constexpr unique_resource(types&&... args)
      : base{base::create(std::forward<types>(args)...)} {
    if (base::valid()) return;
    throw resource_acquisition_error{"Failed to acquire OpenGL resource."};
  }

  /// Destructor is not virtual on purpose to reduce overhead.
  /// Using it in polymorphic contexts is strongly discouraged.
  ///
  constexpr ~unique_resource() noexcept {
    base::destroy(static_cast<base&>(*this));
  }

  /// Disable all copy operations.
  ///
  constexpr unique_resource(const unique_resource&) = delete;
  constexpr unique_resource& operator=(const unique_resource&) = delete;

  /// Move operations do not throw and must implemented manually
  /// to correctly invalidate `other` to let it destroy its resource.
  ///
  constexpr unique_resource(unique_resource&& other) noexcept
      : base{std::move(other)} {
    other.reset();
  }
  constexpr unique_resource& operator=(unique_resource&& other) noexcept {
    base::swap(other);
    return *this;
  }
};

/// Simple tag type whose derived types will be assumed to be view types.
///
struct resource_view_tag {};

/// Definition of generic resource view types.
///
template <typename type>
concept resource_view_like = similar_to<type, resource_view_tag>;

/// Class template to wrap resource types as a lightweight view type.
/// Copy and move operations are allowed but they are not default constructible.
/// Objects of this type template should be forwarded by value.
/// Views can only be created from lvalues of their respective resource types.
///
template <typename type>
struct resource_view : resource_view_tag, type::identifier {
  using resource_type = type;
  using identifier = typename resource_type::identifier;
  using base = identifier;

  /// Views are implicitly constructible from lvalue references.
  /// However, they explicitly forbid construction from rvalues.
  ///
  constexpr resource_view(resource_type const& x) noexcept : base{x} {}
  constexpr resource_view(resource_type const&&) = delete;
};

}  // namespace demo::opengl
