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
  for (size_t vid = 0; vid < in->mNumVertices; ++vid)
    out.vertices.emplace_back(vec3_from(in->mVertices[vid]),
                              vec3_from(in->mNormals[vid]));
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
}

static void load_meshes(const aiScene* in, scene& out) {
  out.meshes.resize(in->mNumMeshes);
  for (size_t mid = 0; mid < in->mNumMeshes; ++mid)
    load(in->mMeshes[mid], out.meshes[mid]);
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

  Assimp::Importer importer{};

  // Assimp doesn’t promise successful triangulation for every arbitrary polygon.
  // If the input mesh has degenerate or invalid faces, you can get leftover stuff.
  const auto post_processing =
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals |
      aiProcess_JoinIdenticalVertices | aiProcess_RemoveComponent |
      /*aiProcess_OptimizeMeshes |*/ /*aiProcess_OptimizeGraph |*/
      aiProcess_FindDegenerates /*| aiProcess_DropNormals*/;

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

static void print_meshes(struct scene const& scene) {
  if (not scene.meshes.empty()) {
    const auto size = scene.meshes.size();
    const auto id_width = static_cast<int>(
        std::ceil(std::log10(static_cast<scene::real>(size + 1))));
    fmt::print(fmt::emphasis::bold, "Meshes:\n");
    for (size_t i = 0; i < scene.meshes.size() - 1; ++i) {
      const auto& mesh = scene.meshes[i];
      fmt::println("├─◬ {:>{}}: {}", i, id_width, mesh.name);
      fmt::println("│   #v = {:>8}", mesh.vertices.size());
      fmt::println("│   #f = {:>8}", mesh.faces.size());
    }
    {
      const auto& mesh = scene.meshes.back();
      fmt::println("└─◬ {:>{}}: {}", scene.meshes.size() - 1, id_width,
                   mesh.name);
      fmt::println("    #v = {:>8}", mesh.vertices.size());
      fmt::println("    #f = {:>8}", mesh.faces.size());
    }
    fmt::println("");
  }
}

static void pretty_print_node(struct scene const& scene,
                              scene::node const& node,
                              std::string const& prefix,
                              std::string const& child_prefix) {
  fmt::println("{}☋ {}: {}: {}", prefix, node.index, node.name,
               fmt::format(fmt::fg(fmt::color::gray), "{} ◬, {} ↣",
                           node.meshes.size(), node.bone_entries.size()));

  auto property_prefix = child_prefix;
  if (node.children.empty())
    property_prefix += "  ";
  else
    property_prefix += "│ ";

  for (auto mid : node.meshes)
    fmt::println("{}{}", property_prefix,
                 fmt::format(fmt::fg(fmt::color::gray), "◬ {}: {}", mid,
                             scene.meshes[mid].name));

  for (auto [mid, weights] : node.bone_entries) {
    if (weights.empty()) continue;
    fmt::println("{}{}", property_prefix,
                 fmt::format(fmt::fg(fmt::color::gray), "↣ ◬ {}: {} ({})", mid,
                             scene.meshes[mid].name, weights.size()));
  }

  auto it = node.children.begin();
  if (it == node.children.end()) return;
  auto next = it;
  ++next;
  for (; next != node.children.end(); ++next) {
    pretty_print_node(scene, *it, child_prefix + "├─", child_prefix + "│ ");
    it = next;
  }
  pretty_print_node(scene, *it, child_prefix + "└─", child_prefix + "  ");
}

static void print_hierarchy(struct scene const& scene) {
  fmt::print(fmt::emphasis::bold, "Hierarchy:\n");
  pretty_print_node(scene, scene.root, "", "");
  fmt::println("");
}

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

void print(struct scene const& scene) {
  fmt::println("Scene: {}", scene.name);
  print_meshes(scene);
  print_hierarchy(scene);
  print_animations(scene);
}

// auto scene_from(const filesystem::path& path) -> scene {
//   // Generate functor for prefixed error messages.
//   //
//   const auto throw_error = [&](czstring str) {
//     throw runtime_error("Failed to load 'polyhedral_surface' from path '"s +
//                         path.string() + "'. " + str);
//   };

//   if (!exists(path)) throw_error("The path does not exist.");

//   Assimp::Importer importer{};

//   // Assimp only needs to generate a continuously connected scene.
//   // So, a lot of information can be stripped from vertices.
//   //
//   importer.SetPropertyInteger(
//       AI_CONFIG_PP_RVC_FLAGS,
//       /*aiComponent_NORMALS |*/ aiComponent_TANGENTS_AND_BITANGENTS |
//           aiComponent_COLORS |
//           /*aiComponent_TEXCOORDS |*/ aiComponent_BONEWEIGHTS |
//           aiComponent_ANIMATIONS | aiComponent_TEXTURES | aiComponent_LIGHTS |
//           aiComponent_CAMERAS /*| aiComponent_MESHES*/ | aiComponent_MATERIALS);

//   // After the stripping and loading,
//   // certain post processing steps are mandatory.
//   //
//   const auto post_processing =
//       aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals |
//       aiProcess_JoinIdenticalVertices | aiProcess_RemoveComponent |
//       /*aiProcess_OptimizeMeshes |*/ /*aiProcess_OptimizeGraph |*/
//       aiProcess_FindDegenerates /*| aiProcess_DropNormals*/;

//   // Now, let Assimp actually load a scene scene from the given file.
//   //
//   const auto input = importer.ReadFile(path.c_str(), post_processing);

//   // Check whether Assimp could load the file at all.
//   //
//   if (!input || input->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !input->mRootNode)
//     throw_error("Assimp could not process the file.");

//   // Now, transform the loaded mesh data from
//   // Assimp's internal structure to a polyhedral scene.
//   //
//   struct scene scene{};

//   // First, get the total number of vertices
//   // and faces and for all meshes.
//   //
//   size_t vertex_count = 0;
//   size_t face_count = 0;
//   for (size_t i = 0; i < input->mNumMeshes; ++i) {
//     vertex_count += input->mMeshes[i]->mNumVertices;
//     face_count += input->mMeshes[i]->mNumFaces;
//   }
//   //
//   scene.vertices.resize(vertex_count);
//   scene.faces.resize(face_count);

//   // Iterate over all meshes and get the vertex and face information.
//   // All meshes will be linearly stored in one polyhedral scene.
//   //
//   uint32 vertex_offset = 0;
//   uint32 face_offset = 0;
//   for (size_t mid = 0; mid < input->mNumMeshes; ++mid) {
//     // Vertices of the Mesh
//     //
//     for (size_t vid = 0; vid < input->mMeshes[mid]->mNumVertices; ++vid) {
//       scene.vertices[vid + vertex_offset] = {
//           .position = {input->mMeshes[mid]->mVertices[vid].x,  //
//                        input->mMeshes[mid]->mVertices[vid].y,  //
//                        input->mMeshes[mid]->mVertices[vid].z},
//           .normal = {input->mMeshes[mid]->mNormals[vid].x,  //
//                      input->mMeshes[mid]->mNormals[vid].y,  //
//                      input->mMeshes[mid]->mNormals[vid].z}};
//     }

//     // Faces of the Mesh
//     //
//     for (size_t fid = 0; fid < input->mMeshes[mid]->mNumFaces; ++fid) {
//       // All faces need to be triangles.
//       // So, use a simple triangulation of polygons.
//       const auto corners = input->mMeshes[mid]->mFaces[fid].mNumIndices;
//       for (size_t k = 2; k < corners; ++k) {
//         scene.faces[face_offset + fid] = {
//             input->mMeshes[mid]->mFaces[fid].mIndices[0] + vertex_offset,  //
//             input->mMeshes[mid]->mFaces[fid].mIndices[k - 1] +
//                 vertex_offset,  //
//             input->mMeshes[mid]->mFaces[fid].mIndices[k] + vertex_offset};
//       }
//     }

//     // Update offsets to not overwrite previously written meshes.
//     //
//     vertex_offset += input->mMeshes[mid]->mNumVertices;
//     face_offset += input->mMeshes[mid]->mNumFaces;
//   }

//   scene.generate_edges();
//   scene.smooth_normals(10);
//   return scene;
// }

}  // namespace demo
