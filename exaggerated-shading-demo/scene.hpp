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
#include <generator>
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

  // struct texture {
  //   std::filesystem::path path{};
  //   texture_index id{};
  // };

  struct material {
    std::string name{};
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;

    texture_index albedo_map{};
    texture_index normal_map{};
    texture_index orm_map{};

    // std::filesystem::path base_color_map;
    // std::filesystem::path normal_map;
    // std::filesystem::path metal_map;
    // std::filesystem::path ambient_texture;
    // std::filesystem::path diffuse_texture;
    // std::filesystem::path specular_texture;
  };

  struct vertex {
    vec3 position;
    vec3 normal;
    vec2 texuv;
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
  std::vector<std::filesystem::path> textures{""};
  std::map<std::filesystem::path, texture_index> texture_map{};

  auto texture_id(std::filesystem::path const& path) -> texture_index {
    auto p = weakly_canonical(path);
    const auto [it, inserted] = texture_map.emplace(p, textures.size());
    if (inserted) textures.push_back(p);
    return it->second;
  }

  std::vector<material> materials{};
  std::vector<mesh> meshes{};
  //
  std::map<std::string_view, node&> node_name_map{};
  std::vector<animation> animations{};

  struct skeleton skeleton{};

  static constexpr auto char_width(std::unsigned_integral auto n) -> int {
    return (n == 0) ? 1 : std::ceil(std::log10(1.0f + n));
  };

  // To-Do:
  // - [ ] add possibility to disable printing id
  // - [ ] add horizontal and vertical spacing options
  //
  auto list_info(
      std::ranges::input_range auto&& data,
      // auto&& info,  // invocable with elements and must return string range
      std::string_view icon = "") const -> std::generator<std::string> {
    if (std::ranges::empty(data)) co_return;
    const auto size = std::ranges::size(data);
    const auto width = char_width(size - 1);
    auto it = std::ranges::begin(data);
    for (std::size_t i = 0; i < size - 1; ++i, ++it) {
      auto infos = info(*it);
      auto it = std::ranges::begin(infos);
      co_yield std::format("├─{} {:>{}}: {}", icon, i, width, *it);
      ++it;
      for (; it != std::ranges::end(infos); ++it)  //
        co_yield std::format("│ {}", *it);
      // co_yield std::format("│   {}", *it);
      // co_yield std::format("│");
    }
    {
      auto i = size - 1;
      auto infos = info(*it);
      auto it = std::ranges::begin(infos);
      co_yield std::format("└─{} {:>{}}: {}", icon, i, width, *it);
      ++it;
      for (; it != std::ranges::end(infos); ++it)  //
        co_yield std::format("  {}", *it);
      // co_yield std::format("    {}", *it);
      // co_yield std::format("");
    }
  }

  auto info(mesh const&) const -> std::generator<std::string>;
  auto info(material const&) const -> std::generator<std::string>;
  auto info(std::filesystem::path const& texture) const
      -> std::generator<std::string>;
  auto info(node const&) const -> std::generator<std::string>;
  auto info(animation const&) const -> std::generator<std::string>;

  auto mesh_list_info() const -> std::generator<std::string>;
  void print_mesh_list() const;

  auto material_list_info() const -> std::generator<std::string>;
  void print_material_list() const;

  auto texture_list_info() const -> std::generator<std::string>;
  void print_texture_list() const;

  auto animation_list_info() const -> std::generator<std::string>;
  void print_animation_list() const;

  auto hierarchy_info() const -> std::generator<std::string>;
  void print_hierarchy() const;
};

struct scene_error : cpptrace::runtime_error {
  using base = cpptrace::runtime_error;
  using base::base;
};

struct scene_file_error : scene_error {
  using base = scene_error;
  using base::base;
};

// void print_mesh_list(scene const&);

/// Print basic scene information
///
// void print(scene const& s);

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

}  // namespace demo
