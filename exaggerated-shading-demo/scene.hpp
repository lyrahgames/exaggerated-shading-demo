#pragma once
#include "aabb.hpp"
#include "stl_surface.hpp"

namespace demo {

struct scene {
  using size_type = uint32;
  static constexpr size_type invalid = -1;

  using real = float32;

  struct vertex {
    vec3 position;
    vec3 normal;
  };
  using vertex_index = size_type;

  using face = array<vertex_index, 3>;
  // struct face : array<vertex_index, 3> {
  //   using base = array<vertex_index, 3>;
  //   using base::base;
  // };
  using face_index = size_type;

  struct edge : array<vertex_index, 2> {
    struct info {
      face_index face;
    };

    struct hasher {
      auto operator()(const edge& e) const noexcept -> size_t {
        return (size_t(e[0]) << 7) ^ size_t(e[1]);
      }
    };
  };

  vector<vertex> vertices{};
  vector<face> faces{};

  unordered_map<edge, edge::info, edge::hasher> edges{};
  vector<vertex_index> neighbor_offsets{};
  vector<vertex_index> neighbors{};

  vector<vec4> smoothed_normals{};

  void generate_edges() {
    edges.clear();
    for (size_t i = 0; i < faces.size(); ++i) {
      const auto& f = faces[i];
      edges[edge{f[0], f[1]}].face = i;
      edges[edge{f[1], f[2]}].face = i;
      edges[edge{f[2], f[0]}].face = i;
    }

    neighbor_offsets.assign(vertices.size() + 1, 0);
    for (const auto& [e, _] : edges) {
      const auto vid = e[0];
      const auto nid = e[1];
      ++neighbor_offsets[vid];
    }
    for (size_type i = 0; i < vertices.size(); ++i)
      neighbor_offsets[i + 1] += neighbor_offsets[i];
    neighbors.resize(neighbor_offsets.back());
    for (const auto& [e, _] : edges) {
      const auto vid = e[0];
      const auto nid = e[1];
      neighbors[--neighbor_offsets[vid]] = nid;
    }
  }

  void smooth_normals(size_type scales) {
    smoothed_normals.resize(vertices.size() * scales);
    for (vertex_index vid = 0; vid < vertices.size(); ++vid) {
      auto n = vertices[vid].normal;
      for (auto k = neighbor_offsets[vid]; k < neighbor_offsets[vid + 1]; ++k)
        n += vertices[neighbors[k]].normal;
      smoothed_normals[vid] = vec4(normalize(n), 0.0);
    }
    for (int i = 1; i < scales; ++i) {
      const auto offset = i * vertices.size();
      const auto offset_1 = (i - 1) * vertices.size();
      for (vertex_index vid = 0; vid < vertices.size(); ++vid) {
        auto n = smoothed_normals[offset_1 + vid];
        for (auto k = neighbor_offsets[vid]; k < neighbor_offsets[vid + 1]; ++k)
          n += smoothed_normals[offset_1 + neighbors[k]];
        smoothed_normals[offset + vid] = normalize(n);
      }
    }
  }
};

auto scene_from(const filesystem::path& path) -> scene;

inline auto scene_from(const stl_surface& stl) -> scene {
  scene s{};
  s.vertices.reserve(stl.triangles.size() * 3);
  s.faces.reserve(stl.triangles.size());
  for (const auto& t : stl.triangles) {
    s.vertices.emplace_back(t.vertex[0], t.normal);
    s.vertices.emplace_back(t.vertex[1], t.normal);
    s.vertices.emplace_back(t.vertex[2], t.normal);

    const auto n = static_cast<scene::vertex_index>(s.vertices.size());
    s.faces.push_back({n - 3, n - 2, n - 1});
  }
  return s;
}

inline auto aabb_from(const scene& s) noexcept -> aabb3 {
  aabb3 result{};
  for (const auto& v : s.vertices) result = aabb{result, v.position};
  return result;
}

}  // namespace demo
