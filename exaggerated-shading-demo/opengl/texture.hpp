#pragma once
#include "defaults.hpp"

namespace demo::opengl {

// GL_TEXTURE_1D
// GL_TEXTURE_2D
// GL_TEXTURE_3D
// GL_TEXTURE_1D_ARRAY
// GL_TEXTURE_2D_ARRAY
// GL_TEXTURE_RECTANGLE
// GL_TEXTURE_CUBE_MAP
// GL_TEXTURE_CUBE_MAP_ARRAY
// GL_TEXTURE_BUFFER
// GL_TEXTURE_2D_MULTISAMPLE
// GL_TEXTURE_2D_MULTISAMPLE_ARRAY

///
///
struct texture_base : object {
  using base = object;
  using base::base;

  ///
  ///
  static auto create(GLenum target) -> texture_base {
    native_handle_type handle{};
    glCreateTextures(target, 1, &handle);
    return texture_base{handle};
  }

  ///
  ///
  static void destroy(texture_base& resource) noexcept {
    glDeleteTextures(1, &resource.handle);
  }

  ///
  ///
  bool valid() const noexcept { return glIsTexture(handle) == GL_TRUE; }
};

struct texture2_base : texture_base {
  using base = texture_base;
  using base::base;

  ///
  ///
  static auto create() -> texture2_base {
    return texture2_base{base::create(GL_TEXTURE_2D).native_handle()};
  }

  ///
  ///
  using base::destroy;

  void alloc(GLsizei levels,
             GLenum format,
             GLsizei width,
             GLsizei height) const noexcept {
    glTextureStorage2D(native_handle(), levels, format, width, height);
  }

  void set_min_filter_to_linear() const noexcept {
    glTextureParameteri(native_handle(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }
  void set_min_filter_to_nearest() const noexcept {
    glTextureParameteri(native_handle(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }

  void set_mag_filter_to_linear() const noexcept {
    glTextureParameteri(native_handle(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  void set_mag_filter_to_nearest() const noexcept {
    glTextureParameteri(native_handle(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  void clamp_to_edges() const noexcept {
    glTextureParameteri(native_handle(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(native_handle(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  void bind_to_unit(GLuint location) const noexcept {
    glBindTextureUnit(location, native_handle());
  }
};

///
///
STRICT_FINAL_USING(texture2, unique<texture2_base>);

///
///
STRICT_FINAL_USING(texture2_view, view<texture2>);

}  // namespace demo::opengl
