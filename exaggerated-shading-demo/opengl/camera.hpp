#pragma once
#include "defaults.hpp"

namespace demo::opengl {

struct viewport {
  ivec2 offset{};
  ivec2 size{};

  constexpr operator ivec4() const noexcept { return {offset, size}; }

  constexpr auto width() const noexcept { return size.x; }
  constexpr auto height() const noexcept { return size.y; }

  constexpr auto left() const noexcept { return offset.x; }
  constexpr auto right() const noexcept { return offset.x + size.x; }
  constexpr auto top() const noexcept { return offset.y + size.y; }
  constexpr auto bottom() const noexcept { return offset.y; }

  constexpr auto aspect_ratio() const noexcept {
    return float(size.x) / size.y;
  }
};

constexpr auto aspect_ratio(viewport const& screen) noexcept {
  return screen.aspect_ratio();
}

// `frame` of reference
// `basis`
// `space`
// variables: `world`, `local`, `model`
struct coordinate_system {
  mat4 basis{1.0};

  constexpr operator mat4() const noexcept { return basis; }

  constexpr auto operator()(vec4 local) const noexcept { return basis * local; }
  constexpr auto operator()(vec3 local) const noexcept {
    return vec3(basis * vec4(local, 1.0));
  }

  constexpr auto right() const noexcept -> vec3 { return basis[0]; }
  constexpr auto up() const noexcept -> vec3 { return basis[1]; }
  constexpr auto out() const noexcept -> vec3 { return basis[2]; }
  constexpr auto origin() const noexcept -> vec3 { return basis[3]; }

  constexpr void set_origin(vec3 position) noexcept {
    basis[3] = vec4(position, 1.0);
  }

  constexpr void look_at(coordinate_system const& world) noexcept {
    basis = lookAt(origin(), world.origin(), world.up());
  }
};

constexpr auto view_matrix(coordinate_system const& world) noexcept {
  return inverse(world.basis);
  // mat3 m = transpose(mat3(world.basis));
  // vec3 o = world.origin();
  // mat4 r = m;
  // r[3] = vec4(-m * o, 1.0);
  // return r;
}

struct horizontal_coordinates {
  real radius{};
  real altitude{};
  real azimuth{};
};

template <typename result_type = vec3>
constexpr auto cartesian(horizontal_coordinates x) noexcept {
  using std::cos;
  using std::sin;
  const auto r = x.radius;
  const auto sin_theta = sin(x.altitude);
  const auto cos_theta = cos(x.altitude);
  const auto sin_phi = sin(x.azimuth);
  const auto cos_phi = cos(x.azimuth);
  return result_type{r * cos_theta * sin_phi,  //
                     r * sin_theta,            //
                     r * cos_theta * cos_phi};
}

constexpr auto cartesian(horizontal_coordinates r,
                         coordinate_system const& basis) noexcept {
  return basis(cartesian(r));
}

constexpr auto local_frame(horizontal_coordinates x) noexcept {
  using std::cos;
  using std::sin;
  const auto r = x.radius;
  const auto sin_theta = sin(x.altitude);
  const auto cos_theta = cos(x.altitude);
  const auto sin_phi = sin(x.azimuth);
  const auto cos_phi = cos(x.azimuth);
  mat4 m{1.0};
  m[0] = vec4(cos_phi, 0, -sin_phi, 0);
  m[1] = vec4(-sin_theta * sin_phi,  //
              cos_theta,             //
              -sin_theta * cos_phi, 0);
  m[2] = vec4(cos_theta * sin_phi,  //
              sin_theta,            //
              cos_theta * cos_phi, 0);
  return coordinate_system{m};
}

constexpr auto local_frame(horizontal_coordinates x,
                           coordinate_system const& basis) noexcept {
  return coordinate_system{basis.basis * local_frame(x).basis};
}

struct camera_base : coordinate_system {
  float _vfov = pi / 4;
  float _near = 0.1f;
  float _far = 1000.0f;
};

struct perspective_camera : camera_base {
  using base = camera_base;

  constexpr void fit(vec3 origin, real radius) noexcept {
    basis = glm::translate(mat4(1.0),
                           origin + vec3(0, 0, radius / std::tan(_vfov / 2)));
  }
};

struct orthographic_camera : camera_base {
  using base = camera_base;

  void fit(vec3 origin, real radius) noexcept {
    basis = glm::translate(mat4(1.0), origin + vec3(0, 0, radius));
    _vfov = 2 * radius;
  }
};

constexpr auto vfov(perspective_camera const& camera) noexcept {
  return camera._vfov;
}

constexpr auto projection(perspective_camera const& camera,
                          viewport const& screen) noexcept {
  return glm::perspective(vfov(camera), aspect_ratio(screen),  //
                          camera._near, camera._far);
}

constexpr auto projection(orthographic_camera const& camera,
                          viewport const& screen) noexcept {
  const auto y = camera._vfov / 2;
  const auto x = aspect_ratio(screen) * y;
  return glm::ortho(-x, x, -y, y, camera._near, camera._far);
}

//
constexpr auto perspective_zoom(real distance, real scale) noexcept {
  return distance * exp(-scale);
}
constexpr auto orthographic_zoom(real fov, real scale) noexcept {
  return fov * exp(scale);
}

struct spherical_observer {
  coordinate_system basis{};
  real radius{};
  real altitude{};
  real azimuth{};

  real _radius{};
  real _altitude{};
  real _azimuth{};

  void turn(vec2 angle) noexcept {
    altitude = _altitude + angle.y;
    azimuth = _azimuth + angle.x;
    constexpr auto bound = pi / 2 - 1e-5f;
    altitude = std::clamp(altitude, -bound, bound);
  }

  void zoom(real scale) noexcept { radius = _radius * exp(-scale); }

  void commit() {
    _radius = radius;
    _altitude = altitude;
    _azimuth = azimuth;
  }

  auto transform() const noexcept {
    return glm::lookAt(
        basis(radius * vec3{std::cos(altitude) * std::sin(azimuth),  //
                            std::sin(altitude),                      //
                            std::cos(altitude) * std::cos(azimuth)}),
        basis.origin(), basis.up());
  }
};

struct free_spherical_observer {
  coordinate_system basis{};
  real radius{};
  real altitude{};
  real azimuth{};

  real _radius{};
  real _altitude{};
  real _azimuth{};

  void turn(vec2 angle) noexcept {
    altitude = _altitude + angle.y;
    azimuth = _azimuth + angle.x;
    constexpr auto bound = pi / 2 - 1e-5f;
    altitude = std::clamp(altitude, -bound, bound);
  }

  void zoom(real scale) noexcept { radius = _radius * exp(-scale); }

  void commit() {
    mat4 r{1.0};
    r[0] = vec4(std::cos(azimuth), 0, -std::sin(azimuth), 0.0);
    r[1] = vec4(-std::sin(altitude) * std::sin(azimuth),  //
                std::cos(altitude),                       //
                -std::sin(altitude) * std::cos(azimuth), 0.0);
    r[2] = vec4(std::cos(altitude) * std::sin(azimuth),  //
                std::sin(altitude),                      //
                std::cos(altitude) * std::cos(azimuth), 0.0);

    basis.basis *= r;
    _radius = radius;
    _altitude = altitude = 0;
    _azimuth = azimuth = 0;
  }

  auto transform() const noexcept {
    return glm::lookAt(
        basis(radius * vec3{std::cos(altitude) * std::sin(azimuth),  //
                            std::sin(altitude),                      //
                            std::cos(altitude) * std::cos(azimuth)}),
        basis.origin(), basis.up());
  }
};

}  // namespace demo::opengl
