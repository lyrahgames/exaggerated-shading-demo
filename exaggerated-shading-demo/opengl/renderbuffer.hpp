#pragma once
#include "defaults.hpp"

namespace demo::opengl {

///
///
struct renderbuffer_base : object {
  using base = object;
  using base::base;

  ///
  ///
  static auto create() noexcept -> renderbuffer_base {
    native_handle_type handle;
    glCreateRenderbuffers(1, &handle);
    return renderbuffer_base{handle};
  }

  ///
  ///
  static void destroy(renderbuffer_base& resource) noexcept {
    glDeleteRenderbuffers(1, &resource.handle);
  }

  ///
  ///
  bool valid() const noexcept { return glIsRenderbuffer(handle) == GL_TRUE; }

  ///
  ///
  void alloc(GLsizei width, GLsizei height) const noexcept {
    glNamedRenderbufferStorage(native_handle(), GL_DEPTH24_STENCIL8,  //
                               width, height);
  }
};

///
///
STRICT_FINAL_USING(renderbuffer, unique<renderbuffer_base>);

///
///
STRICT_FINAL_USING(renderbuffer_view, view<renderbuffer>);

}  // namespace demo::opengl
