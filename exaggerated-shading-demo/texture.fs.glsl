#version 460

uniform sampler2D tex;

layout (location = 0) out vec4 frag_color;

void main() {
  const vec2 uv = gl_FragCoord.xy / vec2(128, 128);
  const float r = length(2 * uv - 1);
  if (r > 1) discard;
  const float alpha = 1.0 - smoothstep(1.0 - 10.0 / 128, 1.0, r);
  frag_color = texture(tex, uv) * vec4(vec3(1.0), alpha);
}
