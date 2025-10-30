#version 460 core

uniform mat4 projection;
uniform mat4 view;

uniform vec4 light = vec4(1, -1, -0.1, 0.0);

layout (location = 0) in vec3 p;
layout (location = 1) in vec3 n;
layout (location = 2) in vec4 nn;

uniform uint count = 0;
uniform uint scales = 0;
uniform uint scale = 0;
layout (std430, binding = 0) readonly buffer smoothed_normals {
  vec4 normals[];
};

// out vec3 normal;
out float intensity;

void main() {
  gl_Position = projection * view * vec4(p, 1.0);

  // normal = vec3(view * vec4(n, 0.0));
  // normal = vec3(view * normals[scale * count + gl_VertexID]);


  const float a = 2.0;
  const vec3 l = -normalize(vec3(inverse(view) * light));
  float w = 1.0;
  float x = w * clamp(a * dot(n, l), -1.0, 1.0);
  for (uint i = 0; i < scales; ++i) {
    const float s = pow(pow(1.0 / sqrt(2.0), i + 1), 0.5);
    w += s;
    x += s * clamp(a * dot(l, vec3(normals[i * count + gl_VertexID])), -1.0, 1.0);
  }
  x /= w;
  x = 0.5 * (1.0 + x);
  x = 0.01 * x + 0.99 * (0.5 * (1.0 + clamp(dot(vec3(n), l), -1.0, 1.0)));
  // x = 0.01 * x + 0.99 * (0.5 * (1.0 + clamp(dot(vec3(normals[gl_VertexID]), l), -1.0, 1.0)));
  intensity = x;
}
