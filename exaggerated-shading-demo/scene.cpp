#include "scene.hpp"
//
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

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

static void load(const aiScene* in, scene& out) noexcept {
  out.name = in->mName.C_Str();
  load_meshes(in, out);
  // load_hierarchy(in, out);
  // load_animations(in, out);
}

void load(std::filesystem::path const& path, scene& out) {
  if (!exists(path)) throw scene_file_error("Path does not exist.");

  Assimp::Importer importer{};

  const auto post_processing =
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals |
      aiProcess_JoinIdenticalVertices | aiProcess_RemoveComponent |
      /*aiProcess_OptimizeMeshes |*/ /*aiProcess_OptimizeGraph |*/
      aiProcess_FindDegenerates /*| aiProcess_DropNormals*/;

  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);  // ?

  const auto in = importer.ReadFile(path.c_str(), post_processing);
  if (!in || in->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !in->mRootNode)
    throw scene_error("Assimp failed to load the file.");

  load(in, out);
  // update_skeleton(out);
}

auto scene_from(std::filesystem::path const& path) -> scene {
  scene out{};
  load(path, out);
  return out;
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
