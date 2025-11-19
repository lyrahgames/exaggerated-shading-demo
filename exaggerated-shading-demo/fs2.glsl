#version 460 core

// in vec3 normal;
in float intensity;

layout (location = 0) out vec4 frag_color;

void main() {
  // float s = abs(normalize(normal).z);
  // float light = 0.2 + 1.0 * pow(s, 1000) + 0.75 * pow(s, 0.2);
  frag_color = vec4(intensity, 0.0, intensity, 1.0);
}
