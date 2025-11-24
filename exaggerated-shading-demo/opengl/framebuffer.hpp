#pragma once
#include "renderbuffer.hpp"
#include "texture.hpp"

namespace demo::opengl {

struct default_framebuffer_t {
  static void bind() noexcept { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
};
inline constexpr default_framebuffer_t default_framebuffer{};

///
///
struct framebuffer_base : object {
  using base = object;
  using base::base;

  ///
  ///
  static auto create() noexcept -> framebuffer_base {
    native_handle_type handle;
    glCreateFramebuffers(1, &handle);
    return framebuffer_base{handle};
  }

  ///
  ///
  static void destroy(framebuffer_base& resource) noexcept {
    glDeleteFramebuffers(1, &resource.handle);
  }

  ///
  ///
  bool valid() const noexcept { return glIsFramebuffer(handle) == GL_TRUE; }

  ///
  ///
  void bind() const noexcept {
    glBindFramebuffer(GL_FRAMEBUFFER, native_handle());
  }

  void attach(texture2_view texture) const noexcept {
    glNamedFramebufferTexture(native_handle(), GL_COLOR_ATTACHMENT0,
                              texture.native_handle(), 0);
  }

  void attach(renderbuffer_view rbo) const noexcept {
    glNamedFramebufferRenderbuffer(native_handle(), GL_DEPTH_STENCIL_ATTACHMENT,
                                   GL_RENDERBUFFER, rbo.native_handle());
  }
};

///
///
STRICT_FINAL_USING(framebuffer, unique<framebuffer_base>);

///
///
STRICT_FINAL_USING(framebuffer_view, view<framebuffer>);

namespace current_framebuffer {

inline void set(default_framebuffer_t fbo) noexcept {
  fbo.bind();
}

inline void set(framebuffer_view fbo) noexcept {
  fbo.bind();
}

inline void set_clear_color(float r,
                            float g,
                            float b,
                            float a = 1.0f) noexcept {
  glClearColor(r, g, b, a);
}

inline void set_clear_color(vec4 color) noexcept {
  set_clear_color(color.r, color.g, color.b, color.a);
}

inline void set_clear_color(vec3 color) noexcept {
  set_clear_color(color.r, color.g, color.b);
}

inline void clear() noexcept {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

}  // namespace current_framebuffer

}  // namespace demo::opengl
