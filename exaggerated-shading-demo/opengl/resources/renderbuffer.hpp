#pragma once
#include "defaults.hpp"

namespace demo::opengl {

///
///
struct renderbuffer_base : identifier {
  using base = identifier;
  using base::base;  // Allow construction from `native_handle_type`.

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
    auto handle = resource.native_handle();
    glDeleteRenderbuffers(1, &handle);
  }

  ///
  ///
  bool valid() const noexcept {
    return glIsRenderbuffer(native_handle()) == GL_TRUE;
  }

  ///
  ///
  void alloc(GLsizei width, GLsizei height) const noexcept {
    glNamedRenderbufferStorage(native_handle(), GL_DEPTH24_STENCIL8,  //
                               width, height);
  }
};

///
///
STRICT_FINAL_USING(renderbuffer, unique_resource<renderbuffer_base>);

///
///
STRICT_FINAL_USING(renderbuffer_view, resource_view<renderbuffer>);

}  // namespace demo::opengl
