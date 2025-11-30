#pragma once
#include "frame.hpp"
#include "viewport.hpp"

namespace demo::opengl {

struct camera_base : frame {
  vec3 focus{};  // must store focus as point for numerical stability
  real fov{1.0};

  constexpr void move(vec3 delta) noexcept {
    translate(delta.x * right() + delta.y * up() + delta.z * out());
  }

  constexpr void move(vec2 delta) noexcept { move(vec3(delta, 0.0)); }

  constexpr void move(real delta) noexcept { move(vec3(0.0, 0.0, delta)); }

  constexpr void trackball(frame const& init, vec2 delta) noexcept {
    const auto n = vec2(delta.y, -delta.x);
    const auto angle = length(n);
    if (angle < 1e-3f) return;
    const auto axis = init.global_direction(vec2(delta.y, -delta.x) / angle);
    rotate(init, focus, axis, angle);
  }
  constexpr void trackball(vec2 delta) noexcept { trackball(*this, delta); }

  constexpr void bell_trackball(frame const& init, vec2 from, vec2 to) {
    constexpr auto depth = [](vec2 x) {
      const auto d = length(x);
      constexpr auto r = 0.8f;
      constexpr auto b = r / std::sqrt(2.0f);
      return (d < b) ? std::sqrt(r * r - d * d) : (r * r / 2.0f / d);
    };
    if (distance(from, to) < 1e-3f) return;
    const auto p = normalize(vec3(from, depth(from)));
    const auto q = normalize(vec3(to, depth(to)));
    const auto axis = normalize(init.global_direction(cross(p, q)));
    const auto angle = -std::acos(dot(p, q));
    rotate(init, focus, axis, angle);
  }
  constexpr void bell_trackball(vec2 from, vec2 to) {
    bell_trackball(*this, from, to);
  }

  constexpr auto view() const noexcept { return local(); }
};

constexpr auto view_matrix(camera_base const& cam) noexcept {
  return cam.view();
}

struct perspective_camera : camera_base {
  using base = camera_base;

  constexpr auto projection(viewport const& screen,
                            real near = 1e-3,
                            real far = 1000.0) noexcept {
    return glm::perspective(fov, aspect_ratio(screen), near, far);
  }

  constexpr void fit(frame const& reference, real radius) noexcept {
    static_cast<frame&>(*this) = reference;
    focus = reference.origin();
    move(radius / std::sin(fov / 2));
  }

  constexpr void zoom(real scale) noexcept {
    translation = (origin() - focus) * exp(-scale) + focus;
  }
};

struct orthographic_camera : camera_base {
  using base = camera_base;

  constexpr auto projection(viewport const& screen,
                            real near = 1e-3,
                            real far = 1000.0) noexcept {
    const auto y = fov / 2;
    const auto x = aspect_ratio(screen) * y;
    return glm::ortho(-x, x, -y, y, near, far);
  }

  constexpr void fit(frame const& reference, real radius) noexcept {
    static_cast<frame&>(*this) = reference;
    focus = reference.origin();
    move(radius);
    fov = 2 * radius;
  }

  constexpr void zoom(real scale) noexcept { fov *= exp(-scale); }
};

struct camera : std::variant<perspective_camera, orthographic_camera> {};

}  // namespace demo::opengl
