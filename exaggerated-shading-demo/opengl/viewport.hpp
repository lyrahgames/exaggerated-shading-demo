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

  constexpr auto aspect_ratio() const noexcept { return real(size.x) / size.y; }

  constexpr auto space(vec2 pixels) const noexcept {
    const auto t = 2 * (pixels - vec2(offset)) / size.y;
    return vec2{t.x - aspect_ratio(), 1 - t.y};
  }
};

constexpr auto width(viewport screen) noexcept {
  return screen.width();
}

constexpr auto height(viewport screen) noexcept {
  return screen.height();
}

constexpr auto left(viewport screen) noexcept {
  return screen.left();
}

constexpr auto right(viewport screen) noexcept {
  return screen.right();
}

constexpr auto top(viewport screen) noexcept {
  return screen.top();
}

constexpr auto bottom(viewport screen) noexcept {
  return screen.bottom();
}

constexpr auto aspect_ratio(viewport screen) noexcept {
  return screen.aspect_ratio();
}

}  // namespace demo::opengl
