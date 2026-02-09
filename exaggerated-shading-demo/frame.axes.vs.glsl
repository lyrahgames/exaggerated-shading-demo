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

const vec4 colors[] = {
  vec4(0.5, 0.5, 0.5, 1),
  vec4(1, 0.1, 0.1, 1),
  vec4(1, 0.1, 0.1, 1),
  vec4(0.0, 0.6, 0.3, 1),
  vec4(0.0, 0.6, 0.3, 1),
  vec4(0.1, 0.3, 1, 1),
  vec4(0.1, 0.3, 1, 1),
};

flat out vec4 color;
flat out int id;

void main() {
  id = (gl_VertexID >> 1) + 1;
  uint vid = 0;
  if (bool(gl_VertexID & 1)) vid = id;

  color = colors[id];
  gl_Position = projection * view * frame * points[vid];
}
