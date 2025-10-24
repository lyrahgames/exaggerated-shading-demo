#version 460 core

uniform vec3 light = vec3(1,-1,-0.1);

in vec3 normal;

layout (location = 0) out vec4 frag_color;

void main() {
  // float s = abs(normalize(normal).z);
  // float light = 0.2 + 1.0 * pow(s, 1000) + 0.75 * pow(s, 0.2);

  float s = dot(normalize(normal), -normalize(light));
  float a = 25.0;
  float tmp = clamp(a * s, -1.0, 1.0);
  float light = 0.5 + 0.5 * tmp;

  frag_color = vec4(vec3(light), 1.0);
}
