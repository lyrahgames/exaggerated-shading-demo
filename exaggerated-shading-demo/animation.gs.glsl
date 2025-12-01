#version 460 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

uniform vec2 screen_size = vec2(100, 100);

in vec3 normal[];

out vec3 nor;
out vec3 vnor;
noperspective out vec3 edge_distance;

void main(){
  vec2 p0 = 0.5 * screen_size * (gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w);
  vec2 p1 = 0.5 * screen_size * (gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w);
  vec2 p2 = 0.5 * screen_size * (gl_in[2].gl_Position.xy / gl_in[2].gl_Position.w);

  const float a = length(p1 - p2);
  const float b = length(p2 - p0);
  const float c = length(p1 - p0);
  const float alpha = acos((b * b + c * c - a * a) / (2.0 * b * c));
  const float beta  = acos((a * a + c * c - b * b) / (2.0 * a * c));
  const float ha = abs(c * sin(beta));
  const float hb = abs(c * sin(alpha));
  const float hc = abs(b * sin(alpha));

  const vec3 n = normalize(cross(
    gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz,
    gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz));

  edge_distance = vec3(ha, 0, 0);
  nor = n;
  vnor = normal[0];
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, hb, 0);
  nor = n;
  vnor = normal[1];
  gl_Position = gl_in[1].gl_Position;
  EmitVertex();

  edge_distance = vec3(0, 0, hc);
  nor = n;
  vnor = normal[2];
  gl_Position = gl_in[2].gl_Position;
  EmitVertex();

  EndPrimitive();
}
