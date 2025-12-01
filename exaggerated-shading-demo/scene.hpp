#pragma once
#include <cstdint>
//
#include <filesystem>
#include <print>
#include <string>
//
#include <array>
#include <list>
#include <map>
#include <ranges>
#include <vector>
//
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
//
#include <glm/ext.hpp>
//
#include <glm/gtx/norm.hpp>
//
#include <cpptrace/cpptrace.hpp>

namespace demo {

struct scene {
  using size_type = std::uint32_t;

  using real = float;
  static constexpr auto infinity = std::numeric_limits<real>::infinity();

  using vec2 = glm::vec2;
  using vec3 = glm::vec3;
  using vec4 = glm::vec4;
  using mat4 = glm::mat4;
  using quat = glm::quat;

  using texture_index = size_type;
  using material_index = size_type;
  using vertex_index = size_type;
  using mesh_index = size_type;
  using node_index = size_type;
  using bone_index = std::int32_t;

  struct texture {};

  struct material {};

  struct vertex {
    vec3 position;
    vec3 normal;
    // vec3 tangent;
    // vec3 bitangent;
    // vec2[4] texuv;
    // vec4[4] color;
  };

  struct face : std::array<vertex_index, 3> {};

  struct mesh {
    material_index material{};
    std::string name{};
    std::vector<vertex> vertices{};
    std::vector<face> faces{};
  };

  struct bone_entry {
    struct weight_entry {
      vertex_index vertex{};
      real weight{};
    };
    // Data
    mesh_index mesh{};
    std::vector<weight_entry> weights{};
  };

  struct node {
    /// Connectivity Data
    /// Assumption: pointer to nodes (`node*`) do not get invalidated
    node* parent{};  // Use bare pointer type for easy no-overhead referencing.
    std::list<node> children{};  // Use `std::list` for easy modification.
    //
    /// Node Data
    node_index index{};
    std::string name{};
    mat4 offset{1.0f};
    mat4 transform{1.0f};
    std::vector<mesh_index> meshes{};
    std::vector<bone_entry> bone_entries{};
  };

  struct animation {
    struct channel {
      template <typename type>
      struct key {
        double time{};
        type data{};
      };
      using position_key = key<vec3>;
      using rotation_key = key<quat>;
      using scaling_key = key<vec3>;

      std::string node_name{};
      std::vector<position_key> positions{};
      std::vector<rotation_key> rotations{};
      std::vector<scaling_key> scalings{};

      auto position(double time) const -> mat4;
      auto rotation(double time) const -> mat4;
      auto scaling(double time) const -> mat4;
      auto transform(double time) const -> mat4;
    };

    std::string name{};
    double duration{};
    double ticks{};
    std::vector<channel> channels{};
  };

  struct skeleton {
    struct bone {
      mat4 offset;
      mat4 transform;
    };

    struct weight_data {
      struct entry {
        bone_index bone{};
        real weight{};
      };
      std::vector<size_type> offsets{};
      std::vector<entry> data{};
    };

    std::vector<bone_index> parents{};
    std::vector<bone> bones{};
    std::vector<node*> nodes{};
    std::vector<weight_data> weights{};

    std::map<std::string_view, bone_index> bone_name_map{};

    // std::vector<scene_animation> animations{};

    auto global_transforms(const scene::animation& animation, double time) const
        -> std::vector<mat4> {
      std::vector<mat4> result{};
      result.reserve(bones.size());
      for (size_t bid = 0; bid < bones.size(); ++bid)
        result.push_back(mat4(1.0f));
      // result.push_back(bones[bid].transform);

      for (const auto& channel : animation.channels) {
        if (auto it = bone_name_map.find(channel.node_name);
            it != bone_name_map.end()) {
          const auto bid = it->second;
          // const auto bid = bone_name_map.at(channel.node_name);
          result[bid] = channel.transform(time * animation.ticks);
        }
      }

      for (size_t bid = 0; bid < bones.size(); ++bid)
        if (parents[bid] < bones.size())
          result[bid] = result[parents[bid]] * result[bid];

      for (size_t bid = 0; bid < bones.size(); ++bid)
        result[bid] *= bones[bid].offset;
      // result[bid] = translate(mat4(1.0f), glm::vec3(1.0f + bid));

      return result;
    }
  };

