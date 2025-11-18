#pragma once
#include "defaults.hpp"

namespace demo::opengl {

struct frame {
  quat orientation{1.0, 0.0, 0.0, 0.0};
  vec3 translation{};

  constexpr auto right() const noexcept { return mat3_cast(orientation)[0]; }
  constexpr auto up() const noexcept { return mat3_cast(orientation)[1]; }
  constexpr auto out() const noexcept { return mat3_cast(orientation)[2]; }
  constexpr auto origin() const noexcept { return translation; }

  constexpr void translate(vec3 delta) noexcept { translation += delta; }
  constexpr void move_to(vec3 position) noexcept { translation = position; }

  constexpr void rotate(frame const& init,
                        vec3 center,
                        vec3 axis,
                        real angle) noexcept {
    const auto m = glm::angleAxis(angle, axis);
    orientation = m * init.orientation;
    translation = m * (init.translation - center) + center;
    repair();
  }

  constexpr void rotate(vec3 center, vec3 axis, real angle) noexcept {
    const auto m = glm::angleAxis(angle, axis);
    orientation = m * orientation;
    translation = m * (translation - center) + center;
    repair();
  }

  constexpr void repair() noexcept { orientation = normalize(orientation); }

  constexpr auto global() const noexcept -> mat4 {
    return glm::translate(mat4_cast(orientation), translation);
  }

  constexpr auto global_direction(vec3 local) const noexcept {
    return mat3_cast(orientation) * local;
  }
  constexpr auto global_direction(vec2 local) const noexcept {
    return global_direction(vec3(local, 0.0));
  }
  constexpr auto global(vec3 local) const noexcept {
    return global_direction(local) + translation;
  }
  constexpr auto global(vec2 local) const noexcept {
    return global(vec3(local, 0.0));
  }

  constexpr auto local() const noexcept -> mat4 {
    auto m = mat4_cast(conjugate(orientation));
    m[3] = m * vec4(-translation, 1.0);
    return m;
  }
};

constexpr auto right(frame const& basis) noexcept {
  return basis.right();
}
constexpr auto up(frame const& basis) noexcept {
  return basis.up();
}
constexpr auto out(frame const& basis) noexcept {
  return basis.out();
}
constexpr auto origin(frame const& basis) noexcept {
  return basis.origin();
}

constexpr auto rotate(frame const& basis, vec3 center, quat rotation) noexcept {
  return frame{normalize(rotation * basis.orientation),
               rotation * (basis.translation - center) + center};
}

}  // namespace demo::opengl
