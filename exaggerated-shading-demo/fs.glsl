#version 460 core

in vec3 normal;

layout (location = 0) out vec4 frag_color;

void main() {
  float s = abs(normalize(normal).z);
  float light = 0.2 + 1.0 * pow(s, 1000) + 0.75 * pow(s, 0.2);
  frag_color = vec4(vec3(light), 1.0);
}
