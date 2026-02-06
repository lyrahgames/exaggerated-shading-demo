#version 460 core

uniform mat4 projection;
uniform mat4 view;

uniform mat4 frame = mat4(1.0);

const vec4 points[] = {
  vec4(0, 0, 0, 1),
  vec4(1, 0, 0, 1),
  vec4(-1, 0, 0, 1),
  vec4(0, 1, 0, 1),
  vec4(0, -1, 0, 1),
  vec4(0, 0, 1, 1),
  vec4(0, 0, -1, 1),
};

void main() {
  gl_Position = projection * view * frame * points[gl_VertexID];
}
