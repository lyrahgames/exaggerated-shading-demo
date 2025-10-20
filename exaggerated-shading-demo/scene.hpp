#pragma once
#include "aabb.hpp"
#include "stl_surface.hpp"

namespace demo {

struct scene {
  struct vertex {
    vec3 position;
    vec3 normal;
  };

  vector<vertex> vertices{};
};

inline auto scene_from(const stl_surface& stl) -> scene {
  scene s{};
  s.vertices.reserve(stl.triangles.size() * 3);
  for (const auto& t : stl.triangles) {
    s.vertices.emplace_back(t.vertex[0], t.normal);
    s.vertices.emplace_back(t.vertex[1], t.normal);
    s.vertices.emplace_back(t.vertex[2], t.normal);
  }
  return s;
}

inline auto aabb_from(const scene& s) noexcept -> aabb3 {
  aabb3 result{};
  for (const auto& v : s.vertices) result = aabb{result, v.position};
  return result;
}

}  // namespace demo
