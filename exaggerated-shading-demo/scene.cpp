#include "scene.hpp"
//
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
//
#include <fmt/base.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

namespace demo {

namespace {
// scoped_chdir -> scoped_current_path
// this is not shared_scoped_current_path
// and as such not save in a multi-threaded context
// `std::filesystem::current_path` itself is atomic
// but also not save for multiple threads due to the shared global state.
struct scoped_chdir final {
  std::filesystem::path cwd{};
  scoped_chdir(std::filesystem::path const& path)
      : cwd{std::filesystem::current_path()} {
    current_path(path);
  }
  ~scoped_chdir() noexcept { current_path(cwd); }
};
}  // namespace

static auto vec3_from(const aiVector3D& v) noexcept -> scene::vec3 {
  return {v.x, v.y, v.z};
};

static auto quat_from(const aiQuaternion& q) noexcept -> scene::quat {
  return {q.w, q.x, q.y, q.z};
}

static auto mat4_from(const aiMatrix4x4& from) noexcept -> scene::mat4 {
  return {from.a1, from.b1, from.c1, from.d1,  //
          from.a2, from.b2, from.c2, from.d2,  //
          from.a3, from.b3, from.c3, from.d3,  //
          from.a4, from.b4, from.c4, from.d4};
};

static void load(const aiMesh* in, scene::mesh& out) {
  out.name = in->mName.C_Str();
  // Vertices
  out.vertices.reserve(in->mNumVertices);
  for (size_t vid = 0; vid < in->mNumVertices; ++vid) {
    out.vertices.emplace_back(vec3_from(in->mVertices[vid]),
                              vec3_from(in->mNormals[vid]));

    if (in->HasTextureCoords(0)) {
      out.vertices[vid].texuv.x = in->mTextureCoords[0][vid].x;
      out.vertices[vid].texuv.y = in->mTextureCoords[0][vid].y;
    }
  }
  // Faces
  out.faces.reserve(in->mNumFaces);
  for (size_t fid = 0; fid < in->mNumFaces; ++fid) {
    auto face = in->mFaces[fid];
    // All faces need to be triangles and Assimp does not
    // ensure this even when triangulation flag is set.
    // Use a simple triangulation of polygons.
    for (size_t k = 2; k < face.mNumIndices; ++k)
      out.faces.push_back({
          face.mIndices[0],
          face.mIndices[k - 1],
          face.mIndices[k],
      });
  }
  // Bones
  // The `scene` data structure stores all bone information and weights
  // in the its hierarchy's nodes and therefore bones are not handled here.

  out.material = in->mMaterialIndex;
}

static void load_meshes(const aiScene* in, scene& out) {
  out.meshes.resize(in->mNumMeshes);
  for (size_t mid = 0; mid < in->mNumMeshes; ++mid)
    load(in->mMeshes[mid], out.meshes[mid]);
}

static void load(const aiMaterial* in,
                 struct scene& scene,
                 scene::material& out) {
  if (aiString name; in->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
    out.name = name.C_Str();

  if (aiColor3D color; in->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
    out.diffuse = {color.r, color.g, color.b};

  if (aiColor3D color; in->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
    out.diffuse = {color.r, color.g, color.b};

  if (aiColor3D color; in->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
    out.specular = {color.r, color.g, color.b};

  if (float shininess; in->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
    out.shininess = shininess;

  if (in->GetTextureCount(aiTextureType_BASE_COLOR)) {
    if (aiString str;
        in->GetTexture(aiTextureType_BASE_COLOR, 0, &str) == AI_SUCCESS)
      out.albedo_map = scene.texture_id(str.C_Str());
  }
  if (in->GetTextureCount(aiTextureType_NORMALS)) {
    if (aiString str;
        in->GetTexture(aiTextureType_NORMALS, 0, &str) == AI_SUCCESS)
      out.normal_map = scene.texture_id(str.C_Str());
  }
  if (in->GetTextureCount(aiTextureType_METALNESS)) {
    if (aiString str;
        in->GetTexture(aiTextureType_METALNESS, 0, &str) == AI_SUCCESS)
      out.orm_map = scene.texture_id(str.C_Str());
  }
}

static void load_materials(const aiScene* in, scene& out) {
  out.materials.resize(in->mNumMaterials);
  for (size_t mid = 0; mid < in->mNumMaterials; ++mid)
    load(in->mMaterials[mid], out, out.materials[mid]);
}

/// Recursive function to load node entries from Assimp into the scene.
///
static void load(const aiNode* in,
                 scene::node& out,
                 scene::node_index& count,
                 scene::node* parent = nullptr) {
  // Index
  out.index = count++;
  // Name
  out.name = in->mName.C_Str();
  // Matrices
  out.transform = mat4_from(in->mTransformation);
  // Contained Meshes
  out.meshes.resize(in->mNumMeshes);
  for (size_t i = 0; i < in->mNumMeshes; ++i) out.meshes[i] = in->mMeshes[i];
  // Connectivity
  out.parent = parent;
  for (size_t i = 0; i < in->mNumChildren; ++i)
    load(in->mChildren[i], out.children.emplace_back(), count, &out);
}

static void traverse(scene::node& node, auto&& f) {
  std::invoke(f, node);
  for (auto& child : node.children) traverse(child, f);
}

static void traverse(const scene::node& node, auto&& f) {
  std::invoke(f, node);
  for (auto& child : node.children) traverse(child, f);
}

static void update_node_name_map(struct scene& scene) {
  scene.node_name_map.clear();
  traverse(scene.root, [&scene](scene::node& node) {
    scene.node_name_map.emplace(node.name, node);
  });
}

static void load_bone_entries(const aiScene* in, scene& out) {
  for (size_t mid = 0; mid < in->mNumMeshes; ++mid) {
    auto mesh = in->mMeshes[mid];
    for (size_t bid = 0; bid < mesh->mNumBones; ++bid) {
      auto bone = mesh->mBones[bid];
      auto& node = out.node_name_map.at(bone->mName.C_Str());
      node.offset = mat4_from(bone->mOffsetMatrix);
      auto& entry = node.bone_entries.emplace_back(mid);
      entry.weights.reserve(bone->mNumWeights);
      for (size_t wid = 0; wid < bone->mNumWeights; ++wid) {
        const auto vid = bone->mWeights[wid].mVertexId;
        const float weight = bone->mWeights[wid].mWeight;
        entry.weights.emplace_back(vid, weight);
      }
    }
  }
}

static void load_hierarchy(const aiScene* in, scene& out) {
  load(in->mRootNode, out.root, out.node_count);
  update_node_name_map(out);
  load_bone_entries(in, out);
}

auto scene::animation::channel::position(double time) const -> glm::mat4 {
  if (positions.empty()) return glm::mat4{1.0f};
  if (positions.size() == 1)
    return glm::translate(glm::mat4{1.0f}, positions[0].data);

  size_t i = 0;
  for (; i < positions.size() - 1; ++i)
    if (time < positions[i + 1].time) break;

  auto t1 = positions[i].time;
  auto t2 = positions[i + 1].time;
  auto t = (time - t1) / (t2 - t1);

  const auto p = glm::mix(positions[i].data, positions[i + 1].data, t);
  return glm::translate(glm::mat4{1.0f}, p);
}

auto scene::animation::channel::rotation(double time) const -> glm::mat4 {
  if (rotations.empty()) return glm::mat4{1.0f};
  if (rotations.size() == 1)
    return glm::toMat4(glm::normalize(rotations[0].data));

  size_t i = 0;
  for (; i < rotations.size() - 1; ++i)
    if (time < rotations[i + 1].time) break;

  auto t1 = rotations[i].time;
  auto t2 = rotations[i + 1].time;
  auto t = (time - t1) / (t2 - t1);

  const auto p = glm::slerp(rotations[i].data, rotations[i + 1].data, float(t));
  return glm::toMat4(glm::normalize(p));
}

auto scene::animation::channel::scaling(double time) const -> glm::mat4 {
  if (scalings.empty()) return glm::mat4{1.0f};
  if (scalings.size() == 1)
    return glm::scale(glm::mat4{1.0f}, scalings[0].data);

  size_t i = 0;
  for (; i < scalings.size() - 1; ++i)
    if (time < scalings[i + 1].time) break;

  auto t1 = scalings[i].time;
  auto t2 = scalings[i + 1].time;
  auto t = (time - t1) / (t2 - t1);

  const auto p = glm::mix(scalings[i].data, scalings[i + 1].data, t);
  return glm::scale(glm::mat4{1.0f}, p);
}

auto scene::animation::channel::transform(double time) const -> glm::mat4 {
  return position(time) * rotation(time) * scaling(time);
}

static void load(const aiNodeAnim* in, scene::animation::channel& out) {
  // Node Name
  out.node_name = in->mNodeName.C_Str();
  // Position Keys
  out.positions.reserve(in->mNumPositionKeys);
  for (size_t i = 0; i < in->mNumPositionKeys; ++i)
    out.positions.push_back(
        {in->mPositionKeys[i].mTime, vec3_from(in->mPositionKeys[i].mValue)});
  // Rotation Keys
  out.rotations.reserve(in->mNumRotationKeys);
  for (size_t i = 0; i < in->mNumRotationKeys; ++i)
    out.rotations.push_back(
        {in->mRotationKeys[i].mTime, quat_from(in->mRotationKeys[i].mValue)});
  // Scaling Keys
  out.scalings.reserve(in->mNumScalingKeys);
  for (size_t i = 0; i < in->mNumScalingKeys; ++i)
    out.scalings.push_back(
        {in->mScalingKeys[i].mTime, vec3_from(in->mScalingKeys[i].mValue)});
}

static void load(const aiAnimation* in, scene::animation& out) {
  // Name
  out.name = in->mName.C_Str();
  // Duration
  out.duration = in->mDuration;
  out.ticks = in->mTicksPerSecond;
  // Bone Animation Channels
  out.channels.resize(in->mNumChannels);
  for (size_t i = 0; i < in->mNumChannels; ++i)
    load(in->mChannels[i], out.channels[i]);
}

static void load_animations(const aiScene* in, scene& out) {
  out.animations.resize(in->mNumAnimations);
  for (size_t i = 0; i < in->mNumAnimations; ++i)
    load(in->mAnimations[i], out.animations[i]);
}

static void load(const aiScene* in, scene& out) noexcept {
  out.name = in->mName.C_Str();
  load_meshes(in, out);
  load_materials(in, out);
  load_hierarchy(in, out);
  load_animations(in, out);
}

static void traverse_skeleton_nodes(scene& s,
                                    scene::node& node,
                                    scene::bone_index parent) {
  s.skeleton.parents.push_back(parent);
  s.skeleton.nodes.emplace_back(&node);
  parent = s.skeleton.bones.size();
  s.skeleton.bone_name_map.emplace(node.name, parent);
  s.skeleton.bones.emplace_back(node.offset, node.transform);

  for (auto& child : node.children) traverse_skeleton_nodes(s, child, parent);
}

static void update_skeleton(scene& s) {
  // traverse_skeleton_nodes(s, s.root, -1, glm::mat4(1.0f));
  traverse_skeleton_nodes(s, s.root, -1);

  //
  s.skeleton.weights.resize(s.meshes.size());
  for (size_t mid = 0; auto& wdata : s.skeleton.weights) {
    wdata.offsets.assign(s.meshes[mid].vertices.size() + 1, 0);
    ++mid;
  }
  // Get the counts
  for (size_t bid = 0; bid < s.skeleton.bones.size(); ++bid) {
    auto& node = *s.skeleton.nodes[bid];
    for (auto& [mid, weights] : node.bone_entries) {
      for (auto& [vid, weight] : weights)
        ++s.skeleton.weights[mid].offsets[vid];
    }
  }
  // accumulate and allocate
  for (auto& data : s.skeleton.weights) {
    for (size_t i = 1; i < data.offsets.size(); ++i)
      data.offsets[i] += data.offsets[i - 1];
    data.data.resize(data.offsets.back());
  }
  // assign weights
  for (scene::bone_index bid = 0; bid < s.skeleton.bones.size(); ++bid) {
    auto& node = *s.skeleton.nodes[bid];
    for (auto& [mid, weights] : node.bone_entries) {
      for (auto& [vid, weight] : weights) {
        auto& wdata = s.skeleton.weights[mid];
        wdata.data[--wdata.offsets[vid]] = {bid, weight};
      }
    }
  }
}

void load(std::filesystem::path const& path, scene& out) {
  if (!exists(path)) throw scene_file_error("Path does not exist.");

  // Make it possible to load relative scene and texture files.
  scoped_chdir _{path.parent_path()};

  Assimp::Importer importer{};

  // Assimp doesn’t promise successful triangulation for every arbitrary polygon.
  // If the input mesh has degenerate or invalid faces, you can get leftover stuff.
  const auto post_processing = aiProcess_Triangulate /*|*/
      /*aiProcess_FlipUVs |*/               /*aiProcess_GenSmoothNormals |*/
      /*aiProcess_JoinIdenticalVertices |*/ /*aiProcess_RemoveComponent |*/
      /*aiProcess_OptimizeMeshes |*/        /*aiProcess_OptimizeGraph |*/
      /*aiProcess_FindDegenerates*/ /*| aiProcess_DropNormals*/;

  // Use simple node transforms that already include any pivot offsets/rotations.
  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);  // ?

  const auto in = importer.ReadFile(path.c_str(), post_processing);
  if (!in || in->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !in->mRootNode)
    throw scene_error("Assimp failed to load the file.");

  load(in, out);
  update_skeleton(out);
}

auto scene_from(std::filesystem::path const& path) -> scene {
  scene out{};
  load(path, out);
  return out;
}

auto scene::info(struct mesh const& mesh) const -> std::generator<std::string> {
  co_yield fmt::format("{}", (mesh.name.empty()) ? "<empty>" : mesh.name);
  co_yield fmt::format(fmt::fg(fmt::color::gray), "material  = {}: {}",
                       mesh.material, materials[mesh.material].name);
  co_yield fmt::format(fmt::fg(fmt::color::gray), "#vertices = {:>8}",
                       mesh.vertices.size());
  co_yield fmt::format(fmt::fg(fmt::color::gray), "#faces    = {:>8}",
                       mesh.faces.size());
}

auto scene::info(struct material const& material) const
    -> std::generator<std::string> {
  co_yield fmt::format("{}",
                       (material.name.empty()) ? "<empty>" : material.name);
  if (material.albedo_map)
    co_yield fmt::format(fmt::fg(fmt::color::gray), "albedo map = {}: {}",
                         material.albedo_map, textures[material.albedo_map]);
  if (material.normal_map)
    co_yield fmt::format(fmt::fg(fmt::color::gray), "normal map = {}: {}",
                         material.normal_map, textures[material.normal_map]);
  if (material.orm_map)
    co_yield fmt::format(fmt::fg(fmt::color::gray), "ORM map    = {}: {}",
                         material.orm_map, textures[material.orm_map]);
}

auto scene::info(std::filesystem::path const& texture) const
    -> std::generator<std::string> {
  co_yield fmt::format("{}", texture.string());
}

auto scene::info(struct animation const& animation) const
    -> std::generator<std::string> {
  co_yield fmt::format("☋ {}", animation.name);
  co_yield fmt::format("☋ {}x{}", animation.duration, animation.ticks);
  // fmt::print(fmt::emphasis::bold, "Animations:\n");
  // for (auto& anim : scene.animations) {
  //   fmt::println("  ☋ {}", anim.name);
  //   fmt::println("  time = {}", anim.duration);
  //   fmt::println("  tick = {}", anim.ticks);
  //   fmt::println("  Channels:");
  //   for (auto& channel : anim.channels)
  //     fmt::println("    {} ({},{},{})",  //
  //                  channel.node_name,    //
  //                  channel.positions.size(), channel.rotations.size(),
  //                  channel.scalings.size());
}

auto scene::info(struct node const& node) const -> std::generator<std::string> {
  co_yield fmt::format(
      "{}: {}", (node.name.empty()) ? "<empty>" : node.name,
      fmt::format(fmt::fg(fmt::color::gray), "{} ◬, {} ↣", node.meshes.size(),
                  node.bone_entries.size()));

  for (auto mid : node.meshes)
    co_yield fmt::format("{}{}", (node.children.empty()) ? "  " : "│ ",
                         fmt::format(fmt::fg(fmt::color::gray), "◬ {}: {}", mid,
                                     meshes[mid].name));

  for (auto [mid, weights] : node.bone_entries) {
    if (weights.empty()) continue;
    co_yield fmt::format(
        "{}{}", (node.children.empty()) ? "  " : "│ ",
        fmt::format(fmt::fg(fmt::color::gray), "↣ ◬ {}: {} ({})", mid,
                    meshes[mid].name, weights.size()));
  }

  for (auto&& str : list_info(node.children, "☋")) co_yield str;
}

auto scene::mesh_list_info() const -> std::generator<std::string> {
  co_yield fmt::format(fmt::emphasis::bold, "Meshes:");
  for (auto&& str : list_info(meshes, "◬"))
    co_yield std::forward<decltype(str)>(str);
}

auto scene::material_list_info() const -> std::generator<std::string> {
  co_yield fmt::format(fmt::emphasis::bold, "Materials:");
  for (auto&& str : list_info(materials, "◍"))
    co_yield std::forward<decltype(str)>(str);
}

auto scene::texture_list_info() const -> std::generator<std::string> {
  co_yield fmt::format(fmt::emphasis::bold, "Textures:");
  for (auto&& str : list_info(textures, "▧"))
    co_yield std::forward<decltype(str)>(str);
}

auto scene::animation_list_info() const -> std::generator<std::string> {
  co_yield fmt::format(fmt::emphasis::bold, "Animations:");
  for (auto&& str : list_info(animations, "⏯"))
    co_yield std::forward<decltype(str)>(str);
}

auto scene::hierarchy_info() const -> std::generator<std::string> {
  co_yield fmt::format(fmt::emphasis::bold, "Hierarchy:");
  for (auto&& str : list_info(std::views::single(root))) co_yield str;
}

static void print(std::generator<std::string> lines) {
  for (auto&& str : lines) std::println("{}", std::forward<decltype(str)>(str));
}

void scene::print_mesh_list() const {
  print(mesh_list_info());
}

void scene::print_material_list() const {
  print(material_list_info());
}

void scene::print_texture_list() const {
  print(texture_list_info());
}

void scene::print_animation_list() const {
  print(animation_list_info());
}

void scene::print_hierarchy() const {
  print(hierarchy_info());
}

// static void pretty_print_node(struct scene const& scene,
//                               scene::node const& node,
//                               std::string const& prefix,
//                               std::string const& child_prefix) {
//   fmt::println("{}☋ {}: {}: {}", prefix, node.index, node.name,
//                fmt::format(fmt::fg(fmt::color::gray), "{} ◬, {} ↣",
//                            node.meshes.size(), node.bone_entries.size()));

//   auto property_prefix = child_prefix;
//   if (node.children.empty())
//     property_prefix += "  ";
//   else
//     property_prefix += "│ ";

//   for (auto mid : node.meshes)
//     fmt::println("{}{}", property_prefix,
//                  fmt::format(fmt::fg(fmt::color::gray), "◬ {}: {}", mid,
//                              scene.meshes[mid].name));

//   for (auto [mid, weights] : node.bone_entries) {
//     if (weights.empty()) continue;
//     fmt::println("{}{}", property_prefix,
//                  fmt::format(fmt::fg(fmt::color::gray), "↣ ◬ {}: {} ({})", mid,
//                              scene.meshes[mid].name, weights.size()));
//   }

//   auto it = node.children.begin();
//   if (it == node.children.end()) return;
//   auto next = it;
//   ++next;
//   for (; next != node.children.end(); ++next) {
//     pretty_print_node(scene, *it, child_prefix + "├─", child_prefix + "│ ");
//     it = next;
//   }
//   pretty_print_node(scene, *it, child_prefix + "└─", child_prefix + "  ");
// }

static void print_animations(struct scene const& scene) {
  fmt::print(fmt::emphasis::bold, "Animations:\n");
  for (auto& anim : scene.animations) {
    fmt::println("  ☋ {}", anim.name);
    fmt::println("  time = {}", anim.duration);
    fmt::println("  tick = {}", anim.ticks);
    fmt::println("  Channels:");
    for (auto& channel : anim.channels)
      fmt::println("    {} ({},{},{})",  //
                   channel.node_name,    //
                   channel.positions.size(), channel.rotations.size(),
                   channel.scalings.size());
    fmt::println("");
  }
}

}  // namespace demo
