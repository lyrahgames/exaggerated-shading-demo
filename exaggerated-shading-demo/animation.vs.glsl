#version 460 core

uniform mat4 projection;
uniform mat4 view;

struct bone_weight {
  uint bid;
  float weight;
};

layout (std430, binding = 0) readonly
buffer bone_weight_offsets {
  uint offsets[];
};

layout (std430, binding = 1) readonly
buffer bone_weight_data {
  bone_weight weights[];
};

layout (std430, binding = 2) readonly
buffer bone_transforms {
  mat4 transforms[];
};

mat4 bone_transform(uint vid) {
  mat4 result = mat4(0.0);
  for (uint i = offsets[vid]; i < offsets[vid + 1]; ++i)
    result += weights[i].weight * transforms[weights[i].bid];
  return result;
}

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;

out vec3 normal;

void main() {
  const mat4 m = view * bone_transform(gl_VertexID);
  gl_Position = projection * m * vec4(p, 1.0);
  normal = vec3(transpose(inverse(m)) * vec4(n, 0.0));
}