  //
  std::string name{};
  node root{};
  node_index node_count{};
  // std::vector<texture> textures{};
  std::vector<material> materials{};
  std::vector<mesh> meshes{};
  //
  std::map<std::string_view, node&> node_name_map{};
  std::vector<animation> animations{};

  struct skeleton skeleton{};
};

struct scene_error : cpptrace::runtime_error {
  using base = cpptrace::runtime_error;
  using base::base;
};

struct scene_file_error : scene_error {
  using base = scene_error;
  using base::base;
};

/// Print basic scene information
///
void print(scene const& s);

/// Uses exceptions for simpler handling of loading.
/// Exceptions allow for easy abort across stack as soon as the first error appears.
/// We only want to load valid files. So, this behavior is good.
/// Thus, in this case, exceptions simplify implementation.
///
auto scene_from(std::filesystem::path const& path) -> scene;
void load(std::filesystem::path const& path, scene& out);

///
///
inline auto min_edge_length(scene::mesh const& mesh,
                            scene::real bound = scene::infinity) noexcept
    -> scene::real {
  using glm::distance;
  scene::real result = bound;
  for (auto vid : mesh.faces) {
    const auto e1 = distance(mesh.vertices[vid[0]].position,
                             mesh.vertices[vid[1]].position);
    const auto e2 = distance(mesh.vertices[vid[1]].position,
                             mesh.vertices[vid[2]].position);
    const auto e0 = distance(mesh.vertices[vid[2]].position,
                             mesh.vertices[vid[0]].position);
    result = std::min(result, std::min(e0, std::min(e1, e2)));
  }
  return result;
}

///
///
inline auto min_edge_length(scene const& s,
                            scene::real bound = scene::infinity) noexcept
    -> scene::real {
  scene::real result = bound;
  for (auto const& mesh : s.meshes) result = min_edge_length(mesh, result);
  return result;
}

///
///
inline auto center(scene::mesh const& mesh) noexcept -> scene::vec3 {
  scene::vec3 result{};
  for (auto const& v : mesh.vertices) result += v.position;
  return result /= mesh.vertices.size();
}

inline auto bounding_radius(scene::mesh const& mesh,
                            scene::vec3 center) noexcept -> scene::real {
  scene::real radius{};
  for (auto const& v : mesh.vertices)
    radius = std::max(radius, distance(center, v.position));
  return radius;
}

inline auto bounding_sphere(scene::mesh const& mesh) noexcept {
  const auto c = center(mesh);
  const auto r = bounding_radius(mesh, c);
  return std::tuple{c, r};
}

/// Must be adjusted to iterate through nodes!
inline auto center(scene const& s) noexcept {
  scene::vec3 result{};
  for (auto const& mesh : s.meshes) result += center(mesh);
  return result /= s.meshes.size();
}

inline auto bounding_sphere(scene const& s) noexcept {
  auto spheres = s.meshes | std::views::transform([](scene::mesh const& mesh) {
                   return bounding_sphere(mesh);
                 }) |
                 std::ranges::to<std::vector>();

  scene::vec3 center{};
  for (auto [c, r] : spheres) center += c;
  center /= spheres.size();

  scene::real radius{};
  for (auto [c, r] : spheres)
    radius = std::max(radius, r + distance(center, c));

  return std::tuple{center, radius};
}

// struct scene {
//   using size_type = uint32;
//   static constexpr size_type invalid = -1;

//   using real = float32;

//   struct vertex {
//     vec3 position;
//     vec3 normal;
//   };
//   using vertex_index = size_type;

//   using face = array<vertex_index, 3>;
//   // struct face : array<vertex_index, 3> {
//   //   using base = array<vertex_index, 3>;
//   //   using base::base;
//   // };
//   using face_index = size_type;

//   struct edge : array<vertex_index, 2> {
//     struct info {
//       face_index face;
//     };

//     struct hasher {
//       auto operator()(const edge& e) const noexcept -> size_t {
//         return (size_t(e[0]) << 7) ^ size_t(e[1]);
//       }
//     };
//   };

//   vector<vertex> vertices{};
//   vector<face> faces{};

//   unordered_map<edge, edge::info, edge::hasher> edges{};
//   vector<vertex_index> neighbor_offsets{};
//   vector<vertex_index> neighbors{};

//   vector<vec4> smoothed_normals{};

//   void generate_edges() {
//     edges.clear();
//     for (size_t i = 0; i < faces.size(); ++i) {
//       const auto& f = faces[i];
//       edges[edge{f[0], f[1]}].face = i;
//       edges[edge{f[1], f[2]}].face = i;
//       edges[edge{f[2], f[0]}].face = i;
//     }

//     neighbor_offsets.assign(vertices.size() + 1, 0);
//     for (const auto& [e, _] : edges) {
//       const auto vid = e[0];
//       const auto nid = e[1];
//       ++neighbor_offsets[vid];
//     }
//     for (size_type i = 0; i < vertices.size(); ++i)
//       neighbor_offsets[i + 1] += neighbor_offsets[i];
//     neighbors.resize(neighbor_offsets.back());
//     for (const auto& [e, _] : edges) {
//       const auto vid = e[0];
//       const auto nid = e[1];
//       neighbors[--neighbor_offsets[vid]] = nid;
//     }
//   }

//   void smooth_normals(size_type scales) {
//     smoothed_normals.resize(vertices.size() * scales);
//     for (vertex_index vid = 0; vid < vertices.size(); ++vid) {
//       auto n = vertices[vid].normal;
//       for (auto k = neighbor_offsets[vid]; k < neighbor_offsets[vid + 1]; ++k)
//         n += vertices[neighbors[k]].normal;
//       smoothed_normals[vid] = vec4(normalize(n), 0.0);
//     }
//     for (int i = 1; i < scales; ++i) {
//       const auto offset = i * vertices.size();
//       const auto offset_1 = (i - 1) * vertices.size();
//       for (vertex_index vid = 0; vid < vertices.size(); ++vid) {
//         auto n = smoothed_normals[offset_1 + vid];
//         for (auto k = neighbor_offsets[vid]; k < neighbor_offsets[vid + 1]; ++k)
//           n += smoothed_normals[offset_1 + neighbors[k]];
//         smoothed_normals[offset + vid] = normalize(n);
//       }
//     }
//   }
// };

// auto scene_from(const filesystem::path& path) -> scene;

// inline auto scene_from(const stl_surface& stl) -> scene {
//   scene s{};
//   s.vertices.reserve(stl.triangles.size() * 3);
//   s.faces.reserve(stl.triangles.size());
//   for (const auto& t : stl.triangles) {
//     s.vertices.emplace_back(t.vertex[0], t.normal);
//     s.vertices.emplace_back(t.vertex[1], t.normal);
//     s.vertices.emplace_back(t.vertex[2], t.normal);

//     const auto n = static_cast<scene::vertex_index>(s.vertices.size());
//     s.faces.push_back({n - 3, n - 2, n - 1});
//   }
//   return s;
// }

// inline auto aabb_from(const scene& s) noexcept -> aabb3 {
//   aabb3 result{};
//   for (const auto& v : s.vertices) result = aabb{result, v.position};
//   return result;
// }

// inline auto center(const scene& s) noexcept {
//   vec3 result{};
//   for (const auto& v : s.vertices) result += v.position;
//   return result /= s.vertices.size();
// }

// inline auto bounding_radius(const scene& s, vec3 center) noexcept {
//   real radius{};
//   for (const auto& v : s.vertices)
//     radius = std::max(radius, distance(center, v.position));
//   return radius;
// }

// inline auto bounding_sphere(const scene& s) noexcept {
//   const auto m = center(s);
//   const auto r = bounding_radius(s, m);
//   return std::tuple{m, r};
// }

}  // namespace demo
